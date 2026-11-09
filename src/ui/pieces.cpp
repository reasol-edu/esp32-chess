#include "pieces.h"
#include "ppa_fx.h"
#include <string.h>

// Respaldo por software: se construye la primera vez que se pide (16 KB por pieza, en el montón de LVGL).
static lv_image_dsc_t sw_rot[12];
static bool sw_ready[12];

const lv_image_dsc_t *piece_rot180(int idx) {
    if (idx < 0 || idx > 11) return &piece_sprites[0];
    if (const lv_image_dsc_t *d = ppa_fx_rot180(idx)) return d;
    if (!sw_ready[idx]) {
        const lv_image_dsc_t &src = piece_sprites[idx];
        int n = PIECE_PX * PIECE_PX;
        uint32_t *dst = (uint32_t *)lv_malloc(n * 4);
        if (!dst) return &src;                       // sin memoria: sin girar, pero sigue funcionando
        const uint32_t *s = (const uint32_t *)src.data;
        for (int i = 0; i < n; i++) dst[i] = s[n - 1 - i];
        sw_rot[idx] = src;
        sw_rot[idx].data = (const uint8_t *)dst;
        sw_ready[idx] = true;
    }
    return &sw_rot[idx];
}
