# Using ScratchTJ

- [Controls](#controls)
- [Home screen](#home-screen)
- [Cue buttons: CUE and SETTINGS modes](#cue-buttons-cue-and-settings-modes)
- [Menu tree](#menu-tree)
- [Recording](#recording)
- [Settings](#settings)
- [Saving settings](#saving-settings)

---

## Controls

| Control | Action |
|---------|--------|
| **Platter** | Touch and move it to scratch the active deck. Let go and it runs on, pulled back to speed like a record on a slipmat |
| **Fader** | Cuts the decks in and out (SC1000-style hard cut, or a crossfade curve: see [SETTINGS.md](SETTINGS.md#fader-mode)) |
| **Knob, turn** | Scroll menus / change a value. From the home screen, opens the main menu |
| **KEY0 key** | Select / confirm / enter. From the home screen, opens the main menu |
| **Knob, short press** | Back / cancel |
| **Knob, long press (0.5 s)** | Switch the active deck (DECK 1 ↔ DECK 2): platter, cue buttons and screen follow it |
| **Cue buttons 1–4** | Depend on the mode, see below |

---

## Home screen

The home screen shows the active deck:

- a live platter drawing with the fader arc, the position needles and a glow
  while you touch it, plus elapsed time;
- an info column with speed, pitch, motor and slip state;
- a waveform strip right of the platter: about 2 seconds of the deck around
  the playhead (the white line), so you can see where a sound starts and
  ends. Cue points show as dotted lines in their button colour. The waveform
  turns grey while the fader has cut the deck;
- a progress bar;
- a compact strip for the other deck;
- a strip showing what the cue buttons do in the current mode, and the cue
  points as a coloured bar.

The title bar shows **[S]** while the buttons are in SETTINGS mode.

The screen turns itself off after 10 minutes without use. Touch any control to
wake it. The timeout is set in *Config → Screen*.

---

## Cue buttons: CUE and SETTINGS modes

Switch with the main menu item **Btn: CUE/SETTINGS**. The instrument starts in
**SETTINGS** mode.

### CUE mode

| Press | Action |
|-------|--------|
| Short | Jump to the stored cue point |
| Long (1 s) | Store a cue point at the current position |

Cue points are saved per track.

### SETTINGS mode

| Button | Colour | Function | How |
|--------|--------|----------|-----|
| 1 | Red | Pitch | Hold and turn the platter (0.25× to 4×) |
| 2 | Green | Platter speed | Hold and turn the platter |
| 3 | Yellow | Pitch bend | Hold and turn the platter (±8 %, for matching tempo) |
| 4 | Blue | Volume (gain) | Hold and turn the platter (up to ×8; above ×1 it clips, see [Soft clip](SETTINGS.md#soft-clip)) |

While you hold a button and turn the platter, an overlay shows the parameter,
its value and a gauge, and the audio position stays frozen. The settings are
saved when you release the button.

---

## Menu tree

```
HOME SCREEN
│  knob turn or KEY0 → main menu      knob long press → switch active deck
│
├── Dk2 [▶/■] filename 0:00 ...... deck 2
│   ├── Load File ................ folder list → file list, KEY0 loads the file
│   │                              (with no sub-folders: Start/Stop, Next / Previous /
│   │                              Random File, Next / Previous Folder, Record)
│   ├── Start/Stop
│   ├── Settings
│   │   ├── Jog Pitch Mode ....... pitch bend with the platter
│   │   ├── Toggle Jog Reverse ... reverse the platter direction
│   │   └── Platter Speed ........ platter-to-audio ratio
│   └── Info ..................... file, path, position, pitch, volume,
│                                  touch state, motor speed, cue points
│
├── Dk1 [▶/■] filename 0:00 ...... deck 1 (same structure)
│
├── Randomize .................... random track on both decks
│
├── Record Dk2 ................... recording workflow
│
├── Btn: CUE/SETTINGS ............ cue button mode
│
├── Config
│   ├── Sound Settings ........... every ALSA mixer control of the sound card
│   ├── Platter .................. speed, direction, touch sensor, slipmat,
│   │                              brake, tracking, speed assist, glitch filter
│   ├── Fader & Volume ........... fader mode, cut beats, open/close points,
│   │                              direction, curve, smoothing, soft clip
│   ├── Screen ................... screen-off timer
│   ├── Save Config .............. write ~/.scratchtj/config.cfg
│   ├── Reset Defaults ........... back to the defaults (asks first)
│   ├── Restart (rescan files) ... restart the program, re-reading beats/ and samples/
│   └── Reboot Pi
│
└── Info
    ├── CPU/Mem .................. live CPU and memory use
    ├── Version .................. software version and build date
    └── Network / FTP ............ IP addresses and FTP server state
```

---

## Recording

*Record Dk2* (or *Load File → Record* on a deck) records the sound card's line
input:

1. **Record Setup**: *Source* selects the input (KEY0 cycles); *RECORD* starts.
2. **Recording**: a timer and a blinking red dot. KEY0 stops.
3. The recording is loaded straight into the deck.

Recordings are written as raw audio to `/tmp/recNNNNNN.raw`, so they are lost
when the Pi reboots. Copy the ones you want to keep.

---

## Settings

ScratchTJ starts out playing like an SC1000 / SC500, and the extras it adds all
start switched off. Every setting can be changed while you play, from
*Config → Platter*, *Config → Fader & Volume* and *Config → Screen*:

| Section | Settings |
|---------|----------|
| Platter | Platter speed, Reverse platter, Touch threshold, Touch by motion, Slipmat, Brake length, Tracking gain, Pitch response, Speed assist, Glitch filter |
| Fader & Volume | Fader mode (SC1000 cut / Curve), Cut beats, Open point, Close point, Fader direction, Curve width, Curve shape, Fader smoothing, Soft clip |
| Screen | Screen off after |

How to change a setting:

1. Turn the knob to pick a setting, then press **KEY0** to open it.
2. The edit screen shows:
   - the value;
   - a live reading where it helps (touch sensor, fader position, record rpm);
   - the default;
   - a short explanation.
3. Turn the knob to change the value. It takes effect straight away.
4. Press **KEY0** to keep the new value, or push the knob to undo.

**[SETTINGS.md](SETTINGS.md)** explains every setting in plain English, with the
SC1000 values and fixes for common problems.

---

## Saving settings

- Changes made in a Config section are saved to `~/.scratchtj/config.cfg` when
  you leave the section. SETTINGS-mode changes are saved when you release the
  button. *Config → Save Config* saves everything at once.
- **Config → Reset Defaults** asks first, then goes back to the defaults.
- [`software/scsettings.txt`](../software/scsettings.txt) holds start-up
  options (audio buffer size, pitch range, button mappings). It also sets the
  defaults for platter speed, slipmat, brake, fader points and cut beats. A
  value saved from the Config menu overrides it. See
  [SETTINGS.md](SETTINGS.md#scsettingstxt-reference).
