/*
 * Custom OLED status screen for the Corne with a Matrix-rain screensaver.
 *
 * Behavior:
 *   - While the keyboard is ACTIVE (you're typing): show a compact status
 *     readout (layer + battery).
 *   - After ~30s of no keypresses ZMK enters the IDLE activity state; we switch
 *     to the falling-character "digital rain" animation.
 *   - At deep sleep (30 min) ZMK powers the panel off entirely.
 *
 * This replaces the stock status screen (CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM=y).
 * For the rain to be visible during idle, CONFIG_ZMK_DISPLAY_BLANK_ON_IDLE must
 * be =n, otherwise ZMK blanks the display the moment it goes idle.
 *
 * The panel is a 1-bit SSD1306: pixels are on or off, no grayscale, so the rain
 * is crisp solid characters (no fade trail). The display is rotated 90° so the
 * rain falls along the long (128px) axis, matching the keyboard's mounting.
 */

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>

#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/activity.h>
#include <zmk/battery.h>
#include <zmk/keymap.h>

#if IS_ENABLED(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM)

/* Logical (post-rotation) resolution and the 8x8 character cell grid. */
#define DISP_W 32
#define DISP_H 128
#define CELL 8
#define COLS (DISP_W / CELL) /* 4 columns of rain */
#define ROWS (DISP_H / CELL) /* 16 character rows deep */

/* No '0'/'O', 'I'/'1', 'Q' look-alikes trimmed for legibility at 8px. */
static const char charset[] =
    "ABCDEFGHJKLMNPQRSTUVWXYZ0123456789@#$%&*+=<>/";
#define CHARSET_LEN (sizeof(charset) - 1)

struct rain_col {
    int head;  /* row of the leading char; may sit above the screen (negative) */
    int len;   /* number of lit chars trailing the head */
    int speed; /* timer ticks between row advances (bigger = slower) */
    int tick;  /* tick accumulator */
    char cells[ROWS];
};

static struct rain_col cols[COLS];
static lv_obj_t *rain_labels[COLS];
static char label_buf[COLS][ROWS * 2]; /* one char + '\n' per row, + NUL */

static lv_obj_t *status_cont;
static lv_obj_t *batt_label;
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
static lv_obj_t *layer_label;
#endif
static bool showing_rain;

static inline uint32_t rnd(uint32_t max) {
    return sys_rand32_get() % max;
}

static void reset_col(struct rain_col *c) {
    c->head = -(int)rnd(ROWS); /* stagger starts so columns don't march in sync */
    c->len = 4 + rnd(ROWS - 4);
    c->speed = 1 + rnd(3);
    c->tick = 0;
    memset(c->cells, ' ', sizeof(c->cells));
}

static void advance_col(int i) {
    struct rain_col *c = &cols[i];
    c->head++;

    if (c->head - c->len > ROWS) { /* whole stream has fallen off the bottom */
        reset_col(c);
        return;
    }

    if (c->head >= 0 && c->head < ROWS) {
        c->cells[c->head] = charset[rnd(CHARSET_LEN)];
    }

    int tail = c->head - c->len; /* switch off the cell leaving the trail */
    if (tail >= 0 && tail < ROWS) {
        c->cells[tail] = ' ';
    }

    /* occasionally re-randomize a char mid-trail so the stream shimmers */
    if (rnd(3) == 0) {
        int r = c->head - (int)rnd(c->len + 1);
        if (r >= 0 && r < ROWS) {
            c->cells[r] = charset[rnd(CHARSET_LEN)];
        }
    }
}

static void render_col(int i) {
    char *p = label_buf[i];
    for (int r = 0; r < ROWS; r++) {
        *p++ = cols[i].cells[r];
        if (r < ROWS - 1) {
            *p++ = '\n';
        }
    }
    *p = '\0';
    lv_label_set_text(rain_labels[i], label_buf[i]);
}

static void set_rain_visible(bool rain) {
    if (rain == showing_rain) {
        return;
    }
    showing_rain = rain;
    if (rain) {
        lv_obj_add_flag(status_cont, LV_OBJ_FLAG_HIDDEN);
        for (int i = 0; i < COLS; i++) {
            lv_obj_clear_flag(rain_labels[i], LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        lv_obj_clear_flag(status_cont, LV_OBJ_FLAG_HIDDEN);
        for (int i = 0; i < COLS; i++) {
            lv_obj_add_flag(rain_labels[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void update_status(void) {
    /* Layer state only lives on the central half; the peripheral (right) half
     * doesn't run the keymap, so zmk_keymap_highest_layer_active() isn't even
     * compiled there. Show the layer only where it's meaningful. */
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    lv_label_set_text_fmt(layer_label, "L%d", zmk_keymap_highest_layer_active());
#endif
    lv_label_set_text_fmt(batt_label, "%d%%", zmk_battery_state_of_charge());
}

static void tick_cb(lv_timer_t *timer) {
    ARG_UNUSED(timer);

    if (zmk_activity_get_state() == ZMK_ACTIVITY_ACTIVE) {
        set_rain_visible(false);
        update_status();
        return;
    }

    set_rain_visible(true);
    for (int i = 0; i < COLS; i++) {
        if (++cols[i].tick >= cols[i].speed) {
            cols[i].tick = 0;
            advance_col(i);
            render_col(i);
        }
    }
}

lv_obj_t *zmk_display_status_screen(void) {
    /* Rotate 90° so the long axis is vertical and the rain falls "down". */
    lv_disp_set_rotation(lv_disp_get_default(), LV_DISP_ROT_90);

    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    /* --- Status view (shown while active) --- */
    status_cont = lv_obj_create(screen);
    lv_obj_remove_style_all(status_cont);
    lv_obj_set_size(status_cont, DISP_W, DISP_H);
    lv_obj_set_pos(status_cont, 0, 0);

    batt_label = lv_label_create(status_cont);
    lv_obj_set_style_text_font(batt_label, &lv_font_unscii_8, LV_PART_MAIN);
    lv_obj_set_style_text_color(batt_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(batt_label, LV_ALIGN_TOP_LEFT, 0, 2);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    layer_label = lv_label_create(status_cont);
    lv_obj_set_style_text_font(layer_label, &lv_font_unscii_8, LV_PART_MAIN);
    lv_obj_set_style_text_color(layer_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(layer_label, LV_ALIGN_TOP_LEFT, 0, 14);
#endif

    /* --- Rain view (one vertical label per column, shown while idle) --- */
    for (int i = 0; i < COLS; i++) {
        reset_col(&cols[i]);
        rain_labels[i] = lv_label_create(screen);
        lv_obj_remove_style_all(rain_labels[i]);
        lv_obj_set_style_text_font(rain_labels[i], &lv_font_unscii_8, LV_PART_MAIN);
        lv_obj_set_style_text_color(rain_labels[i], lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_text_line_space(rain_labels[i], 0, LV_PART_MAIN);
        lv_obj_set_pos(rain_labels[i], i * CELL, 0);
        lv_obj_add_flag(rain_labels[i], LV_OBJ_FLAG_HIDDEN);
        render_col(i);
    }

    showing_rain = false;
    update_status();

    /* ~16 fps animation tick. */
    lv_timer_create(tick_cb, 60, NULL);

    return screen;
}

#endif /* CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM */
