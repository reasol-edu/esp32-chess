// API pública del motor (ver engine_core.cpp). NO es reentrante: usar siempre desde una sola tarea.
// Casillas 0..63 con a8=0, b8=1 … h8=7, a7=8 … h1=63. Piezas: +blancas −negras, 1=P 2=N 3=B 4=R 5=Q 6=K.
#pragma once
#include <stdint.h>

namespace ce {

struct Move {
    int8_t from, to;
    char promo;  // 0 o 'n','b','r','q'
    bool operator==(const Move &o) const { return from == o.from && to == o.to && promo == o.promo; }
};

enum Status : uint8_t {
    ST_PLAY, ST_CHECK, ST_MATE, ST_STALEMATE,
    ST_DRAW_MATERIAL, ST_DRAW_REPETITION, ST_DRAW_50
};

struct Limits {
    int maxDepth;     // plies de la profundización iterativa (1..20)
    uint32_t timeMs;  // tiempo máximo de búsqueda
};

void newGame();
bool setFen(const char *fen);                 // posición inicial arbitraria (borra el historial)
const int8_t *board();                        // 64 casillas
bool whiteToMove();
int legalMoves(Move *out, int maxOut);        // jugadas legales del bando que mueve
bool play(const Move &m);                     // false si no es legal
bool undoPly();                               // deshace una jugada (medio movimiento)
int plies();                                  // jugadas hechas desde el inicio
bool historyMove(int i, Move &m);              // i-ésima jugada hecha desde el inicio (0..plies()-1)
bool lastMove(Move &m);                       // última jugada hecha (false si ninguna)
Status status();
bool search(const Limits &lim, Move &best, int &scoreCp);  // no juega la jugada; false si no hay jugadas
void stop();                                  // aborta la búsqueda en curso (desde otra tarea)
void toFen(char *out, int outLen);

}  // namespace ce
