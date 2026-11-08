// Escalado de sprites con el PPA (Pixel-Processing Accelerator) del ESP32-P4.
// Al arrancar se generan, con la operación SRM (scale-rotate-mirror) y color ARGB8888:
//   - 12 piezas "levantadas" de 80x80 (1,25x), para la pieza seleccionada
//   - 12 miniaturas de 32x32 (0,5x), para la bandeja de capturas
// Después se comprueba la salida (alfa en el centro y esquinas transparentes); si algo falla se
// liberan los búferes y la UI escala por software. Sin USE_PPA (entorno *_noppa) todo son stubs.
#include "ppa_fx.h"
#include "pieces.h"

#ifdef USE_PPA
#include <Arduino.h>
#include <string.h>
#include "driver/ppa.h"
#include "esp_cache.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"

static const size_t LINE = 128;  // línea de caché L2 del P4
static lv_image_dsc_t lifted_dsc[12], mini_dsc[12];
static uint8_t *bufs[25];
static int nbuf;
static bool active;

static size_t up(size_t n) { return (n + LINE - 1) & ~(LINE - 1); }

static uint8_t *alloc_buf(size_t n) {
    uint8_t *p = (uint8_t *)heap_caps_aligned_alloc(LINE, up(n), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (p) { memset(p, 0, up(n)); bufs[nbuf++] = p; }
    return p;
}

static bool scale(ppa_client_handle_t cli, const uint8_t *src, int sw, uint8_t *dst, int dw, float k) {
    ppa_srm_oper_config_t op;
    memset(&op, 0, sizeof op);
    op.in.buffer = src;
    op.in.pic_w = op.in.block_w = sw;
    op.in.pic_h = op.in.block_h = sw;
    op.in.srm_cm = PPA_SRM_COLOR_MODE_ARGB8888;
    op.out.buffer = dst;
    op.out.buffer_size = up((size_t)dw * dw * 4);
    op.out.pic_w = dw;
    op.out.pic_h = dw;
    op.out.srm_cm = PPA_SRM_COLOR_MODE_ARGB8888;
    op.rotation_angle = PPA_SRM_ROTATION_ANGLE_0;
    op.scale_x = k;
    op.scale_y = k;
    op.alpha_update_mode = PPA_ALPHA_NO_CHANGE;
    op.mode = PPA_TRANS_MODE_BLOCKING;
    return ppa_do_scale_rotate_mirror(cli, &op) == ESP_OK;
}

static void fill_dsc(lv_image_dsc_t &d, uint8_t *data, int w) {
    memset(&d, 0, sizeof d);
    d.header.magic = LV_IMAGE_HEADER_MAGIC;
    d.header.cf = LV_COLOR_FORMAT_ARGB8888;
    d.header.w = w;
    d.header.h = w;
    d.header.stride = w * 4;
    d.data_size = w * w * 4;
    d.data = data;
}

static bool sane(const uint8_t *px, int w) {  // pieza: esquina transparente y algo opaco
    esp_cache_msync((void *)px, up((size_t)w * w * 4), ESP_CACHE_MSYNC_FLAG_DIR_M2C | ESP_CACHE_MSYNC_FLAG_INVALIDATE);
    int opaque = 0;
    for (int i = 0; i < w * w; i++) opaque += px[i * 4 + 3] > 200;
    return px[3] == 0 && opaque > w * w / 10;
}

void ppa_fx_init() {
    int64_t t0 = esp_timer_get_time();
    ppa_client_handle_t cli = nullptr;
    ppa_client_config_t cc;
    memset(&cc, 0, sizeof cc);
    cc.oper_type = PPA_OPERATION_SRM;
    cc.max_pending_trans_num = 1;
    if (ppa_register_client(&cc, &cli) != ESP_OK) { Serial.println("[PPA] no se pudo registrar el cliente: escalado por software"); return; }
    uint8_t *src = alloc_buf(PIECE_PX * PIECE_PX * 4);
    bool ok = src != nullptr;
    for (int i = 0; ok && i < 12; i++) {
        memcpy(src, piece_sprites[i].data, PIECE_PX * PIECE_PX * 4);  // el PPA no lee de flash: copia a PSRAM
        esp_cache_msync(src, up(PIECE_PX * PIECE_PX * 4), ESP_CACHE_MSYNC_FLAG_DIR_C2M);
        uint8_t *big = alloc_buf(80 * 80 * 4), *small = alloc_buf(32 * 32 * 4);
        ok = big && small && scale(cli, src, PIECE_PX, big, 80, 1.25f) && scale(cli, src, PIECE_PX, small, 32, 0.5f) && sane(big, 80) && sane(small, 32);
        if (ok) { fill_dsc(lifted_dsc[i], big, 80); fill_dsc(mini_dsc[i], small, 32); }
    }
    ppa_unregister_client(cli);
    if (ok) {
        active = true;
        Serial.printf("[PPA] 24 sprites generados con SRM en %lld us\n", (long long)(esp_timer_get_time() - t0));
    } else {
        for (int i = 0; i < nbuf; i++) heap_caps_free(bufs[i]);
        nbuf = 0;
        Serial.println("[PPA] la comprobación de los sprites falló: escalado por software");
    }
}

bool ppa_fx_active() { return active; }
const lv_image_dsc_t *ppa_fx_lifted(int idx) { return active ? &lifted_dsc[idx] : nullptr; }
const lv_image_dsc_t *ppa_fx_mini(int idx) { return active ? &mini_dsc[idx] : nullptr; }

#else
void ppa_fx_init() {}
bool ppa_fx_active() { return false; }
const lv_image_dsc_t *ppa_fx_lifted(int) { return nullptr; }
const lv_image_dsc_t *ppa_fx_mini(int) { return nullptr; }
#endif
