#include <stdio.h>
#include "lcd_menu.h"
#include "deck.h"
#include "main_menu.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "sc_input.h"
#include "cues.h"
#include "settings.h"
#include "xwax.h"

#define ROTARY_DEBOUNCE_DELAY 5

/* Globals */
static struct deck **decks;
static int deck_count;
bool needsUpdate = true;
MainMenuState mainMenuState = MENU_HOME;


/* Auto-return and home screen refresh timers */
static unsigned long lastActivityTime = 0;
static unsigned long lastHomeRedrawTime = 0;
#define HOME_REDRAW_INTERVAL_MS  22
#define AUTO_RETURN_TIMEOUT_MS  10000

static unsigned long menu_millis(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

/* ── Screen saver ──────────────────────────────────────────────── */
/* After "Screen off after" minutes without the knob, KB0, platter,
 * fader, touch or cue buttons being used, the panel is put to sleep.
 * Playing the instrument wakes it; so does the knob or KB0, whose
 * first event is then swallowed so it does not also act on the menu.
 *
 * Everything here runs on the menu thread, the only one that talks to
 * the display. The input thread just stamps input_activity_ms. */

static unsigned long screen_blanked_at = 0;

static void screen_wake(unsigned long now) {
    oled_set_blank(false);
    lastActivityTime = now;
    needsUpdate = true;
}

/* Called on every poll of the knob. Returns true while the screen is off */
static bool screen_saver_poll(void) {
    unsigned long now = menu_millis();
    unsigned long input = input_activity_ms;
    unsigned long last = lastActivityTime;

    /* Latest of the menu and the playing controls (wrap-safe) */
    if ((long)(input - last) > 0)
        last = input;

    if (oled_is_blank()) {
        if ((long)(input - screen_blanked_at) > 0)
            screen_wake(now);
        return oled_is_blank();
    }

    if (cfg_screen_off < 1.0f)
        return false;
    /* Keep the screen on while recording, to see that it is */
    if (deck[0].player.recording || deck[1].player.recording)
        return false;

    if (now - last >= (unsigned long)(cfg_screen_off * 60000.0f)) {
        screen_blanked_at = now;
        oled_set_blank(true);
        return true;
    }
    return false;
}

/* A knob or KB0 event arrived: wake the screen if it is off.
 * Returns true if the event was used up waking it */
static bool screen_wake_by_menu(void) {
    if (!oled_is_blank())
        return false;
    screen_wake(menu_millis());
    return true;
}

void lcd_menu_init(struct deck *deck_array[], int count) {
    if (gpio_direct_init() < 0) {
        fprintf(stderr, "Couldn't initialize GPIO\n");
        exit(1);
    }

    /* Initialize display */
    if (oled_init() < 0) {
        fprintf(stderr, "Error initializing display\n");
        exit(1);
    }

    /* Setup encoder pins */
    gpio_set_input(ROTARY_CLK);
    gpio_set_input(ROTARY_DT);
    gpio_set_input(ROTARY_SW);
    gpio_set_input(ROTARY_KO);
    gpio_set_pullup(ROTARY_CLK);
    gpio_set_pullup(ROTARY_DT);
    gpio_set_pullup(ROTARY_SW);
    gpio_set_pullup(ROTARY_KO);

    printf("TFT menu initialized.\n");

    decks = deck_array;
    deck_count = count;

    lastActivityTime = menu_millis();
    display_main_menu(decks, deck_count);
}

/* Poll encoder and update display if needed */
void poll_rotary_encoder() {
    unsigned long now = menu_millis();

    /* Update cue bar overlay with active deck cue data */
    if (deck_count >= 2) {
        int ad = active_deck;
        double cue_positions[4];
        int states[4];
        for (int i = 0; i < 4; i++) {
            /* Set or empty comes from the deck itself, so the bar follows
             * track loads and deck switches; ACTIVE is the jump flash */
            cue_positions[i] = cues_get(&decks[ad]->cues, i);
            if (cue_positions[i] == CUE_UNSET)
                states[i] = CUE_STATE_EMPTY;
            else if (cue_display_states[i] == CUE_STATE_ACTIVE)
                states[i] = CUE_STATE_ACTIVE;
            else
                states[i] = CUE_STATE_SET;
        }
        oled_set_cue_overlay(states, cue_positions);
    }

    handle_main_menu_navigation(decks, deck_count);

    /* No auto-return to home: KB0 or holding the knob goes back,
     * and auto-return caused problems during recording and other
     * functional screens (volume adjust, etc.) */

    /* Periodic redraw for home screen (live data at ~45fps); not
     * while the screen is off, there is nothing to see */
    if (mainMenuState == MENU_HOME && !oled_is_blank() &&
        (now - lastHomeRedrawTime) >= HOME_REDRAW_INTERVAL_MS) {
        lastHomeRedrawTime = now;
        needsUpdate = true;
    }

    if (needsUpdate) {
        display_main_menu(decks, deck_count);
        /* display_deck_menu manages needsUpdate for deck submenus.
         * For HOME/MAIN/other top-level screens, clear it here. */
        if (mainMenuState == MENU_HOME || mainMenuState == MENU_MAIN ||
            mainMenuState == MENU_CONTROLLER || mainMenuState == MENU_INFO)
            needsUpdate = false;
    }
}

/* Detects rotary encoder movement, returns 1 for CW, -1 for CCW, 0 for no movement.
 * Uses a full quadrature state table to track all 4 transitions per detent.
 * Accumulates sub-steps and only reports a move when a full detent (4 edges)
 * is completed, providing reliable debouncing. */
int rotary_encoder_moved() {
    static const int8_t transition_table[4][4] = {
        /*  00   01   10   11  <- current */
        {   0,  -1,   1,   0 }, /* last = 00 */
        {   1,   0,   0,  -1 }, /* last = 01 */
        {  -1,   0,   0,   1 }, /* last = 10 */
        {   0,   1,  -1,   0 }, /* last = 11 */
    };

    static int lastState = -1;
    static int accumulator = 0;

    screen_saver_poll();

    int clk = gpio_read(ROTARY_CLK);
    int dt = gpio_read(ROTARY_DT);
    int currentState = (clk << 1) | dt;

    if (lastState == -1) {
        lastState = currentState;
        return 0;
    }

    if (currentState == lastState) return 0;

    int direction = transition_table[lastState][currentState];
    lastState = currentState;

    if (direction == 0) return 0;

    accumulator += direction;

    if (accumulator >= 4) {
        accumulator = 0;
        lastActivityTime = menu_millis();
        return screen_wake_by_menu() ? 0 : 1;  /* CW */
    } else if (accumulator <= -4) {
        accumulator = 0;
        lastActivityTime = menu_millis();
        return screen_wake_by_menu() ? 0 : -1; /* CCW */
    }

    return 0;
}

/* ── Menu buttons ──────────────────────────────────────────────
 * Knob push (ROTARY_SW): click = enter/select, hold = back.
 * KB0 (GPIO17): press = back.
 *
 * nav_poll() samples both pins and leaves at most one pending event of
 * each kind; menu_select_pressed() and menu_back_pressed() take them.
 * Debouncing is by time, so a screen polling both in one pass is fine.
 * An event a screen does not ask for is dropped after NAV_EVENT_TTL_MS
 * so it cannot leak into the next screen. */
#define NAV_DEBOUNCE_MS   25
#define NAV_HOLD_MS       500
#define NAV_EVENT_TTL_MS  60

typedef struct {
    bool raw;               /* last raw reading */
    unsigned long raw_at;   /* when the raw reading last changed */
    bool down;              /* debounced state */
    unsigned long down_at;  /* when the debounced press began */
    bool held;              /* the hold was already reported */
} NavButton;

static NavButton nav_push, nav_kb0;
static unsigned long nav_select_at = 0, nav_back_at = 0;  /* 0 = none */
static bool nav_back_hold = false;

/* +1 on a debounced press, -1 on a release, 0 otherwise */
static int nav_debounce(NavButton *b, bool raw, unsigned long now) {
    if (raw != b->raw) {
        b->raw = raw;
        b->raw_at = now;
        return 0;
    }
    if (raw == b->down || now - b->raw_at < NAV_DEBOUNCE_MS)
        return 0;
    b->down = raw;
    if (raw) {
        b->down_at = now;
        b->held = false;
        return 1;
    }
    return -1;
}

static void nav_poll(void) {
    unsigned long now = gpio_millis();
    if (now == 0) now = 1;   /* 0 means "no event" */

    int e = nav_debounce(&nav_push, gpio_read(ROTARY_SW) == 0, now);
    if (nav_push.down && !nav_push.held &&
        now - nav_push.down_at >= NAV_HOLD_MS) {
        nav_push.held = true;          /* hold: back, while still held */
        nav_back_at = now;
        nav_back_hold = true;
    } else if (e < 0 && !nav_push.held) {
        nav_select_at = now;           /* click: select */
    }

    if (nav_debounce(&nav_kb0, gpio_read(ROTARY_KO) == 0, now) > 0) {
        nav_back_at = now;
        nav_back_hold = false;
    }
}

static int nav_take(unsigned long *at) {
    nav_poll();
    if (*at == 0)
        return 0;
    unsigned long age = gpio_millis() - *at;
    *at = 0;
    if (age > NAV_EVENT_TTL_MS)
        return 0;
    lastActivityTime = menu_millis();
    return screen_wake_by_menu() ? 0 : 1;
}

int menu_select_pressed(void) {
    return nav_take(&nav_select_at);
}

int menu_back_pressed(void) {
    return nav_take(&nav_back_at);
}

bool menu_back_was_hold(void) {
    return nav_back_hold;
}

void trigger_io_event(unsigned char action, unsigned char deckNo, unsigned char param) {
    struct mapping temp_map = {0};
    temp_map.Action = action;
    temp_map.DeckNo = deckNo;
    temp_map.Type = MAP_IO;
    temp_map.Param = param;
    IOevent(&temp_map, NULL);
}

void trigger_io_event_no_param(unsigned char action, unsigned char deckNo) {
    unsigned char defaultVolume = 64;
    trigger_io_event(action, deckNo, defaultVolume);
}

void* rotary_encoder_thread(void* arg) {
    while (true) {
        poll_rotary_encoder();
        usleep(5000); /* Poll every 5ms (200Hz) */
    }
    return NULL;
}
