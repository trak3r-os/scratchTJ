# Software

The program in [`software/`](../software/) is a fork of
[xwax](http://www.xwax.co.uk/) by way of the [SC1000](https://github.com/rasteri/SC1000).
xwax provides the audio engine (track loading, playback with pitch, the
platter/turntable model). ScratchTJ adds the hardware input, the display and
menu system, live parameters and recording, so the whole thing runs as a
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
 │                                   fader curve, cue / settings buttons,
 │                                   platter → deck position
 └── menu thread (lcd_menu.c) ...... knob + key, menu state machine,
                                     screen rendering
```

Values shared between threads (the live parameters) go through the
mutex-protected registry in `shared_variables.c`.

## Data flow

```
 Arduino ──USB serial──►┐
  fader, touch, buttons │
                        ▼
 MT6701 ───── I2C ───► sc_input.c ─────► deck / player (xwax) ─────► ALSA ─► AudioInjector
  angle                 │  touch, fader curve,     pitch, position,
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
| `sc_input.c` | The input thread. Opens `/dev/ttyUSB0` at 500 000 baud and parses Arduino packets; reads the MT6701 over I2C (`/dev/i2c-1`, address `0x06`); decides touch (cap value > 5000, or motion when `motion_touch` is on); applies the fader curve; handles cue / settings buttons; converts angle changes to deck position with `platterspeed` |
| `platter.c` | Platter angle arithmetic |
| `player.c` | Playback: cubic interpolation, pitch filtering, velocity feed-forward, slipmat simulation, soft-clip limiter |
| `deck.c`, `track.c`, `cues.c` | Decks, loaded tracks (imported through `xwax-import`), per-track cue points |
| `recording.c` | Captures from an ALSA input into a file for a deck |

### Display and menus

| File | Role |
|------|------|
| `st7789.c` | SPI driver for the 240×240 ST7789: RGB565 framebuffer, 40 MHz SPI, built-in 8×8 font in three sizes, one transfer per frame |
| `oled_display.c` | Themed drawing on top of `st7789.c`: title bars, menu lists with scrollbar, value screens, cue bar, progress and fader widgets, confirm dialogs. Sets the display pins (DC = GPIO 24, RES = GPIO 25) |
| `gpio_direct.c` | GPIO by direct register access through `/dev/mem` (base `0x3F200000`, Pi 2/3). Needs root |
| `lcd_menu.c` | The menu thread: debounced knob (CLK 23, DT 22, push 27) and key (17), short / long press, state machine, redraw timing |
| `main_menu.c` | Home screen (platter view, info column, fader graph, cue bar, settings overlay) and the main menu |
| `deck_menu.c` | Per deck: file and folder browser, transport, settings, info, recording screens |
| `controller_menu.c` | Config: ALSA mixer controls, live parameters, save / reset, restart, reboot |
| `info_menu.c` | CPU / memory, version, network and FTP status |
| `preset_menu.c` | Three preset slots in `~/.scratchtj/presets` (built, not reachable from the menu yet) |
| `shared_variables.c` | Registry of live parameters: name, pointer, range, step; load / save `key=value` files |

### Audio and engine

| File | Role |
|------|------|
| `xwax.c` | Start-up: reads `scsettings.txt`, opens the sound card (`plughw:1,0`, 48 kHz), loads the music folders, registers the live parameters, loads `~/.scratchtj/config.cfg`, starts the threads |
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
  loop), clamped to 16 bits. The Pi treats a value above 5000 as touched.
- **Fader:** read with `analogRead` while the previous packet is on the wire.
- The firmware waits for each packet to finish (`Serial.flush()`) before the
  next touch sample, so the transmit interrupt cannot disturb the measurement
  and the CH340 bridge gets a gap between packets.

Firmware and Pi software must always be updated together.

---

## Configuration files

| File | Contents | Written by |
|------|----------|------------|
| `~/.scratchtj/config.cfg` | Live parameters (`name=value`) | *Config → Save Config*, SETTINGS buttons |
| `~/ScratchTJ/software/scsettings.txt` | Start-up options and input mappings | You (text editor) |
| `/media/sda/scsettings.txt` | Same, takes priority if present (USB stick) | You |

`scsettings.txt` is a `key=value` file; lines starting with `#` are comments.
The options that matter for ScratchTJ:

| Key | Default | Meaning |
|-----|---------|---------|
| `buffersize` | 256 | ALSA period in frames (latency) |
| `faderopenpoint` / `faderclosepoint` | 5 / 3 | Fader hysteresis points |
| `platterenabled` | 1 | Platter on / off |
| `platterspeed` | 9100 | Initial platter ratio |
| `brakespeed` | 3000 | Initial stop speed |
| `pitchrange` | 50 | Pitch range for MIDI pitch controls |

The `gpio=` and `midii=` lines map inputs to actions
(`gpio=port,pin,pullup,edge,ACTION`, `midii=status,channel,data,edge,ACTION`),
inherited from the SC1000 configuration format.

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
