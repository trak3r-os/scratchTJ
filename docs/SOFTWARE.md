# Software

The program in [`software/`](../software/) is a fork of
[xwax](http://www.xwax.co.uk/) by way of the [SC1000](https://github.com/rasteri/SC1000).
xwax provides the audio engine (track loading, playback with pitch, the
platter/turntable model). ScratchTJ adds the hardware input, the display and
menu system, settings and recording, so the whole thing runs as a
stand-alone instrument with no monitor or keyboard.

It is plain C with standard Linux interfaces (ALSA, `/dev/i2c-1`,
`/dev/spidev0.0`, `/dev/mem`, a serial tty). No wiringPi, no Python.

- [Threads](#threads)
- [Data flow](#data-flow)
- [Modules](#modules)
- [Serial protocol](#serial-protocol)
- [Configuration files](#configuration-files)
- [Building](#building)

---

## Threads

```
main (xwax.c)
 ├── realtime audio thread ......... ALSA playback of both decks, high priority
 ├── track import threads .......... decode files through xwax-import
 ├── input thread (sc_input.c) ..... Arduino packets, MT6701 angle, touch,
 │                                   fader cut / curve, coloured buttons,
 │                                   platter → deck position
 └── menu thread (lcd_menu.c) ...... knob + key, menu state machine,
                                     screen rendering
```

Settings are plain `float` globals (`cfg_*`, declared in `settings.h`). The
audio and input threads read them directly, since a float read is atomic. The
menu thread changes them through the registry in `shared_variables.c`.

The menu thread also reads track data to draw the waveform. It holds
`player_track_read_lock()` while it reads. Swapping a deck's track releases
the old track under the same lock, so the track can't be freed mid-read.

## Data flow

```
 Arduino ──USB serial──►┐
  fader, touch, buttons │
                        ▼
 MT6701 ───── I2C ───► sc_input.c ─────► deck / player (xwax) ─────► ALSA ─► AudioInjector
  angle                 │  touch, fader cut,       pitch, position,
                        │  cue/settings buttons    slipmat model
                        ▼
                    lcd_menu.c ◄──── GPIO: knob + key
                        │
                    main_menu.c / deck_menu.c / controller_menu.c / info_menu.c
                        │
                    oled_display.c ─► st7789.c ─── SPI ──► display
```

---

## Modules

### Input and platter

| File | Role |
|------|------|
| `sc_input.c` | The input thread. Opens `/dev/ttyUSB0` at 500 000 baud and parses Arduino packets; reads the MT6701 over I2C (`/dev/i2c-1`, address `0x06`); decides touch (cap value above *Touch threshold*, or a turn bigger than *Motion threshold* when *Touch by motion* is On; with Motor, a virtual motor moves the target on at motor speed and the player feeds that speed forward); sets each deck's fader volume, either with the SC1000 hard cut (open/close hysteresis, cut beats) or with the crossfade curve; handles the coloured buttons (yellow cycles CUE / SETTINGS / PLAY; the other three jump to or set cues, change a setting while held and the platter turns, or change sample and start / stop the beat); converts angle changes to deck position with *Platter speed*. It also time-stamps player activity for the screen saver |
| `platter.c` | Platter angle arithmetic |
| `player.c` | Playback: cubic interpolation, pitch filtering, velocity feed-forward (*Speed assist*), slipmat and brake, fader smoothing, hard clip or soft-clip limiter |
| `deck.c`, `track.c`, `cues.c` | Decks, loaded tracks (imported through `xwax-import`), per-track cue points |
| `recording.c` | Captures from an ALSA input into a file for a deck |

### Display and menus

| File | Role |
|------|------|
| `st7789.c` | SPI driver for the 240×240 ST7789: RGB565 framebuffer, 40 MHz SPI, built-in 8×8 font in three sizes, one transfer per frame. `st7789_sleep()` puts the panel to sleep and wakes it, for the screen saver; the backlight is wired to 3.3 V and stays lit |
| `oled_display.c` | Themed drawing on top of `st7789.c`: title bars, menu lists with scrollbar, value screens, cue bar, progress and fader widgets, value lists, confirm dialogs. Sets the display pins (DC = GPIO 24, RES = GPIO 25). `oled_set_blank()` skips SPI transfers while the screen saver is on |
| `gpio_direct.c` | GPIO by direct register access through `/dev/mem` (base `0x3F200000`, Pi 2/3). Needs root |
| `lcd_menu.c` | The menu thread: debounced knob (CLK 23, DT 22, push 27) and key (17): a click selects, KEY0 or holding the knob for 0.5 s goes back, state machine, redraw timing, screen saver (a knob turn or press that wakes the screen is not passed on to the menu) |
| `main_menu.c` | Home screen and the main menu. The home screen has the platter view, info column, cue bar and settings overlay. It also has a 2 s waveform around the playhead, drawn from the per-64-sample peak meter that xwax fills while importing (`track_get_ppm()`), so a frame costs about 1500 byte reads |
| `deck_menu.c` | Per deck: file and folder browser, transport, settings, info, recording screens |
| `controller_menu.c` | Config: ALSA mixer controls; the Platter, Fader & Volume and Screen sections (value list, edit screen with live readout, default and help; auto-save on leaving); save, reset, restart and reboot |
| `info_menu.c` | CPU / memory, version, network and FTP status |
| `preset_menu.c` | Three preset slots in `~/.scratchtj/presets` (built, not reachable from the menu yet) |
| `settings.c` | The settings table. Each entry has a key, its `cfg_*` global, label, menu section, kind (number / On-Off / choice), range, step, unit and plain-English help. `settings_register()` seeds values from `scsettings.txt`, so those become the defaults |
| `shared_variables.c` | Registry of settings with their metadata; formats values for display; loads and saves `key=value` files (unknown keys skipped, values clamped) |

### Audio and engine

| File | Role |
|------|------|
| `xwax.c` | Start-up: reads `scsettings.txt`, opens the sound card (`plughw:1,0`, 48 kHz), loads the music folders, registers the settings, loads `~/.scratchtj/config.cfg`, starts the threads |
| `alsa.c`, `alsa_mixer.c` | ALSA playback / capture and mixer control access |
| `realtime.c`, `thread.c`, `rig.c` | xwax real-time and event loop infrastructure |
| `midi.c`, `sc_midimap.c`, `dicer.c` | MIDI input and mapping (from the SC1000 code base; optional) |
| `xwax-import`, `xwax-scan` | Shell helpers that decode audio files and scan folders |

---

## Serial protocol

Arduino → Pi, 7 bytes per packet, sent continuously at 500 000 baud:

```
 byte   0      1      2      3      4      5         6
        0xAA   fHi    fLo    cHi    cLo    buttons   XOR of bytes 1–5
        sync   fader (10 bit)  touch (16 bit)  bit 0–3 = cue 1–4
```

- **Handshake:** at start-up the Pi sends `0x53` ('S'), the Arduino answers `'T'`.
- **Parsing:** the Pi keeps a ring buffer, scans for `0xAA` and drops any packet
  whose checksum does not match. The port is opened in low-latency mode.
- **Touch value:** sum of the last 30 single capacitive samples (one sample per
  loop), clamped to 16 bits. The Pi treats a value above *Touch threshold* (default
  5000) as touched.
- **Fader:** read with `analogRead` while the previous packet is on the wire.
- The firmware waits for each packet to finish (`Serial.flush()`) before the
  next touch sample, so the transmit interrupt cannot disturb the measurement
  and the CH340 bridge gets a gap between packets.

Firmware and Pi software must always be updated together.

---

## Configuration files

| File | Contents | Written by |
|------|----------|------------|
| `~/.scratchtj/config.cfg` | Settings (`name=value`); overrides `scsettings.txt` | Config menu sections, *Save Config*, SETTINGS buttons, restart / reboot from the menu |
| `~/ScratchTJ/software/scsettings.txt` | Start-up options and input mappings | You (text editor) |
| `/media/sda/scsettings.txt` | Same, takes priority if present (USB stick) | You |

`scsettings.txt` is a `key=value` file; lines starting with `#` are comments.
The options that matter for ScratchTJ:

| Key | Default | Meaning |
|-----|---------|---------|
| `buffersize` | 384 | ALSA buffer in frames (latency: 384 is 8 ms at 48 kHz) |
| `periods` | 3 | Periods the buffer is split into; one is refilled at a time, so more periods leave more slack against dropouts |
| `faderopenpoint` / `faderclosepoint` | 5 / 3 | Default fader open / close points |
| `cutbeats` | 0 | Default for *Cut beats* (0 off, 1 left end, 2 right end) |
| `platterenabled` | 1 | Platter on / off |
| `platterspeed` | 9100 | Default platter speed |
| `slippiness` | 200 | Default slipmat |
| `brakespeed` | 3000 | Default brake length |
| `jogreverse` | 1 | Default platter direction |
| `pitchrange` | 50 | Pitch range for MIDI pitch controls |

The `gpio=` and `midii=` lines map inputs to actions
(`gpio=port,pin,pullup,edge,ACTION`, `midii=status,channel,data,edge,ACTION`),
inherited from the SC1000 configuration format.

[SETTINGS.md](SETTINGS.md) lists every key of both files.

---

## Building

On the Pi:

```bash
cd ~/ScratchTJ/software
make -j4        # builds ./xwax
make clean
```

Dependencies: gcc, make, `libasound2-dev`, `libsdl2-dev`, pthreads, libm.
The ALSA headers are only on the Pi, so a full build runs there; see
[INSTALLATION.md](INSTALLATION.md#8-developing-from-another-computer) for
syncing from another computer.
