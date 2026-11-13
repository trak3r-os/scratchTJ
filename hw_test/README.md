# Hardware tests

Stand-alone programs for checking each part of the hardware without running
the full instrument. Use them after wiring, or when something stops working.

Run them on the Pi, with xwax stopped (`sudo pkill xwax`): xwax holds the
serial port and the display.

```bash
cd ~/ScratchTJ/hw_test
make all            # builds test_all_hw, mag_sensor_test, test_tft_ui
```

| Program | Tests | Run |
|---------|-------|-----|
| [`test_all_hw.c`](test_all_hw.c) | Everything at once | `sudo ./test_all_hw` |
| [`mag_sensor_test.c`](mag_sensor_test.c) | MT6701 angle sensor | `sudo ./mag_sensor_test` |
| [`test_tft_ui.c`](test_tft_ui.c) | Display driver and drawing | `sudo ./test_tft_ui` |
| [`test_tft_diag.py`](test_tft_diag.py) | Display init settings | `sudo python3 test_tft_diag.py` |
| [`arduino_test/`](arduino_test/) | Arduino firmware for serial tests | Flash to the Nano |

---

## test_all_hw

Brings up every device and shows them live on the display:

- **MT6701**: angle in degrees and a dial (or *NO SIGNAL*);
- **Arduino**: fader value, touch value and the four buttons;
- **Knob and key**: position count, push and KEY0 state.

At start-up it prints *OK* / *FAILED* for each device to the terminal. Stop with
Ctrl+C. It is self-contained (no code from `software/`), so it also tells you
whether a fault is in the wiring or in the main program.

## mag_sensor_test

Scans the I2C bus, finds the MT6701 (address `0x06`) and polls the angle at
1 kHz. Once a second it prints the raw angle (0–16383), total turns, rpm,
read rate and error count.

```bash
sudo ./mag_sensor_test          # summary
sudo ./mag_sensor_test -v       # every reading
sudo ./mag_sensor_test -c       # CSV output, e.g. > angle.csv
sudo ./mag_sensor_test -a 0x06  # skip the scan, use this address
```

Turn the platter slowly through a full turn: the raw angle should change
smoothly through 0–16383 and wrap with no jumps. Jumps or a stuck value mean the magnet
is off-centre, too far from the chip (it should be 1 mm) or not diametric.

## test_tft_ui

Draws a mock home screen with moving values using the same display code as
the main program (`software/st7789.c`, `oled_display.c`, `gpio_direct.c`).
If this works and xwax shows nothing, the problem is not the display.

## test_tft_diag.py

For a display that stays blank or shows garbage. It cycles through ten
combinations of SPI speed, orientation, colour inversion and offset, three
seconds each, and fills the screen red. Note which one shows a solid red
screen. Needs `python3-spidev` and `python3-rpi.gpio`
(`sudo apt install python3-spidev python3-rpi.gpio`). If none works, check the
SPI wiring: MOSI → SDA (pin 19), SCLK → SCL (pin 23), CE0 → CS (pin 24).

## arduino_test

A simpler firmware that sends the same 7-byte packets as the real one, at a
fixed 200 Hz and with one 30-sample touch reading per packet. Use it to rule
out the firmware when the fader, touch or buttons do not respond: flash it,
then run `test_all_hw`. Flash
[`arduino_nano/arduino_nano.ino`](../arduino_nano/arduino_nano.ino) back
afterwards.

To look at raw packets without any program:

```bash
stty -F /dev/ttyUSB0 500000 raw && xxd /dev/ttyUSB0 | head
```

Every packet starts with `aa`.
