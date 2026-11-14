# Hardware

This page lists every component, explains what it does and why it was chosen,
and gives the complete wiring. For the printed case, see
[../enclosure/README.md](../enclosure/README.md).

- [Components](#components)
- [Wiring](#wiring)
- [Capacitive touch](#capacitive-touch)
- [Platter sensing](#platter-sensing)
- [Power](#power)
- [Checking the hardware](#checking-the-hardware)

---

## Components

### Raspberry Pi 2 Model B

The brain of the instrument. It runs the xwax audio engine, the platter and
fader processing, the menus and the display driver.

| | |
|---|---|
| SoC | Broadcom BCM2836, 4 × ARM Cortex-A7 @ 900 MHz (32-bit, ARMv7) |
| RAM | 1 GB |
| Ports used | 40-pin GPIO header (sound card, sensor, display, knob), 1 USB port (Arduino), micro-USB power |
| Network | 100 Mbit Ethernet. **No built-in Wi-Fi**: use the Ethernet port or a USB Wi-Fi dongle for FTP / SSH |
| OS | Raspberry Pi OS **32-bit** (armhf) |

Why a Pi 2: four cores are enough to run audio at 48 kHz with a 256-frame
buffer while the input thread polls the sensors and the display redraws, and it
fits the case with the sound card on top.

**Other models.** The direct GPIO driver (`software/gpio_direct.c`) maps the
BCM2836/BCM2837 peripheral base `0x3F200000`, so a **Raspberry Pi 3** works
without changes. A Pi 4 uses `0xFE200000` and a Pi 1 / Zero uses `0x20200000`:
change `GPIO_BASE` before using those. The Pi 5 has a different GPIO block and
is not supported. The case is sized for the Pi 2/3 B board outline and port
layout.

You also need a **microSD card** (8 GB or more) and a **5 V / 2 A micro-USB
power supply**.

### AudioInjector Stereo sound card

A HAT that plugs onto the Pi header: a Wolfson **WM8731** codec over I2S,
stereo RCA line in and line out, a 3.5 mm headphone jack and a volume
potentiometer.

- Both decks play through it; the software opens it as `plughw:1,0` at 48 kHz.
- Its line input is the source for the **Record** function.
- The codec is configured over I2C (address `0x1a`) on the same bus as the
  MT6701 (`0x06`); the two addresses do not clash.

The case has openings for both RCA pairs, the headphone jack and both
potentiometer shafts.

### Arduino Nano

Reads the analog and timing-critical inputs and sends them to the Pi over its
own USB cable (`/dev/ttyUSB0`, 500 000 baud):

- the fader on **A5** (10-bit ADC),
- the capacitive touch sensor on **D10 / D12**,
- the four coloured buttons on **A0–A3**.

Why an Arduino: the Pi has no analog inputs, and capacitive sensing needs
tight microsecond timing that a Linux user-space program cannot guarantee.
The USB cable also powers the Nano, so no extra wiring is needed.

Any Nano works, including the cheap **CH340** clones, with an ATmega328P or
an ATmega168: the firmware is small. Install the `CapacitiveSensor` library to
build it.

### MT6701 magnetic angle sensor

A 23 × 23 mm breakout board with the MagnTek MT6701 hall-effect angle sensor.
It sits in a tube in the tray, 1 mm under a magnet glued to the tip of the
platter bolt, and the Pi reads the absolute angle over **I2C** (address
`0x06`, 14 bits = 16 384 steps per turn).

Why: it is contactless (no wear, no shaft coupling), absolute (no homing, no
drift), reads zero noise when the platter is still, and a read takes under a
millisecond at 100 kHz I2C. It needs only a magnet and a correct air gap,
which the bearing keeps constant.

You also need a small **diametrically magnetised disc magnet**, **Ø4 × 2 mm**,
glued to the end of the M6 bolt. These are usually sold together with MT6701
boards.

### Display module: ST7789 1.54" 240×240 + EC11 knob + key

One 40 × 65 mm board that carries:

- a **1.54" ST7789 colour display**, 240 × 240 pixels, on SPI,
- an **EC11 rotary encoder** with push switch (turn to scroll, click to
  select, hold for half a second to go back),
- a **push key** labelled `KEY0` (back).

It has a single 12-pin header, which keeps the wiring simple. The display is
driven at 40 MHz SPI from a framebuffer in memory.

### Innofader Mini Pro

The scratch fader. It is a contactless hall-effect fader with a very short cut
distance, designed for scratching. Its output is a clean analog voltage, so
the cut points can sit very close to the end of travel. It is mounted under
the high deck with two M3 screws into its own threaded holes (65 mm apart).

Do not drive long screws into the fader: with the 3 mm deck, use M3 × 6
countersunk or M3 × 5 pan head.

### Coloured buttons

Four momentary push buttons with a 7 mm threaded bushing, mounted through the
low deck. They connect between Arduino A0–A3 and GND (internal pull-ups, no
resistors). Use four colours: on the panel, from left to right, **blue,
yellow, green, red**. Yellow is the mode button (CUE / SETTINGS / PLAY); what
the other three do depends on the mode, see [USAGE.md](USAGE.md).

### Platter and axle

| Part | Size | Notes |
|------|------|-------|
| Platter | 95 mm diameter, 1 mm thick, 25 mm centre hole | An aluminium platter from a 3.5" hard disk is ideal: flat, balanced and conductive |
| Ball bearing | **626ZZ**, 6 × 19 × 6 mm | Press fit into a Ø19.15 seat in the high deck, 0.2 mm proud |
| Bolt | **M6 pan head**, 25.5 mm under the head (about 27 mm overall) | The axle. It turns with the platter |
| Nut | M6, 4 mm thick | Under the bearing, locks the bolt to the inner race |
| Magnet | Ø4 × 2 mm, diametric | Glued to the bolt tip, 1 mm over the MT6701 chip |

The printed hub, spacer, bearing retainer, bearing collar and platter cap
(which replaces a wide washer) complete the stack; see the
[platter stack in the enclosure guide](../enclosure/README.md#platter-stack).

### Small parts

- 1 × **1.2 kΩ resistor** (touch circuit)
- Thin insulated wire for the touch contact (about 0.5 mm)
- **DuPont jumper wires**, female–female, for the sensor, display and Arduino
- **M3 and M2 screws**: the full list is in the
  [enclosure guide](../enclosure/README.md#screws-and-hardware)

---

## Wiring

### Overview

```
                         RASPBERRY PI 2 (BCM numbering, physical pin in brackets)
                         ┌────────────────────────────────────────────────────┐
 AudioInjector HAT  ◄───►│ I2S: GPIO 18, 19, 20, 21 (on the HAT connector)    │
                         │                                                    │
 MT6701         SDA ◄───►│ GPIO 2  [3]   I2C1 SDA                             │
                SCL ◄────│ GPIO 3  [5]   I2C1 SCL                             │
                         │                                                    │
 Display        SCL ◄────│ GPIO 11 [23]  SPI0 SCLK                            │
                SDA ◄────│ GPIO 10 [19]  SPI0 MOSI                            │
                CS  ◄────│ GPIO 8  [24]  SPI0 CE0                             │
                DC  ◄────│ GPIO 24 [18]                                       │
                RES ◄────│ GPIO 25 [22]                                       │
                         │                                                    │
 EC11 knob      A   ────►│ GPIO 23 [16]  (CLK)                                │
                B   ────►│ GPIO 22 [15]  (DT)                                 │
                push────►│ GPIO 27 [13]  (click = select, hold = back)        │
 KEY0 key       ────────►│ GPIO 17 [11]  (back)                               │
                         │                                                    │
 Arduino Nano   ◄═══════►│ USB port  (/dev/ttyUSB0, 500 000 baud, also power) │
                         └────────────────────────────────────────────────────┘

                         ARDUINO NANO
                         ┌────────────────────────────────────────────────────┐
 Innofader      out ────►│ A5                                                 │
                VCC ◄────│ 5V                                                 │
                GND ─────│ GND                                                │
 Red button     ────────►│ A0  ─┐                                             │
 Yellow (mode)  ────────►│ A1   │ other side of each button to GND           │
 Blue button    ────────►│ A2   │                                             │
 Green button   ────────►│ A3  ─┘                                             │
                         │ D10 ──[ 1.2 kΩ ]──┐                                │
 Touch wire     ────────►│ D12 ──────────────┘  (wire to the bearing seat)    │
                         └────────────────────────────────────────────────────┘
```

### Raspberry Pi pin map

| BCM GPIO | Physical pin | Connects to | Direction |
|----------|--------------|-------------|-----------|
| 2 | 3 | MT6701 SDA | I2C |
| 3 | 5 | MT6701 SCL | I2C |
| 8 | 24 | Display CS | Out |
| 10 | 19 | Display SDA (MOSI) | Out |
| 11 | 23 | Display SCL (SCLK) | Out |
| 17 | 11 | KEY0 key (back) | In, active low |
| 22 | 15 | EC11 B (DT) | In |
| 23 | 16 | EC11 A (CLK) | In |
| 24 | 18 | Display DC | Out |
| 25 | 22 | Display RES | Out |
| 27 | 13 | EC11 push switch (select; hold = back) | In, active low |
| 18–21 | 12, 35, 38, 40 | AudioInjector (I2S) | Used by the HAT |

Power both the MT6701 and the display module from the Pi **3.3 V** pins (1 or
17) and any GND pin. The display backlight pin (BLK), if your board has one,
goes to 3.3 V.

The pin numbers are set in the source: `software/lcd_menu.h` (knob and key)
and `software/oled_display.c` (display DC / RES).

### Arduino pin map

| Pin | Connects to | Notes |
|-----|-------------|-------|
| A5 | Fader output | 10-bit analog |
| A0 | Red button | To GND when pressed, internal pull-up |
| A1 | Yellow button (mode) | " |
| A2 | Blue button | " |
| A3 | Green button | " |
| D10 | Touch send | Through the 1.2 kΩ resistor to D12 |
| D12 | Touch sense | Also to the touch wire |
| 5V / GND | Fader supply | |
| USB | Raspberry Pi USB port | Data and power |

Leave the Arduino plugged into the Pi permanently: the Pi can reset it (DTR)
and even re-flash it in place (see [INSTALLATION.md](INSTALLATION.md#6-flash-the-arduino)).

---

## Capacitive touch

The software needs to know when your hand is on the platter: while you touch
it, the platter controls the audio; when you let go, the record runs on with a
slipmat-style pull back to speed.

```
 Arduino D10 ──[ 1.2 kΩ ]──┬── Arduino D12
                           │
                     touch wire  (runs up a groove in the bearing seat,
                           │       pinched against the bearing outer race)
                     bearing outer race → balls → inner race
                           │
                     M6 bolt → short stripped wire (under the bolt head,
                           │       in the groove under the cap) → platter
                           │
                     your hand (body capacitance)
```

The `CapacitiveSensor` library sends a pulse on D10 and times how long D12
takes to follow through the resistor. A hand on the platter adds capacitance,
so the time goes up sharply. The firmware takes one sample per loop and
reports the sum of the last 30. The Pi treats a value **above 5000** as
touched. You can change this in *Config → Platter → Touch threshold*: the edit
screen shows the live sensor value, so you can read it with your hand off and
on the platter and set the threshold halfway between.

There is no slip ring: the signal goes through the bearing itself. The wire
lies in a vertical groove in the bearing seat and the bearing presses it
against the plastic, and from there the path is metal to metal up to the
platter. The printed top cap is plastic, so a short stripped wire carries the
signal from the bolt to the platter: one end pinched under the bolt head, the
other in the groove on the underside of the cap, pressed against the platter
top.

**Keep your hands off the platter for the first two seconds after start-up.**
The serial port resets the Arduino when xwax opens it, and the touch baseline
is measured right then.

**No touch?** Set *Config → Platter → Touch by motion* to **On** or
**Motor**. With On, a moving platter counts as touched, and a platter held
still is released after a fraction of a second. With Motor, the record always
turns at normal speed and the platter adds to or takes from it. Either lets you
play while you debug the touch circuit; see [SETTINGS.md](SETTINGS.md#touch-by-motion).
Turn it off again once touch works.

---

## Platter sensing

```
   platter  ═══════════════════
   bolt          ┃  turns with the platter (M6)
   bearing      ▓┃▓   626ZZ
   collar + nut  ┃
   magnet        ▀   Ø4 x 2, glued to the bolt tip
                  1 mm air gap
   MT6701  ▬▬▬▬▬▬▬▬▬  fixed in the tray
```

- The magnet must be **diametrically** magnetised (north and south on opposite
  sides of the disc, not on the faces).
- Centre it on the bolt and glue it flat; keep the chip right under the axis.
- The bearing holds the gap constant through the whole turn. The model sets
  the gap to 1 mm.
- The gap is set by the bolt length and the thickness of the printed top cap.
  The model assumes a 25.5 mm bolt (under the head); with a 24.6 mm bolt the
  gap is 1.9 mm, still in the sensor's range. If it is wrong on the real
  thing, reprint only the cap (1 mm thicker moves the magnet 1 mm up).
- The Pi converts angle changes into playback position. The ratio is the
  `platterspeed` parameter; the default 9100 is set for this sensor.

---

## Power

The whole instrument runs from the Pi's micro-USB supply: the Pi powers the
sound card through the header, the MT6701 and display from its 3.3 V pins, and
the Arduino (and through it the fader) from the USB port. A good **5 V / 2 A**
supply is enough. The power socket and HDMI are reachable through the back
wall, and the microSD card through the right wall.

---

## Checking the hardware

| Check | How |
|-------|-----|
| Sensor present | `i2cdetect -y 1` shows `06` (the sound card codec shows as `UU` at `1a` once its driver is loaded) |
| Sensor reads | `i2cget -y 1 0x06 0x03 w` changes as you turn the platter |
| Platter turns freely | Spin by hand: no rubbing, no wobble |
| Arduino talks | `ls /dev/ttyUSB0`, then the Arduino test in [`hw_test/`](../hw_test/) |
| Sound card | `aplay -l` lists the AudioInjector as card 1 |
| Display, knob, key, sensor, Arduino together | `cd hw_test && make test_all_hw && sudo ./test_all_hw` |

More tools: [../hw_test/README.md](../hw_test/README.md).
