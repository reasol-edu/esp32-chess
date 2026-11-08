#include "game_core.h"
#include <string.h>

GameCore::GameCore() : rng_(2463534242u) { memset(&snap_, 0, sizeof snap_); snap_.humanWhite = true; }

uint32_t GameCore::next() {  // xorshift32
    rng_ ^= rng_ << 13;
    rng_ ^= rng_ >> 17;
    rng_ ^= rng_ << 5;
    return rng_;
}

void GameCore::refresh() {
    const int8_t *b = ce::board();
    memcpy(snap_.board, b, 64);
    snap_.whiteToMove = ce::whiteToMove();
    snap_.status = ce::status();
    snap_.plies = ce::plies();
    snap_.hasLast = ce::lastMove(snap_.last);
    snap_.nLegal = snap_.status >= ce::ST_MATE ? 0 : ce::legalMoves(snap_.legal, 256);
    static const uint8_t init[6] = {8, 2, 2, 2, 1, 1};
    int cw[6] = {0}, cb[6] = {0};
    for (int i = 0; i < 64; i++) {
        if (b[i] > 0) cw[b[i] - 1]++;
        else if (b[i] < 0) cb[-b[i] - 1]++;
    }
    for (int t = 0; t < 6; t++) {
        snap_.missingWhite[t] = cw[t] < init[t] ? init[t] - cw[t] : 0;  // las promociones pueden dar más de las iniciales
        snap_.missingBlack[t] = cb[t] < init[t] ? init[t] - cb[t] : 0;
    }
    snap_.hasHint = false;
    snap_.seq++;
}

void GameCore::newGame(bool humanWhite, int level) {
    ce::newGame();
    snap_.humanWhite = humanWhite;
    snap_.level = level;
    refresh();
}

bool GameCore::loadFen(const char *fen, bool humanWhite, int level) {
    if (!ce::setFen(fen)) return false;
    snap_.humanWhite = humanWhite;
    snap_.level = level;
    refresh();
    return true;
}

bool GameCore::humanMove(const ce::Move &m) {
    if (!humanToMove() || !ce::play(m)) return false;
    refresh();
    return true;
}

bool GameCore::cpuMove(ce::Move *played) {
    if (!cpuToMove()) return false;
    const Level &lv = level_info(snap_.level);
    ce::Move mv;
    if ((int)(next() % 100) < lv.randomPct && snap_.nLegal > 0) {
        mv = snap_.legal[next() % snap_.nLegal];
    } else {
        int score;
        if (!ce::search({lv.depth, lv.timeMs}, mv, score)) return false;
    }
    if (!ce::play(mv)) return false;
    if (played) *played = mv;
    refresh();
    return true;
}

bool GameCore::undo() {
    if (snap_.plies == 0) return false;
    int removed = 0;
    while (removed < 2 && ce::plies() > 0) {
        ce::undoPly();
        removed++;
        if (ce::whiteToMove() == snap_.humanWhite && removed >= 1 && ce::plies() == 0) break;
    }
    refresh();
    return true;
}

bool GameCore::hint() {
    if (!humanToMove()) return false;
    ce::Move mv;
    int score;
    if (!ce::search({4, 1500}, mv, score)) return false;
    snap_.hint = mv;
    snap_.hasHint = true;
    snap_.seq++;
    return true;
}
