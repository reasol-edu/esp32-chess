// Almacén de partidas guardadas por huecos. En el dispositivo usa NVS (src/save_store_nvs.*);
// MemSaveStore sirve para los tests y el simulador.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <vector>

constexpr int SAVE_SLOTS = 3;
constexpr int PREFS_SLOT = SAVE_SLOTS;  // el almacén tiene un hueco extra para las preferencias

struct SaveStore {
    virtual ~SaveStore() {}
    virtual bool write(int slot, const uint8_t *data, size_t len) = 0;
    virtual int read(int slot, uint8_t *buf, size_t cap) = 0;  // bytes leídos, o −1 si el hueco está vacío o no cabe
    virtual bool erase(int slot) = 0;
};

struct MemSaveStore : SaveStore {
    std::vector<uint8_t> d[SAVE_SLOTS + 1];
    bool failWrites = false;  // para probar errores
    bool write(int s, const uint8_t *p, size_t n) override { if (failWrites || s < 0 || s > PREFS_SLOT) return false; d[s].assign(p, p + n); return true; }
    int read(int s, uint8_t *b, size_t cap) override { if (s < 0 || s > PREFS_SLOT || d[s].empty() || d[s].size() > cap) return -1; memcpy(b, d[s].data(), d[s].size()); return (int)d[s].size(); }
    bool erase(int s) override { if (s < 0 || s > PREFS_SLOT) return false; d[s].clear(); return true; }
};
