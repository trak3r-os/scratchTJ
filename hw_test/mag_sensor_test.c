/*
 * mag_sensor_test.c - MT6701 magnetic hall rotary sensor test for RPi2
 *
 * Tests a MagnTek MT6701 magnetic rotary encoder via I2C on /dev/i2c-1.
 * First scans for the sensor on common addresses, then polls angle data.
 *
 * Build:  gcc -O2 -o mag_sensor_test mag_sensor_test.c -lm
 * Run:    sudo ./mag_sensor_test
 *
 * Wiring:
 *   Sensor VCC -> RPi Pin 1 (3.3V)
 *   Sensor GND -> RPi Pin 6 (GND)
 *   Sensor SDA -> RPi Pin 3 (GPIO2)
 *   Sensor SCL -> RPi Pin 5 (GPIO3)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

/* MT6701 default I2C address */
#define MT6701_ADDR       0x06
/* MT6701 angle registers: reg 0x03 = angle[13:6], reg 0x04 bits[7:2] = angle[5:0] */
#define REG_ANGLE_HI      0x03
#define REG_ANGLE_LO      0x04

#define SENSOR_CPR        16384  /* 14-bit resolution */
#define I2C_BUS           "/dev/i2c-1"

#define POLL_RATE_HZ      1000
#define POLL_INTERVAL_US  (1000000 / POLL_RATE_HZ)

#define XWAX_CPR          4096  /* Scale down to 12-bit for xwax compatibility */

static volatile bool running = true;
static int i2c_fd = -1;

static void signal_handler(int sig) {
    (void)sig;
    running = false;
}

static int i2c_open(const char *bus, uint8_t addr) {
    int fd = open(bus, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", bus, strerror(errno));
        return -1;
    }
    if (ioctl(fd, I2C_SLAVE, addr) < 0) {
        fprintf(stderr, "Failed to set I2C addr 0x%02X: %s\n", addr, strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}

static int i2c_read_byte(int fd, uint8_t reg, uint8_t *val) {
    if (write(fd, &reg, 1) != 1) return -1;
    if (read(fd, val, 1) != 1) return -1;
    return 0;
}

/* Read MT6701 14-bit angle (0-16383) */
static int read_raw_angle(int fd, uint16_t *angle) {
    uint8_t hi, lo;
    if (i2c_read_byte(fd, REG_ANGLE_HI, &hi) < 0) return -1;
    if (i2c_read_byte(fd, REG_ANGLE_LO, &lo) < 0) return -1;
    /* MT6701: reg 0x03 = angle[13:6], reg 0x04 bits[7:2] = angle[5:0] */
    *angle = ((uint16_t)hi << 6) | (lo >> 2);
    return 0;
}

/* Scan I2C bus for any device, return first found address or -1 */
static int scan_i2c_bus(const char *bus) {
    int fd = open(bus, O_RDWR);
    if (fd < 0) return -1;

    printf("Scanning %s for devices...\n", bus);
    int found = -1;
    for (int addr = 0x03; addr <= 0x77; addr++) {
        if (ioctl(fd, I2C_SLAVE, addr) < 0) continue;
        uint8_t dummy;
        /* Try a simple read to see if device ACKs */
        if (read(fd, &dummy, 1) == 1) {
            printf("  Found device at 0x%02X\n", addr);
            if (found == -1) found = addr;
        }
    }
    close(fd);
    if (found == -1) printf("  No devices found!\n");
    return found;
}

static double timespec_diff_sec(struct timespec *a, struct timespec *b) {
    return (a->tv_sec - b->tv_sec) + (a->tv_nsec - b->tv_nsec) / 1e9;
}

static int wrap_delta(int delta, int cpr) {
    if (delta > cpr / 2) delta -= cpr;
    else if (delta < -cpr / 2) delta += cpr;
    return delta;
}

int main(int argc, char *argv[]) {
    bool verbose = false;
    bool csv_mode = false;
    int force_addr = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) verbose = true;
        else if (strcmp(argv[i], "-c") == 0) csv_mode = true;
        else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
            force_addr = (int)strtol(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [-v] [-c] [-a 0xNN]\n", argv[0]);
            printf("  -v       Verbose (show every reading)\n");
            printf("  -c       CSV output\n");
            printf("  -a 0xNN  Force I2C address (default: scan then 0x%02X)\n", MT6701_ADDR);
            return 0;
        }
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Scan bus first */
    printf("=== MT6701 Magnetic Sensor Test ===\n\n");
    int sensor_addr;

    if (force_addr >= 0) {
        sensor_addr = force_addr;
        printf("Using forced address: 0x%02X\n", sensor_addr);
    } else {
        int found = scan_i2c_bus(I2C_BUS);
        if (found >= 0 && found != 0x1A) {  /* Skip 0x1A = LCD */
            sensor_addr = found;
            printf("Using detected address: 0x%02X\n", sensor_addr);
        } else {
            sensor_addr = MT6701_ADDR;
            printf("No new device found, trying default MT6701 addr: 0x%02X\n", sensor_addr);
        }
    }
    printf("\n");

    /* Open I2C */
    i2c_fd = i2c_open(I2C_BUS, sensor_addr);
    if (i2c_fd < 0) {
        fprintf(stderr, "Cannot open sensor. Is I2C enabled?\n");
        return 1;
    }

    /* Initial read */
    uint16_t angle, last_angle;
    if (read_raw_angle(i2c_fd, &angle) < 0) {
        fprintf(stderr, "Failed to read angle from 0x%02X. Check wiring.\n", sensor_addr);
        close(i2c_fd);
        return 1;
    }
    printf("Initial angle: %d (of %d)\n", angle, SENSOR_CPR);
    last_angle = angle;

    if (csv_mode)
        printf("timestamp_us,raw_angle,xwax_angle,delta,velocity,rpm\n");
    else
        printf("Polling at %d Hz. Ctrl+C to stop.\n\n", POLL_RATE_HZ);

    struct timespec t_start, t_now, t_last;
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    t_last = t_start;

    long total_delta = 0;
    int samples = 0;
    int errors = 0;
    double velocity = 0.0;

    while (running) {
        if (read_raw_angle(i2c_fd, &angle) < 0) {
            errors++;
            if (errors > 100) {
                fprintf(stderr, "Too many read errors, aborting.\n");
                break;
            }
            usleep(1000);
            continue;
        }

        clock_gettime(CLOCK_MONOTONIC, &t_now);
        double dt = timespec_diff_sec(&t_now, &t_last);

        int delta = wrap_delta((int)angle - (int)last_angle, SENSOR_CPR);
        total_delta += delta;
        samples++;

        if (dt > 0.0 && dt < 0.1) {
            double inst_vel = (double)delta / dt;
            velocity = velocity * 0.8 + inst_vel * 0.2;
        }

        double rpm = (velocity / SENSOR_CPR) * 60.0;

        /* Scale 14-bit to 12-bit for xwax */
        int xwax_angle = (int)((long)angle * XWAX_CPR / SENSOR_CPR);

        if (csv_mode) {
            long us = (t_now.tv_sec - t_start.tv_sec) * 1000000L +
                      (t_now.tv_nsec - t_start.tv_nsec) / 1000;
            printf("%ld,%d,%d,%d,%.1f,%.2f\n", us, angle, xwax_angle, delta, velocity, rpm);
        } else if (verbose) {
            printf("raw=%5d  xwax=%4d  delta=%+4d  vel=%+8.1f  rpm=%+7.2f  dt=%.4f\n",
                   angle, xwax_angle, delta, velocity, rpm, dt);
        } else if (samples % 500 == 0) {
            double elapsed = timespec_diff_sec(&t_now, &t_start);
            double actual_hz = samples / elapsed;
            printf("[%6.1fs] angle=%5d  total_turns=%.2f  rpm=%+.2f  rate=%.0fHz  errors=%d\n",
                   elapsed, angle, (double)total_delta / SENSOR_CPR, rpm, actual_hz, errors);
        }

        last_angle = angle;
        t_last = t_now;

        usleep(POLL_INTERVAL_US);
    }

    printf("\n=== Summary ===\n");
    printf("Samples: %d  Errors: %d\n", samples, errors);
    printf("Total rotation: %.2f turns\n", (double)total_delta / SENSOR_CPR);

    close(i2c_fd);
    return 0;
}
