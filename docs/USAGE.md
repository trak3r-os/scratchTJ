# Using ScratchTJ

- [Controls](#controls)
- [Home screen](#home-screen)
- [Cue buttons: CUE and SETTINGS modes](#cue-buttons-cue-and-settings-modes)
- [Menu tree](#menu-tree)
- [Recording](#recording)
- [Live parameters](#live-parameters)
- [Saving settings](#saving-settings)

---

## Controls

| Control | Action |
|---------|--------|
| **Platter** | Touch and move it to scratch the active deck. Let go and it runs on, pulled back to speed like a record on a slipmat |
| **Fader** | Cuts the scratch deck in and out |
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
- a fader history graph and a progress bar;
- a compact strip for the other deck;
- a strip showing what the cue buttons do in the current mode, and the cue
  points as a coloured bar.

The title bar shows **[S]** while the buttons are in SETTINGS mode.

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
| 4 | Blue | Volume (gain) | Hold and turn the platter (up to ×8, soft-clipped) |

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
│   ├── Global Settings .......... every live parameter (table below);
│   │                              fader parameters show the curve graph
│   ├── Save Config .............. write ~/.scratchtj/config.cfg
│   ├── Reset Defaults ........... back to factory values
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

## Live parameters

All of these can be changed while playing, from *Config → Global Settings*.

| Parameter | Default | Range | Step | What it does |
|-----------|---------|-------|------|--------------|
| Fad Factor | 0.1 | 0.01–10 | 0.01 | Fader curve transition point |
| Fad Power | 0.2 | 0.01–10 | 0.01 | Fader curve exponent |
| Fad Switch | 1 | 0–1 | 1 | Fader direction (0 = normal, 1 = reversed) |
| fader_sharp | 1.0 | 0–1 | 0.05 | Fader sharpness |
| slippiness | 200 | 1–3000 | 25 | Slipmat feel: how fast the record returns to speed after you let go |
| target_pitch | 40 | 1–240 | 1 | Position tracking gain |
| pitch_filter | 0.1 | 0.01–1 | 0.01 | Pitch low-pass (small = smooth, 1 = instant) |
| vel_ff | 1.0 | 0–1 | 0.1 | Platter velocity feed-forward: 1 = audio locked to your hand, 0 = position tracking only (then `target_pitch` and `pitch_filter` govern the feel) |
| platterspeed | 9100 | 1–32768 | 256 | Platter-to-audio ratio: how much audio one turn of the platter covers |
| blipthreshold | 400 | 50–2048 | 50 | Rejects sensor glitches larger than this |
| brakespeed | 3000 | 1–10000 | 500 | How fast the deck stops on Start/Stop |
| motion_touch | 0 | 0–1 | 1 | 1 = a moving platter counts as touched (play without the touch sensor); a platter held still is released after 0.12 s |
| fhist_secs | 2.0 | 0.1–120 | 0.1 | Time window of the fader history graph, in seconds |

---

## Saving settings

- **Config → Save Config** writes every live parameter to
  `~/.scratchtj/config.cfg`; it is loaded at start-up. SETTINGS-mode changes
  are saved automatically.
- **Config → Reset Defaults** restores the values in the table above.
- Start-up options that are not live parameters (audio buffer size, fader
  hysteresis points, pitch range) are in
  [`software/scsettings.txt`](../software/scsettings.txt). See
  [SOFTWARE.md](SOFTWARE.md#configuration-files).
