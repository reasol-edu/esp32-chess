// Ajedrez táctil — Guition JC1060P470 (ESP32-P4, 1024x600): LVGL 9.6 + PPA + motor esp32-chess-engine.
#include <Arduino.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include "touch.h"
#include "ui_game.h"
#include "ppa_fx.h"
#include "game_service.h"

#define TFT_BL 23

// Dos versiones de la placa: el panel del lote 2026 ("V2" en la trasera, chip ESP32-P4 rev v3.x) usa otros tiempos,
// otra secuencia de inicio y reset en GPIO0. Para el panel antiguo, compilar con -DPANEL_JC1060P470_V1.
#ifdef PANEL_JC1060P470_V1
#define PANEL_TIMING 160, 160, 40, 23, 12, 10, 48000000   // hsync pulse/back/front, vsync pulse/back/front, pclk
#define PANEL_LANE_MBPS 750
#define PANEL_RST GFX_NOT_DEFINED
#define PANEL_INIT jd9165_init_operations, sizeof(jd9165_init_operations) / sizeof(lcd_init_cmd_t)
#else
#include "jd9165_v2.h"
#define PANEL_TIMING 24, 136, 160, 2, 21, 12, 52000000
#define PANEL_LANE_MBPS 750
#define PANEL_RST 0
#define PANEL_INIT jd9165_v2_init_operations, sizeof(jd9165_v2_init_operations) / sizeof(lcd_init_cmd_t)
#endif

Arduino_ESP32DSIPanel *dsipanel = new Arduino_ESP32DSIPanel(PANEL_TIMING, PANEL_LANE_MBPS);
Arduino_GFX *gfx = new Arduino_DSI_Display(
    SCREEN_WIDTH, SCREEN_HEIGHT, dsipanel, 0 /* rotation */, true /* auto_flush */, PANEL_RST, PANEL_INIT);

static GameService service;
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
    lv_display_t *disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_display_set_flush_cb(disp, flush_cb);
    lv_display_set_buffers(disp, fb_lvgl, nullptr, bytes, LV_DISPLAY_RENDER_MODE_FULL);
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_cb);

    ppa_fx_init();
    service.begin();
    ui_game_create(lv_screen_active(), &service);
    Serial.printf("LVGL %d.%d.%d PPA=%d sprites_ppa=%d\n", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH, (int)LV_USE_PPA, (int)ppa_fx_active());
}

void loop() {
    lv_timer_handler();
    delay(5);
}
