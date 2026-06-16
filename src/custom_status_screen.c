/*
 * Custom OLED status screen for the Corne - STEP 2 (look tuning).
 *
 * Step 1 proved the custom-screen path boots and types with NO display rotation.
 * This step fixes how it looks, based on what the panel actually shows:
 *   - Colors are INVERTED on this panel (lv_color_black lights the pixel), so we
 *     paint the background white (-> dark) and the characters black (-> lit blue).
 *   - Rain now falls DOWN the long axis (we render each lane bottom-up to flip it).
 *   - Denser: two independent streams per column, with longer trails.
 *
 * Still NO rotation, NO keymap query, NO idle switching - those come next once
 * the look is dialed in. The OLEDs are mounted portrait, so the 128px (long)
 * framebuffer axis is vertical on screen and the rain falls along it naturally.
 */

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>

#include <lvgl.h>

#include <zmk/display.h>

#if IS_ENABLED(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM)

#define CELL 8
#define LANES 4           /* 32px / 8px - columns across the narrow (width) axis */
#define DEPTH 16          /* 128px / 8px - cells along the long (vertical) axis */
#define DROPS_PER_LANE 2  /* independent streams per column for density */

static const char charset[] =
    "ABCDEFGHJKLMNPQRSTUVWXYZ0123456789@#$%&*+=<>/";
#define CHARSET_LEN (sizeof(charset) - 1)

struct drop {
    int head;  /* leading cell; starts above the top (negative) */
    int len;   /* lit characters trailing the head */
    int speed; /* timer ticks between advances (bigger = slower) */
    int tick;
};

struct lane {
    struct drop drops[DROPS_PER_LANE];
    char cells[DEPTH]; /* persistent glyph per cell, refreshed as heads pass */
};

static struct lane lanes[LANES];
static lv_obj_t *lane_labels[LANES];
static char label_buf[LANES][DEPTH + 1];

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
        /* stagger the second stream so they don't overlap perfectly */
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
        /* render bottom-up so the rain falls DOWN on the portrait screen */
        label_buf[i][DEPTH - 1 - r] = ch;
    }
    label_buf[i][DEPTH] = '\0';
    lv_label_set_text(lane_labels[i], label_buf[i]);
}

static void tick_cb(lv_timer_t *timer) {
    ARG_UNUSED(timer);
    for (int i = 0; i < LANES; i++) {
        for (int d = 0; d < DROPS_PER_LANE; d++) {
            advance_drop(&lanes[i], &lanes[i].drops[d]);
        }
        render_lane(i);
    }
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    /* Panel maps colors inverted: white -> dark, black -> lit blue. */
    lv_obj_set_style_bg_color(screen, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    for (int i = 0; i < LANES; i++) {
        init_lane(&lanes[i]);
        lane_labels[i] = lv_label_create(screen);
        lv_obj_remove_style_all(lane_labels[i]);
        lv_obj_set_style_text_font(lane_labels[i], &lv_font_unscii_8, LV_PART_MAIN);
        lv_obj_set_style_text_color(lane_labels[i], lv_color_black(), LV_PART_MAIN);
        lv_obj_set_pos(lane_labels[i], 0, i * CELL);
        render_lane(i);
    }

    lv_timer_create(tick_cb, 60, NULL);
    return screen;
}

#endif /* CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM */
