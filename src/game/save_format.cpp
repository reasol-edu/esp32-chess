#include "save_format.h"

static const uint8_t MAGIC = 0xC5, VERSION = 1;

uint16_t save_crc16(const uint8_t *p, size_t n) {  // CRC-16/CCITT-FALSE
    uint16_t c = 0xFFFF;
    for (size_t i = 0; i < n; i++) {
        c ^= (uint16_t)p[i] << 8;
        for (int k = 0; k < 8; k++) c = (c & 0x8000) ? (c << 1) ^ 0x1021 : (c << 1);
    }
    return c;
}

bool SaveWriter::begin(const SaveHeader &h) {
    if (cap_ < 10) return ok_ = false;
    b_[0] = MAGIC;
    b_[1] = VERSION;
    b_[2] = (h.humanWhite ? 1 : 0) | (h.coach ? 2 : 0) | (h.adaptive ? 4 : 0);
    b_[3] = h.level;
    b_[4] = (uint8_t)h.adj;
    b_[5] = 0;
    b_[6] = b_[7] = 0;
    len_ = 8;
    return true;
}

bool SaveWriter::add(const ce::Move &m) {
    if (!ok_ || n_ >= SAVE_MAX_PLIES || len_ + 4 > cap_) return ok_ = false;
    int promo = m.promo == 'n' ? 1 : m.promo == 'b' ? 2 : m.promo == 'r' ? 3 : m.promo == 'q' ? 4 : 0;
    uint16_t v = (uint16_t)(m.from & 63) | (uint16_t)((m.to & 63) << 6) | (uint16_t)(promo << 12);
    b_[len_++] = v & 0xFF;
    b_[len_++] = v >> 8;
    n_++;
    return true;
}

size_t SaveWriter::finish() {
    if (!ok_) return 0;
    b_[6] = n_ & 0xFF;
    b_[7] = n_ >> 8;
    uint16_t c = save_crc16(b_, len_);
    b_[len_++] = c & 0xFF;
    b_[len_++] = c >> 8;
    return len_;
}

bool SaveReader::parse(const uint8_t *b, size_t len) {
    if (len < 10 || b[0] != MAGIC || b[1] != VERSION) return false;
    uint16_t n = b[6] | (b[7] << 8);
    if (n > SAVE_MAX_PLIES || len != 8 + 2 * (size_t)n + 2) return false;
    uint16_t crc = b[len - 2] | (b[len - 1] << 8);
    if (crc != save_crc16(b, len - 2)) return false;
    h_.humanWhite = b[2] & 1;
    h_.coach = b[2] & 2;
    h_.adaptive = b[2] & 4;
    h_.level = b[3];
    h_.adj = (int8_t)b[4];
    h_.plies = n;
    b_ = b;
    return true;
}

ce::Move SaveReader::move(int i) const {
    uint16_t v = b_[8 + 2 * i] | (b_[9 + 2 * i] << 8);
    int p = v >> 12;
    return ce::Move{(int8_t)(v & 63), (int8_t)((v >> 6) & 63), (char)(p == 1 ? 'n' : p == 2 ? 'b' : p == 3 ? 'r' : p == 4 ? 'q' : 0)};
}
