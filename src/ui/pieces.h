// Sprites de las piezas (cburnett, 64x64 ARGB8888) y variantes derivadas con el PPA.
#pragma once
#include <lvgl.h>

#define PIECE_PX 64

#ifdef __cplusplus
extern "C" {
#endif
// Índices 0..5 = P N B R Q K blancas; 6..11 = P N B R Q K negras.
extern const lv_image_dsc_t piece_sprites[12];
#ifdef __cplusplus
}
#endif

// Pieza con signo del motor (+blancas / −negras, 1=P … 6=K) → índice de sprite.
inline int piece_index(int8_t p) { return p > 0 ? p - 1 : 6 + (-p - 1); }
