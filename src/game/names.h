// Nombres de jugador: texto corto en UTF-8, saneado antes de guardarlo o mostrarlo.
#pragma once
#include <stddef.h>
#include <string.h>

constexpr int NAME_LEN = 24;       // bytes con el NUL; 10 caracteres de 2 bytes (ñ, á…) caben de sobra
constexpr int NAME_MAX_CHARS = 10;

// Copia `in` a `out`: sin espacios a los lados, sin caracteres de control, como mucho NAME_MAX_CHARS
// caracteres y sin cortar nunca un carácter UTF-8 por la mitad.
inline void name_sanitize(char out[NAME_LEN], const char *in) {
    size_t n = 0;
    int chars = 0;
    out[0] = 0;
    if (!in) return;
    while (*in == ' ') in++;
    while (*in && chars < NAME_MAX_CHARS) {
        unsigned char c = (unsigned char)*in;
        size_t len = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 0;
        if (len == 0) { in++; continue; }                      // byte suelto inválido
        bool ok = true;
        for (size_t k = 1; k < len; k++) if (((unsigned char)in[k] & 0xC0) != 0x80) ok = false;  // truncado
        if (!ok) break;
        if (len == 1 && c < 0x20) { in++; continue; }          // control
        if (n + len >= (size_t)NAME_LEN) break;
        memcpy(out + n, in, len);
        n += len;
        in += len;
        chars++;
    }
    while (n > 0 && out[n - 1] == ' ') n--;
    out[n] = 0;
}
