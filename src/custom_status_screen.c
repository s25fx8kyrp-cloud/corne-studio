/*
 * Custom OLED status screen for the Corne - STEP 1 (minimal, crash-isolation).
 *
 * Goal of this version: prove the custom-screen + module path boots and types
 * on real hardware WITHOUT any of the things that bricked the first attempt:
 *   - NO display rotation (lv_disp_set_rotation was the prime crash suspect)
 *   - NO keymap layer query (broke the peripheral build/runtime)
 *   - NO activity/idle switching yet
 *
 * It just draws a continuously-falling "digital rain" on both halves. The panel
 * is a 1-bit SSD1306 (128x32, landscape), so pixels are on/off only - the rain
 * is crisp solid characters. Rain flows along the long (128px) axis as 4 lanes,
 * each up to 16 characters deep, giving long streamers.
 *
 * Once this is confirmed working, later steps add: status-while-typing, idle
 * switching, and (carefully) portrait orientation.
 */

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>

#include <lvgl.h>

#include <zmk/display.h>

#if IS_ENABLED(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM)

#define CELL 8
#define LANES 4   /* 32px / 8px - one text row per lane */
#define DEPTH 16  /* 128px / 8px - characters along the long axis */

static const char charset[] =
    "ABCDEFGHJKLMNPQRSTUVWXYZ0123456789@#$%&*+=<>/";
#define CHARSET_LEN (sizeof(charset) - 1)

struct lane {
    int head;  /* leading position along the lane; may start off-screen */
    int len;   /* lit characters trailing the head */
    int speed; /* timer ticks between advances (bigger = slower) */
    int tick;  /* tick accumulator */
    char cells[DEPTH];
};

static struct lane lanes[LANES];
static lv_obj_t *lane_labels[LANES];
static char label_buf[LANES][DEPTH + 1];

static inline uint32_t rnd(uint32_t max) {
    return sys_rand32_get() % max;
}

static void reset_lane(struct lane *l) {
    l->head = -(int)rnd(DEPTH);
    l->len = 4 + rnd(DEPTH - 4);
    l->speed = 1 + rnd(3);
    l->tick = 0;
    memset(l->cells, ' ', sizeof(l->cells));
}

static void advance_lane(int i) {
    struct lane *l = &lanes[i];
    l->head++;

    if (l->head - l->len > DEPTH) {
        reset_lane(l);
        return;
    }
    if (l->head >= 0 && l->head < DEPTH) {
        l->cells[l->head] = charset[rnd(CHARSET_LEN)];
    }
    int tail = l->head - l->len;
    if (tail >= 0 && tail < DEPTH) {
        l->cells[tail] = ' ';
    }
    if (rnd(3) == 0) {
        int r = l->head - (int)rnd(l->len + 1);
        if (r >= 0 && r < DEPTH) {
            l->cells[r] = charset[rnd(CHARSET_LEN)];
        }
    }
}

static void render_lane(int i) {
    memcpy(label_buf[i], lanes[i].cells, DEPTH);
    label_buf[i][DEPTH] = '\0';
    lv_label_set_text(lane_labels[i], label_buf[i]);
}

static void tick_cb(lv_timer_t *timer) {
    ARG_UNUSED(timer);
    for (int i = 0; i < LANES; i++) {
        if (++lanes[i].tick >= lanes[i].speed) {
            lanes[i].tick = 0;
            advance_lane(i);
            render_lane(i);
        }
    }
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    for (int i = 0; i < LANES; i++) {
        reset_lane(&lanes[i]);
        lane_labels[i] = lv_label_create(screen);
        lv_obj_remove_style_all(lane_labels[i]);
        lv_obj_set_style_text_font(lane_labels[i], &lv_font_unscii_8, LV_PART_MAIN);
        lv_obj_set_style_text_color(lane_labels[i], lv_color_white(), LV_PART_MAIN);
        lv_obj_set_pos(lane_labels[i], 0, i * CELL);
        render_lane(i);
    }

    lv_timer_create(tick_cb, 60, NULL);
    return screen;
}

#endif /* CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM */
