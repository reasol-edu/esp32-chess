// Sprites derivados de las piezas generados con el PPA del ESP32-P4 (escalado SRM) al arrancar.
// En el simulador y si el PPA falla devuelven nullptr y la UI escala por software.
#pragma once
#include <lvgl.h>

void ppa_fx_init();                           // llamar tras lv_init(); mide y registra el tiempo por Serial
bool ppa_fx_active();                         // true si se generaron con el PPA
const lv_image_dsc_t *ppa_fx_lifted(int idx); // pieza "levantada" 80x80 (1,25x) o nullptr
const lv_image_dsc_t *ppa_fx_mini(int idx);   // miniatura 32x32 (0,5x) o nullptr
