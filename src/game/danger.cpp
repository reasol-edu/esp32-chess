#include "danger.h"
#include <stdlib.h>
#include <string.h>

namespace {

const int VAL[7] = {0, 100, 320, 330, 500, 900, 0};
const int MAX_NODES = 1500;  // tope de jugadas simuladas para acotar el tiempo en el ESP32

struct Ctx {
    bool humanWhite;
    int nodes;
};

int diff(const int8_t *b, bool hw) {
    int w = 0, k = 0;
    for (int i = 0; i < 64; i++) {
        if (b[i] > 0) w += VAL[b[i]];
        else if (b[i] < 0) k += VAL[-b[i]];
    }
    return hw ? w - k : k - w;
}

bool is_capture(const int8_t *b, const ce::Move &m) {
    if (b[m.to]) return true;
    return abs(b[m.from]) == 1 && (m.from & 7) != (m.to & 7);  // al paso
}

// Valor (material desde el humano) tras capturas alternadas; quien mueve puede "pasar" (stand pat).
int qs(Ctx &c, int depth, int alpha, int beta) {
    int8_t b[64];
    memcpy(b, ce::board(), 64);
    bool humanMoves = ce::whiteToMove() == c.humanWhite;
    int best = diff(b, c.humanWhite);
    if (depth == 0 || c.nodes > MAX_NODES) return best;
    if (humanMoves) { if (best >= beta) return best; if (best > alpha) alpha = best; }
    else            { if (best <= alpha) return best; if (best < beta) beta = best; }
    ce::Move mv[256];
    int n = ce::legalMoves(mv, 256);
    for (int i = 0; i < n; i++) {
        if (!is_capture(b, mv[i])) continue;
        if (!ce::play(mv[i])) continue;
        c.nodes++;
        int v = qs(c, depth - 1, alpha, beta);
        ce::undoPly();
        if (humanMoves) { if (v > best) best = v; if (best > alpha) alpha = best; }
        else            { if (v < best) best = v; if (best < beta) beta = best; }
        if (alpha >= beta) break;
    }
    return best;
}

}  // namespace

Danger analyze_danger(bool humanWhite) {
    Danger d;
    int8_t b[64];
    memcpy(b, ce::board(), 64);
    int base = diff(b, humanWhite);
    ce::Move mv[256];
    int n = ce::legalMoves(mv, 256);
    Ctx c{humanWhite, 0};
    int worst = base;
    for (int i = 0; i < n; i++) {
        if (!is_capture(b, mv[i])) continue;
        if (!ce::play(mv[i])) continue;
        c.nodes++;
        int v = qs(c, 3, -100000, 100000);
        ce::undoPly();
        if (v < worst) { worst = v; d.square = mv[i].to; }
    }
    d.loss = base - worst;
    d.found = d.loss >= DANGER_MIN_LOSS && d.square >= 0;
    if (!d.found) d.square = -1;
    return d;
}
