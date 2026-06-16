/*
 * Custom OLED status screen for the Corne - STEP 3 (screensaver behavior).
 *
 * Working rain from step 2, now with activity-based switching:
 *   - ACTIVE (typing): show compact status (battery; layer on the central half).
 *   - IDLE (after CONFIG_ZMK_IDLE_TIMEOUT, 30s): show the Matrix rain.
 *   - Deep sleep (30 min) powers the OLEDs off entirely (handled by ZMK).
 *
 * Still NO display rotation (that bricked the board). The OLEDs are portrait,
 * so the 128px framebuffer axis is vertical on screen; rain falls along it and
 * text shares that orientation. Panel colors are inverted: paint white -> dark,
 * black -> lit blue.
 */

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>

#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/activity.h>
#include <zmk/battery.h>
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/keymap.h>
#endif

#if IS_ENABLED(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM)

#define CELL 8
#define LANES 4
#define DEPTH 16
#define DROPS_PER_LANE 2

static const char charset[] =
    "ABCDEFGHJKLMNPQRSTUVWXYZ0123456789@#$%&*+=<>/";
#define CHARSET_LEN (sizeof(charset) - 1)

struct drop {
    int head;
    int len;
    int speed;
    int tick;
};

struct lane {
    struct drop drops[DROPS_PER_LANE];
    char cells[DEPTH];
};

static struct lane lanes[LANES];
static lv_obj_t *lane_labels[LANES];
static char label_buf[LANES][DEPTH + 1];

static lv_obj_t *batt_label;
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
static lv_obj_t *layer_label;
#endif

static bool showing_rain = true; /* start in rain; first tick corrects it */

static inline uint32_t rnd(uint32_t max) {
    return sys_rand32_get() % max;
}

static void reset_drop(struct drop *d) {
    d->head = -(int)rnd(DEPTH);
    d->len = 5 + rnd(DEPTH - 5);
    d->speed = 1 + rnd(3);
    d->tick = 0;
}

static void init_lane(struct lane *l) {
    memset(l->cells, ' ', sizeof(l->cells));
    for (int d = 0; d < DROPS_PER_LANE; d++) {
        reset_drop(&l->drops[d]);
        l->drops[d].head -= d * (DEPTH / 2);
    }
}

static void advance_drop(struct lane *l, struct drop *d) {
    if (++d->tick < d->speed) {
        return;
    }
    d->tick = 0;
    d->head++;
    if (d->head >= 0 && d->head < DEPTH) {
        l->cells[d->head] = charset[rnd(CHARSET_LEN)];
    }
    if (d->head - d->len > DEPTH) {
        reset_drop(d);
    }
}

static bool cell_lit(const struct lane *l, int r) {
    for (int d = 0; d < DROPS_PER_LANE; d++) {
        const struct drop *dr = &l->drops[d];
        if (r <= dr->head && r > dr->head - dr->len) {
            return true;
        }
    }
    return false;
}

static void render_lane(int i) {
    struct lane *l = &lanes[i];
    for (int r = 0; r < DEPTH; r++) {
        char ch = ' ';
        if (cell_lit(l, r)) {
            ch = l->cells[r] != ' ' ? l->cells[r] : charset[rnd(CHARSET_LEN)];
        }
        label_buf[i][DEPTH - 1 - r] = ch; /* bottom-up so it falls DOWN */
    }
    label_buf[i][DEPTH] = '\0';
    lv_label_set_text(lane_labels[i], label_buf[i]);
}

static void set_mode(bool rain) {
    if (rain == showing_rain) {
        return;
    }
    showing_rain = rain;
    for (int i = 0; i < LANES; i++) {
        if (rain) {
            lv_obj_clear_flag(lane_labels[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(lane_labels[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (rain) {
        lv_obj_add_flag(batt_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(batt_label, LV_OBJ_FLAG_HIDDEN);
    }
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    if (rain) {
        lv_obj_add_flag(layer_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(layer_label, LV_OBJ_FLAG_HIDDEN);
    }
#endif
}

static void update_status(void) {
    lv_label_set_text_fmt(batt_label, "%d%%", zmk_battery_state_of_charge());
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    lv_label_set_text_fmt(layer_label, "L%d", zmk_keymap_highest_layer_active());
#endif
}

static void tick_cb(lv_timer_t *timer) {
    ARG_UNUSED(timer);

    if (zmk_activity_get_state() == ZMK_ACTIVITY_ACTIVE) {
        set_mode(false);
        update_status();
        return;
    }

    set_mode(true);
    for (int i = 0; i < LANES; i++) {
        for (int d = 0; d < DROPS_PER_LANE; d++) {
            advance_drop(&lanes[i], &lanes[i].drops[d]);
        }
        render_lane(i);
    }
}

static lv_obj_t *make_label(lv_obj_t *parent, int y) {
    lv_obj_t *lbl = lv_label_create(parent);
    lv_obj_remove_style_all(lbl);
    lv_obj_set_style_text_font(lbl, &lv_font_unscii_8, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_pos(lbl, 0, y);
    return lbl;
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    for (int i = 0; i < LANES; i++) {
        init_lane(&lanes[i]);
        lane_labels[i] = make_label(screen, i * CELL);
        render_lane(i);
    }

    batt_label = make_label(screen, 0);
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    layer_label = make_label(screen, 16);
#endif

    /* Start showing status; the first tick will flip to rain if already idle. */
    showing_rain = true;
    set_mode(false);
    update_status();

    lv_timer_create(tick_cb, 60, NULL);
    return screen;
}

#endif /* CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM */
