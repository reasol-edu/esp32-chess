#include "save_format.h"
#include "difficulty.h"
#define LEVEL_COUNT_FOR_PREFS LEVEL_COUNT

static const uint8_t MAGIC = 0xC5, VERSION = 2;          // v2: modo 2 jugadores y nombres
static const uint8_t PMAGIC = 0xC6, PVERSION = 1;        // preferencias

uint16_t save_crc16(const uint8_t *p, size_t n) {  // CRC-16/CCITT-FALSE
    uint16_t c = 0xFFFF;
    for (size_t i = 0; i < n; i++) {
        c ^= (uint16_t)p[i] << 8;
        for (int k = 0; k < 8; k++) c = (c & 0x8000) ? (c << 1) ^ 0x1021 : (c << 1);
    }
    return c;
}

bool SaveWriter::begin(const SaveHeader &h) {
    if (cap_ < SAVE_HEADER_BYTES + 2) return ok_ = false;
    b_[0] = MAGIC;
    b_[1] = VERSION;
    b_[2] = (h.humanWhite ? 1 : 0) | (h.coach ? 2 : 0) | (h.adaptive ? 4 : 0) | (h.twoPlayer ? 8 : 0);
    b_[3] = h.level;
    b_[4] = (uint8_t)h.adj;
    b_[5] = 0;
    b_[6] = b_[7] = 0;
    memcpy(b_ + 8, h.name1, NAME_LEN);
    memcpy(b_ + 8 + NAME_LEN, h.name2, NAME_LEN);
    len_ = SAVE_HEADER_BYTES;
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
    if (len < SAVE_HEADER_BYTES + 2 || b[0] != MAGIC || b[1] != VERSION) return false;
    uint16_t n = b[6] | (b[7] << 8);
    if (n > SAVE_MAX_PLIES || len != SAVE_HEADER_BYTES + 2 * (size_t)n + 2) return false;
    uint16_t crc = b[len - 2] | (b[len - 1] << 8);
    if (crc != save_crc16(b, len - 2)) return false;
    h_.humanWhite = b[2] & 1;
    h_.coach = b[2] & 2;
    h_.adaptive = b[2] & 4;
    h_.twoPlayer = b[2] & 8;
    h_.level = b[3];
    h_.adj = (int8_t)b[4];
    h_.plies = n;
    name_sanitize(h_.name1, (const char *)b + 8);   // el CRC protege de la corrupción; se sanea por si acaso
    name_sanitize(h_.name2, (const char *)b + 8 + NAME_LEN);
    b_ = b;
    return true;
}

ce::Move SaveReader::move(int i) const {
    uint16_t v = b_[SAVE_HEADER_BYTES + 2 * i] | (b_[SAVE_HEADER_BYTES + 1 + 2 * i] << 8);
    int p = v >> 12;
    return ce::Move{(int8_t)(v & 63), (int8_t)((v >> 6) & 63), (char)(p == 1 ? 'n' : p == 2 ? 'b' : p == 3 ? 'r' : p == 4 ? 'q' : 0)};
}

size_t prefs_encode(const GamePrefs &p, uint8_t *b, size_t cap) {
    const size_t n = 4 + 2 * NAME_LEN + 2;
    if (cap < n) return 0;
    b[0] = PMAGIC;
    b[1] = PVERSION;
    b[2] = (p.humanWhite ? 1 : 0) | (p.coach ? 2 : 0) | (p.adaptive ? 4 : 0) | (p.twoPlayer ? 8 : 0);
    b[3] = p.level;
    memcpy(b + 4, p.name1, NAME_LEN);
    memcpy(b + 4 + NAME_LEN, p.name2, NAME_LEN);
    uint16_t c = save_crc16(b, n - 2);
    b[n - 2] = c & 0xFF;
    b[n - 1] = c >> 8;
    return n;
}

bool prefs_decode(const uint8_t *b, size_t len, GamePrefs &o) {
    const size_t n = 4 + 2 * NAME_LEN + 2;
    if (len != n || b[0] != PMAGIC || b[1] != PVERSION) return false;
    if ((b[n - 2] | (b[n - 1] << 8)) != save_crc16(b, n - 2)) return false;
    o.humanWhite = b[2] & 1;
    o.coach = b[2] & 2;
    o.adaptive = b[2] & 4;
    o.twoPlayer = b[2] & 8;
    o.level = b[3] < LEVEL_COUNT_FOR_PREFS ? b[3] : LEVEL_COUNT_FOR_PREFS - 1;
    name_sanitize(o.name1, (const char *)b + 4);
    name_sanitize(o.name2, (const char *)b + 4 + NAME_LEN);
    return true;
}
