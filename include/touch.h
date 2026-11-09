// Táctil GT911 de la Guition JC1060P470 (copiado de deye-monitor, solo esta placa).
// Panel nativo 1024x600, sin rotación ni intercambio de ejes.
#pragma once
#include <Wire.h>
#include "TAMC_GT911.h"

#define TOUCH_GT911_SCL 8
#define TOUCH_GT911_SDA 7
#define TOUCH_GT911_INT 21
#define TOUCH_GT911_RST 22

static TAMC_GT911 ts(TOUCH_GT911_SDA, TOUCH_GT911_SCL, TOUCH_GT911_INT, TOUCH_GT911_RST, SCREEN_WIDTH, SCREEN_HEIGHT);

inline void touch_init() {
    Wire.setBufferSize(GT911_CONFIG_SIZE);
    Wire.begin(TOUCH_GT911_SDA, TOUCH_GT911_SCL);
    ts.begin();
    ts.setRotation(ROTATION_NORMAL);
}

// Devuelve true y las coordenadas de pantalla (con el mismo mapeo invertido que deye-monitor).
inline bool touch_read(int &x, int &y) {
    ts.read();
    if (!ts.isTouched) return false;
    x = map(ts.points[0].x, SCREEN_WIDTH, 0, 0, SCREEN_WIDTH - 1);
    y = map(ts.points[0].y, SCREEN_HEIGHT, 0, 0, SCREEN_HEIGHT - 1);
    return true;
}

// Lectura en una tarea propia: LVGL consulta el táctil solo cada ~33 ms y, si un repintado tarda más, un toque corto cae
// entre dos lecturas y se pierde. La tarea muestrea cada 8 ms y retiene cada pulsación hasta que LVGL la ha visto.
static volatile bool t_down = false, t_sticky = false;
static volatile int t_x = 0, t_y = 0;

static void touch_task(void *) {
    for (;;) {
        int x, y;
        bool d = touch_read(x, y);
        if (d) {
            t_x = x; t_y = y;
            if (!t_down) t_sticky = true;
        }
        t_down = d;
        vTaskDelay(pdMS_TO_TICKS(8));
    }
}

inline void touch_start_task() { xTaskCreatePinnedToCore(touch_task, "touch", 4096, nullptr, 2, nullptr, 0); }

// Para el callback de LVGL: devuelve true si hay (o hubo desde la última lectura) una pulsación.
inline bool touch_poll(int &x, int &y) {
    if (!t_down && !t_sticky) return false;
    x = t_x; y = t_y;
    t_sticky = false;
    return true;
}
