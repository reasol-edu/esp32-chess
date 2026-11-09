#include "save_store_nvs.h"
#include <Preferences.h>

static const char *NS = "chess";

static void key(int slot, char *k) { snprintf(k, 8, "slot%d", slot); }

bool NvsSaveStore::write(int slot, const uint8_t *data, size_t len) {
    if (slot < 0 || slot >= SAVE_SLOTS) return false;
    Preferences p;
    if (!p.begin(NS, false)) return false;
    char k[8]; key(slot, k);
    bool ok = p.putBytes(k, data, len) == len;   // NVS escribe la nueva versión antes de liberar la anterior
    p.end();
    return ok;
}

int NvsSaveStore::read(int slot, uint8_t *buf, size_t cap) {
    if (slot < 0 || slot >= SAVE_SLOTS) return -1;
    Preferences p;
    if (!p.begin(NS, true)) return -1;           // solo lectura: si no existe el espacio, hueco vacío
    char k[8]; key(slot, k);
    size_t n = p.getBytesLength(k);
    int r = -1;
    if (n > 0 && n <= cap && p.getBytes(k, buf, n) == n) r = (int)n;
    p.end();
    return r;
}

bool NvsSaveStore::erase(int slot) {
    if (slot < 0 || slot >= SAVE_SLOTS) return false;
    Preferences p;
    if (!p.begin(NS, false)) return false;
    char k[8]; key(slot, k);
    bool ok = !p.isKey(k) || p.remove(k);
    p.end();
    return ok;
}
