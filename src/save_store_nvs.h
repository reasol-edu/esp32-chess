// Almacén de partidas en NVS (Preferences, espacio "chess"): una entrada binaria por hueco.
#pragma once
#include "save_store.h"

class NvsSaveStore : public SaveStore {
public:
    bool write(int slot, const uint8_t *data, size_t len) override;
    int read(int slot, uint8_t *buf, size_t cap) override;
    bool erase(int slot) override;
};
