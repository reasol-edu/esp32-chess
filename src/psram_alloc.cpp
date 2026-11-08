// Montón de LVGL en PSRAM (LV_USE_STDLIB_MALLOC = LV_STDLIB_CUSTOM). Igual que deye-monitor.
#include <Arduino.h>
#include <esp_heap_caps.h>

extern "C" {
void lv_mem_init(void) {}
void lv_mem_deinit(void) {}
void *lv_malloc_core(size_t size) { return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT); }
void lv_free_core(void *p) { heap_caps_free(p); }
void *lv_realloc_core(void *p, size_t n) { return heap_caps_realloc(p, n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT); }
}
