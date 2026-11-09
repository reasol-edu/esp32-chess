#include "game_core.h"
#include <string.h>
#include <vector>

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
    snap_.coach = coach_;
    snap_.adaptive = adaptive_;
    snap_.effLevel = adaptive_ ? ad_.effective(snap_.level) : snap_.level;
    snap_.awaiting = awaiting_;
    snap_.dangerSq = awaiting_ ? danger_.square : -1;
    snap_.dangerLoss = awaiting_ ? danger_.loss : 0;
    snap_.adaptEvent = event_;
    event_ = 0;
    for (int i = 0; i < SAVE_SLOTS; i++) snap_.slots[i] = slots_[i];
    snap_.lastOp = op_;
    op_ = 0;
    snap_.hasHint = false;
    snap_.seq++;
}

void GameCore::newGame(bool humanWhite, int level, bool coach, bool adaptive) {
    ce::newGame();
    snap_.humanWhite = humanWhite;
    snap_.level = level;
    coach_ = coach;
    adaptive_ = adaptive;
    awaiting_ = false;
    ended_ = false;
    fromFen_ = false;
    event_ = 0;
    ad_.cooldown = 0;  // el ajuste (adj) se conserva entre partidas
    refresh();
}

void GameCore::checkEnd() {
    if (ended_ || !over()) return;
    ended_ = true;
    if (snap_.status != ce::ST_MATE) return;
    bool humanMated = snap_.whiteToMove == snap_.humanWhite;
    if (adaptive_) ad_.onGameEnd(!humanMated, humanMated);
}

bool GameCore::confirm() {
    if (!awaiting_) return false;
    awaiting_ = false;
    refresh();
    return true;
}

bool GameCore::loadFen(const char *fen, bool humanWhite, int level) {
    if (!ce::setFen(fen)) return false;
    snap_.humanWhite = humanWhite;
    snap_.level = level;
    awaiting_ = false;
    ended_ = false;
    fromFen_ = true;
    event_ = 0;
    refresh();
    return true;
}

bool GameCore::humanMove(const ce::Move &m) {
    if (!humanToMove() || !ce::play(m)) return false;
    awaiting_ = false;
    refresh();
    checkEnd();
    if (coach_ && !over()) {
        danger_ = analyze_danger(snap_.humanWhite);
        if (danger_.found) { awaiting_ = true; refresh(); }
    }
    return true;
}

bool GameCore::cpuMove(ce::Move *played) {
    if (!cpuMayMove()) return false;
    const Level &lv = level_info(adaptive_ ? ad_.effective(snap_.level) : snap_.level);
    ce::Move mv;
    if ((int)(next() % 100) < lv.randomPct && snap_.nLegal > 0) {
        mv = snap_.legal[next() % snap_.nLegal];
    } else {
        int score;
        if (!ce::search({lv.depth, lv.timeMs}, mv, score)) return false;
    }
    if (!ce::play(mv)) return false;
    if (played) *played = mv;
    if (adaptive_) {  // balance de material desde el humano tras la jugada de la CPU
        int w = 0, b = 0;
        static const int V[7] = {0, 100, 320, 330, 500, 900, 0};
        const int8_t *bd = ce::board();
        for (int i = 0; i < 64; i++) { if (bd[i] > 0) w += V[bd[i]]; else if (bd[i] < 0) b += V[-bd[i]]; }
        event_ = ad_.onMaterial(snap_.humanWhite ? w - b : b - w);
    }
    refresh();
    checkEnd();
    return true;
}

bool GameCore::undo() {
    if (ce::plies() == 0) return false;
    // Con el aviso pendiente (o si ya le toca a la CPU) solo se retira la jugada del humano;
    // en su turno se retiran la respuesta de la CPU y su jugada anterior.
    int n = cpuToMove() ? 1 : 2;
    while (n-- > 0 && ce::plies() > 0) ce::undoPly();
    awaiting_ = false;
    ended_ = false;
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

// ── Partidas guardadas ──────────────────────────────────────────────────
void GameCore::refreshSlots() {
    static uint8_t buf[SAVE_MAX_BYTES];
    for (int i = 0; i < SAVE_SLOTS; i++) {
        slots_[i] = SlotInfo();
        int n = store_ ? store_->read(i, buf, sizeof buf) : -1;
        SaveReader r;
        if (n > 0 && r.parse(buf, n)) {
            slots_[i].used = true;
            slots_[i].humanWhite = r.header().humanWhite;
            slots_[i].level = r.header().level;
            slots_[i].plies = r.header().plies;
        }
    }
    snap_.seq++;
    for (int i = 0; i < SAVE_SLOTS; i++) snap_.slots[i] = slots_[i];
}

bool GameCore::saveSlot(int slot) {
    static uint8_t buf[SAVE_MAX_BYTES];
    bool ok = store_ && slot >= 0 && slot < SAVE_SLOTS && !fromFen_ && ce::plies() > 0;
    if (ok) {
        SaveHeader h;
        h.humanWhite = snap_.humanWhite;
        h.coach = coach_;
        h.adaptive = adaptive_;
        h.level = (uint8_t)snap_.level;
        h.adj = (int8_t)ad_.adj;
        SaveWriter w(buf, sizeof buf);
        ok = w.begin(h);
        for (int i = 0; ok && i < ce::plies(); i++) {
            ce::Move m;
            ok = ce::historyMove(i, m) && w.add(m);
        }
        size_t n = ok ? w.finish() : 0;
        ok = n > 0 && store_->write(slot, buf, n);
    }
    if (ok) refreshSlots();
    op_ = ok ? 1 : -1;
    refresh();
    return ok;
}

bool GameCore::loadSlot(int slot) {
    static uint8_t buf[SAVE_MAX_BYTES];
    SaveReader r;
    int n = (store_ && slot >= 0 && slot < SAVE_SLOTS) ? store_->read(slot, buf, sizeof buf) : -1;
    bool ok = n > 0 && r.parse(buf, n);
    if (ok) {
        // Guardar la partida actual para restaurarla si el hueco no se puede rejugar (jugada ilegal)
        std::vector<ce::Move> old;
        for (int i = 0; i < ce::plies(); i++) { ce::Move m; if (ce::historyMove(i, m)) old.push_back(m); }
        ce::newGame();
        for (int i = 0; ok && i < r.header().plies; i++) ok = ce::play(r.move(i));
        if (ok) {
            const SaveHeader &h = r.header();
            snap_.humanWhite = h.humanWhite;
            snap_.level = h.level < LEVEL_COUNT ? h.level : LEVEL_COUNT - 1;
            coach_ = h.coach;
            adaptive_ = h.adaptive;
            ad_.adj = h.adj < -Adaptive::MAX_ADJ ? -Adaptive::MAX_ADJ : (h.adj > Adaptive::MAX_ADJ ? Adaptive::MAX_ADJ : h.adj);
            ad_.cooldown = 0;
            awaiting_ = false;
            fromFen_ = false;
            event_ = 0;
        } else {
            ce::newGame();
            for (const ce::Move &m : old) ce::play(m);
        }
    }
    op_ = ok ? 2 : -2;
    refresh();
    if (ok) ended_ = over();  // una partida terminada cargada no vuelve a ajustar el nivel
    return ok;
}

bool GameCore::deleteSlot(int slot) {
    bool ok = store_ && slot >= 0 && slot < SAVE_SLOTS && store_->erase(slot);
    if (ok) refreshSlots();
    op_ = ok ? 3 : -3;
    refresh();
    return ok;
}
