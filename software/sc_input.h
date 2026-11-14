#ifndef SC_INPUT_H
#define SC_INPUT_H

#include <stdint.h>
#include <stdbool.h>

/* The four coloured buttons, by index. Left to right on the panel
 * they are blue, yellow, green, red. Red always switches mode. */
#define BTN_RED     0
#define BTN_GREEN   1
#define BTN_YELLOW  2
#define BTN_BLUE    3

/* What each button does. Red (rightmost on the panel) cycles the mode
 * when tapped, and held switches the deck SETTINGS and PLAY act on
 * (button_deck); the other three are cues in CUE mode, and these in the
 * other modes: */
#define BTN_MODE        BTN_RED
#define BTN_SET_VOLUME  BTN_BLUE    /* SETTINGS: hold + spin */
#define BTN_SET_PITCH   BTN_YELLOW
#define BTN_SET_SPEED   BTN_GREEN
#define BTN_PLAY_PREV   BTN_BLUE    /* PLAY: previous file */
#define BTN_PLAY_START  BTN_YELLOW  /* PLAY: start / stop */
#define BTN_PLAY_NEXT   BTN_GREEN   /* PLAY: next file */

/* Button index at each panel position, left to right */
extern const int button_panel_order[4];

/* What the blue, yellow and green buttons do */
typedef enum {
    FUNC_MODE_CUE = 0,       /* cue points: tap = jump, hold = set */
    FUNC_MODE_SETTINGS = 1,  /* hold + spin = pitch, platter speed, volume */
    FUNC_MODE_PLAY = 2,      /* change file, start/stop */
    FUNC_MODE_COUNT
} FunctionMode;

extern volatile FunctionMode current_function_mode;
extern volatile uint8_t settings_buttons_held;  /* bitmask of held buttons in SETTINGS */
void function_mode_set(FunctionMode mode);
const char *function_mode_name(FunctionMode mode);   /* "SETTINGS" */
const char *function_mode_short(FunctionMode mode);  /* "SET" */
extern volatile int active_deck;                /* 0 or 1: which deck platter controls */
extern volatile int button_deck;                /* 0 or 1: the deck SETTINGS and PLAY act on */

void SC_Input_Start();
void process_rot(int raw, double when);
void process_cue_buttons(uint8_t button_byte);

/* Cue display states (for TFT cue bar) */
extern int cue_display_states[4];

/* Hardware state (for home screen) */
extern bool capIsTouched;
extern unsigned int ADCs[4];
extern volatile unsigned int cap_raw;            /* last touch sensor reading */
extern volatile unsigned long input_activity_ms; /* CLOCK_MONOTONIC ms of the last
                                                    platter/fader/touch/button use */

#endif