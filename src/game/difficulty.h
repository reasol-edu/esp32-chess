// Niveles de dificultad pensados para niños: profundidad y tiempo de búsqueda del motor más una
// probabilidad de "despiste" (jugada legal al azar). Lógica pura, probada en nativo.
#pragma once
#include <stdint.h>

struct Level {
    const char *name;     // nombre del animal
    const char *tagline;  // frase corta para la tarjeta
    int depth;            // plies máximos de la búsqueda
    uint32_t timeMs;      // tiempo máximo
    int randomPct;        // % de jugadas al azar (despistes)
};

constexpr int LEVEL_COUNT = 6;

inline const Level &level_info(int idx) {
    static const Level L[LEVEL_COUNT] = {
        {"Pollito",  "Acaba de aprender",   1,   300, 55},
        {"Tortuga",  "Despacito y sin prisa", 1, 400, 25},
        {"Conejo",   "Rápido pero se despista", 2, 500, 10},
        {"Zorro",    "Listo y astuto",      3,   800,  3},
        {"Búho",     "Piensa mucho",        5,  3000,  0},
        {"Dragón",   "¡El maestro!",       20,  8000,  0},
    };
    return L[idx < 0 ? 0 : (idx >= LEVEL_COUNT ? LEVEL_COUNT - 1 : idx)];
}
