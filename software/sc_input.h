#ifndef SC_INPUT_H
#define SC_INPUT_H

#include <stdint.h>
#include <stdbool.h>

/* The four coloured buttons, by index. Left to right on the panel
 * they are blue, yellow, green, red. Yellow always switches mode. */
#define BTN_RED     0
#define BTN_GREEN   1
#define BTN_YELLOW  2
#define BTN_BLUE    3
#define BTN_MODE    BTN_YELLOW

/* Button index at each panel position, left to right */
extern const int button_panel_order[4];

/* What the red, green and blue buttons do */
typedef enum {
    FUNC_MODE_CUE = 0,       /* cue points: tap = jump, hold = set */
    FUNC_MODE_SETTINGS = 1,  /* hold + spin = pitch, platter speed, volume */
    FUNC_MODE_PLAY = 2,      /* change sample/beat, start/stop the beat */
    FUNC_MODE_COUNT
} FunctionMode;

extern volatile FunctionMode current_function_mode;
extern volatile uint8_t settings_buttons_held;  /* bitmask of held buttons in SETTINGS */
void function_mode_set(FunctionMode mode);
const char *function_mode_name(FunctionMode mode);   /* "SETTINGS" */
const char *function_mode_short(FunctionMode mode);  /* "SET" */
extern volatile int active_deck;                /* 0 or 1: which deck platter controls */

void SC_Input_Start();
void process_rot(int raw, double when);
void process_cue_buttons(uint8_t button_byte);

/* Cue display states (for TFT cue bar) */
extern int cue_display_states[4];

/* Hardware state (for home screen) */
extern bool capIsTouched;
extern bool motionTouched;  /* Touch by motion On: the platter is being turned */
extern volatile unsigned long motion_touch_count; /* touches by motion so far */
extern unsigned int ADCs[4];
extern volatile unsigned int cap_raw;            /* last touch sensor reading */
extern volatile unsigned long input_activity_ms; /* CLOCK_MONOTONIC ms of the last
                                                    platter/fader/touch/button use */

#endif