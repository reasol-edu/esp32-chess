// Ajedrez táctil — Guition JC1060P470 (ESP32-P4, 1024x600). Fase 1: pantalla + táctil + PPA.
#include <Arduino.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include "touch.h"

#define TFT_BL 23

Arduino_ESP32DSIPanel *dsipanel = new Arduino_ESP32DSIPanel(
    160, 160, 40,      // hsync pulse / back porch / front porch
    23, 12, 10,        // vsync pulse / back porch / front porch
    48000000);
Arduino_GFX *gfx = new Arduino_DSI_Display(
    SCREEN_WIDTH, SCREEN_HEIGHT, dsipanel, 0 /* rotation */, true /* auto_flush */,
    GFX_NOT_DEFINED, jd9165_init_operations, sizeof(jd9165_init_operations) / sizeof(lcd_init_cmd_t));

static lv_display_t *disp;
static uint8_t *fb_lvgl;

static void flush_cb(lv_display_t *d, const lv_area_t *a, uint8_t *px) {
    // DMA2D copia al back-buffer DSI y hace el swap en vsync (sin tearing). Solo válido con rotation=0.
    esp_lcd_panel_draw_bitmap(dsipanel->getPanelHandle(), a->x1, a->y1, a->x2 + 1, a->y2 + 1, px);
    lv_display_flush_ready(d);
}

static void touch_cb(lv_indev_t *, lv_indev_data_t *data) {
    int x, y;
    if (touch_read(x, y)) { data->state = LV_INDEV_STATE_PRESSED; data->point.x = x; data->point.y = y; }
    else data->state = LV_INDEV_STATE_RELEASED;
}

static uint32_t tick_cb() { return millis(); }

static lv_obj_t *info;
static void square_cb(lv_event_t *e) {
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    lv_label_set_text_fmt(info, "Casilla %c%d", 'a' + idx % 8, 8 - idx / 8);
}

static void build_test_screen() {
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x2b2118), 0);
    const int S = 70, X0 = 20, Y0 = 20;
    for (int i = 0; i < 64; i++) {
        lv_obj_t *sq = lv_obj_create(scr);
        lv_obj_remove_style_all(sq);
        lv_obj_set_size(sq, S, S);
        lv_obj_set_pos(sq, X0 + (i % 8) * S, Y0 + (i / 8) * S);
        bool light = ((i % 8) + (i / 8)) % 2 == 0;
        lv_obj_set_style_bg_color(sq, lv_color_hex(light ? 0xf0d9b5 : 0xb58863), 0);
        lv_obj_set_style_bg_opa(sq, LV_OPA_COVER, 0);
        lv_obj_add_flag(sq, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(sq, square_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }
    info = lv_label_create(scr);
    lv_obj_set_pos(info, 620, 40);
    lv_obj_set_style_text_color(info, lv_color_white(), 0);
    lv_obj_set_style_text_font(info, &lv_font_montserrat_42, 0);
    lv_label_set_text(info, "Toca una casilla");
}

void setup() {
    Serial.begin(115200);
    touch_init();
    gfx->begin();
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    lv_init();
    lv_tick_set_cb(tick_cb);
    // Búfer completo en PSRAM, alineado a la línea de caché L2 (128 B) como exige el PPA.
    size_t bytes = SCREEN_WIDTH * SCREEN_HEIGHT * 2;
    fb_lvgl = (uint8_t *)heap_caps_aligned_alloc(128, bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!fb_lvgl) { Serial.println("sin PSRAM para el búfer LVGL"); while (true) delay(1000); }
    disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_display_set_flush_cb(disp, flush_cb);
    lv_display_set_buffers(disp, fb_lvgl, nullptr, bytes, LV_DISPLAY_RENDER_MODE_FULL);
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_cb);

    build_test_screen();
    Serial.printf("LVGL %d.%d.%d PPA=%d\n", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH, (int)LV_USE_PPA);
}

void loop() {
    lv_timer_handler();
    delay(5);
}
