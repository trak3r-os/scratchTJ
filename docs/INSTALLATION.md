# Installation

From a blank microSD card to a running instrument.

- [1. Raspberry Pi OS](#1-raspberry-pi-os)
- [2. Interfaces and sound card](#2-interfaces-and-sound-card)
- [3. Build the software](#3-build-the-software)
- [4. Run and autostart](#4-run-and-autostart)
- [5. Music files](#5-music-files)
- [6. Flash the Arduino](#6-flash-the-arduino)
- [7. FTP access (optional)](#7-ftp-access-optional)
- [8. Developing from another computer](#8-developing-from-another-computer)
- [Troubleshooting](#troubleshooting)

---

## 1. Raspberry Pi OS

1. Flash **Raspberry Pi OS Lite (32-bit)** with Raspberry Pi Imager. The
   Raspberry Pi 2 is 32-bit only.
2. In the Imager settings, enable SSH and create the user **`pi`**.

   > The software uses fixed paths under `/home/pi` (music folders, settings
   > and saved config). With that user name everything works as is. To use
   > another user, replace the paths before building:
   >
   > ```bash
   > grep -rl '/home/pi' software docs/setup_ftp.sh | xargs sed -i "s#/home/pi#$HOME#g"
   > sed -i -e "s/ftp_username=pi/ftp_username=$USER/" -e "s/'pi\\n/'$USER\\n/" \
   >     -e "s/passwd -S pi/passwd -S $USER/" -e "s/act as pi,/act as $USER,/" docs/setup_ftp.sh
   > ```

3. Boot the Pi, log in over SSH and install the packages:

   ```bash
   sudo apt update && sudo apt upgrade -y
   sudo apt install -y build-essential git libasound2-dev libsdl2-dev \
       i2c-tools ffmpeg mpg123 avrdude
   ```

   | Package | Used for |
   |---------|----------|
   | `build-essential` | gcc and make |
   | `git` | getting the code |
   | `libasound2-dev` | ALSA audio headers |
   | `libsdl2-dev` | SDL2 headers (the xwax build links against it) |
   | `i2c-tools` | `i2cdetect` / `i2cget` to check the sensor |
   | `ffmpeg`, `mpg123` | decoding MP3, FLAC, OGG and other non-WAV files |
   | `avrdude` | flashing the Arduino from the Pi (optional) |

4. Generate the **en_GB.UTF-8** locale (xwax refuses to start without a UTF-8
   locale): `sudo raspi-config` → *Localisation Options* → *Locale* → tick
   `en_GB.UTF-8 UTF-8`.

---

## 2. Interfaces and sound card

### I2C and SPI

`sudo raspi-config` → *Interface Options*: enable **I2C** (MT6701 sensor) and
**SPI** (display). The serial port is not needed: the Arduino talks over USB.

### AudioInjector overlay

Add to `/boot/config.txt` (`/boot/firmware/config.txt` on newer images):

```
dtoverlay=audioinjector-wm8731-audio
```

**Keep the on-board audio enabled** (`dtparam=audio=on`). The software opens
the AudioInjector as **card 1** (`plughw:1,0`); the on-board audio is card 0.
Turning it off would make the AudioInjector card 0.

Make card 1 the default mixer, so the volume controls in the menu act on the
AudioInjector. Create `/etc/asound.conf`:

```
pcm.!default { type hw card 1 }
ctl.!default { type hw card 1 }
```

Reboot, then check:

```bash
aplay -l          # card 1: audioinjector...
i2cdetect -y 1    # 06 = MT6701 (the codec shows as UU at 1a)
```

Turn the output on and set a level (the menu can change these later, under
*Config → Sound Settings*):

```bash
amixer -c 1 sset 'Output Mixer HiFi' on
amixer -c 1 sset 'Line' 31
```

---

## 3. Build the software

```bash
git clone <this repository> ~/ScratchTJ
cd ~/ScratchTJ/software
make -j4
```

The result is `~/ScratchTJ/software/xwax`. A full build takes a few minutes on
a Pi 2.

The settings file is read from `~/ScratchTJ/software/scsettings.txt` (or from
`/media/sda/scsettings.txt` if a USB stick with one is mounted there), so
clone the repository to `~/ScratchTJ`.

---

## 4. Run and autostart

### Run by hand

```bash
cd ~/ScratchTJ/software
sudo LC_ALL=en_GB.utf8 nice -n -19 ./xwax
```

- `sudo`: direct access to GPIO (`/dev/mem`), SPI and I2C.
- `LC_ALL=en_GB.utf8`: without it xwax stops with *"Could not honour the local
  encoding"*.
- `nice -n -19`: high priority for the audio and input threads.

Keep your hands off the platter for the first two seconds: the Arduino resets
when the serial port opens and measures the touch baseline right then.

### Start at boot

Add this line to `/etc/rc.local`, before `exit 0`:

```bash
cd /home/pi/ScratchTJ/software && LC_ALL=en_GB.utf8 nice -n -19 ./xwax &
```

`rc.local` runs as root, so `sudo` is not needed. After a rebuild, reboot (or
use *Config → Reboot Pi*) to start the new binary.

---

## 5. Music files

| Folder | Deck |
|--------|------|
| `~/beats/` | Deck 1: beats and loops |
| `~/samples/` | Deck 2: scratch samples |

Sub-folders work: the file browser and *Next / Previous Folder* walk them.
At start-up the software also tries to load `~/samples/scratch.wav` on deck 2.

**Formats:** WAV, MP3, FLAC, OGG and anything else `ffmpeg` can decode
(handled by `software/xwax-import`).

**Fastest:** 16-bit stereo WAV with a plain 44-byte header. These load
directly, with no decoding. Convert with:

```bash
ffmpeg -y -i input.mp3 -map 0:a -ac 2 -acodec pcm_s16le \
    -bitexact -map_metadata -1 output.wav
```

`-bitexact -map_metadata -1` matters: a WAV with tags or cover art has a
longer header, and the fast path then plays the header as noise. Mono files
must be converted to stereo (`-ac 2`).

The folders are scanned at start-up. After adding files, use
*Config → Restart (rescan files)*.

---

## 6. Flash the Arduino

The firmware is [`arduino_nano/arduino_nano.ino`](../arduino_nano/arduino_nano.ino)
and needs the **CapacitiveSensor** library (Library Manager:
"CapacitiveSensor" by Paul Badger).

### From a computer (Arduino IDE)

1. Open the sketch, install CapacitiveSensor.
2. *Tools → Board*: **Arduino Nano**. *Processor*: **ATmega328P (Old
   Bootloader)** for most CH340 clones, or **ATmega168** if your board has that
   chip.
3. Select the port and upload.

### In place, from the Pi

The Nano can stay plugged into the Pi. Build the `.hex` on a computer with
`arduino-cli`:

```bash
arduino-cli compile --fqbn arduino:avr:nano:cpu=atmega328old --output-dir out arduino_nano
# ATmega168 board: --fqbn arduino:avr:nano:cpu=atmega168
```

copy `out/arduino_nano.ino.hex` to the Pi, stop xwax (it holds the port), and
flash:

```bash
sudo pkill xwax
avrdude -p atmega328p -c arduino -P /dev/ttyUSB0 -b 57600  -U flash:w:arduino_nano.ino.hex:i
# ATmega168:      -p atmega168                    -b 19200
```

The firmware and the Pi software share the serial protocol: update both
together.

---

## 7. FTP access (optional)

To manage `beats` and `samples` from another computer:

```bash
bash ~/ScratchTJ/docs/setup_ftp.sh
```

It installs `vsftpd` with an FTP root that contains only the two music
folders. You can log in as `anonymous` (no password) or as the Pi user with
its password. Both can upload, rename and delete, so anyone on your network can
change the music. The script's comments show how to make anonymous access
read-only. The Pi's address is shown under *Info → Network / FTP*.

FTP is not encrypted: use it on your own network only.

---

## 8. Developing from another computer

Edit on your computer, then sync and build on the Pi:

```bash
rsync -avz --exclude='.git' --exclude='*.o' --exclude='*.d' \
    software/ pi@<pi-address>:~/ScratchTJ/software/
ssh pi@<pi-address> "cd ~/ScratchTJ/software && make -j4"
```

Start xwax from the Pi itself (or reboot it), not through a long-running SSH
command: at `nice -19` the audio threads can starve the SSH session.

Useful commands on the Pi:

```bash
i2cdetect -y 1                     # sensor at 06
i2cget -y 1 0x06 0x03 w            # raw angle, changes as the platter turns
stty -F /dev/ttyUSB0 500000 raw && xxd /dev/ttyUSB0 | head   # raw Arduino packets (0xAA ...)
```

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| *Could not honour the local encoding* | Generate `en_GB.UTF-8` and start with `LC_ALL=en_GB.utf8` |
| No sound | `aplay -l` must list the AudioInjector as **card 1**; check the overlay, keep on-board audio on, check `amixer -c 1` |
| Blank display | SPI enabled? Check DC (GPIO 24) and RES (GPIO 25) |
| Platter does nothing | `i2cdetect -y 1` must show `06`; check the magnet is diametric and 1 mm over the chip |
| Platter works only while moving / never "touched" | Check the touch wire (bearing seat groove, and the short wire from the bolt head to the platter); do not touch the platter at start-up; set `motion_touch = 1` to play meanwhile |
| Fader or buttons dead | `/dev/ttyUSB0` must exist; re-flash the Arduino; nothing else may hold the port |
| Permission denied | Run with `sudo` |
| WAV plays as noise | The file has metadata: convert with `-bitexact -map_metadata -1` |
| WAV plays at double speed | Mono file: convert with `-ac 2` |
| New files do not appear | *Config → Restart (rescan files)* |
