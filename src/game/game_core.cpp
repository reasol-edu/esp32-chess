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
    snap_.over = snap_.status >= ce::ST_MATE || resigned_ != 0;
    snap_.resigned = resigned_;
    snap_.twoPlayer = twoPlayer_;
    memcpy(snap_.name1, name1_, NAME_LEN);
    memcpy(snap_.name2, name2_, NAME_LEN);
    snap_.prefs = prefs_;
    snap_.nLegal = snap_.over ? 0 : ce::legalMoves(snap_.legal, 256);
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
    NewGameOpts o;
    o.humanWhite = humanWhite;
    o.level = level;
    o.coach = coach;
    o.adaptive = adaptive;
    memcpy(o.name1, name1_, NAME_LEN);  // se conservan los nombres de la partida anterior
    memcpy(o.name2, name2_, NAME_LEN);
    o.twoPlayer = false;
    newGame(o);
}

void GameCore::newGame(const NewGameOpts &o) {
    ce::newGame();
    twoPlayer_ = o.twoPlayer;
    snap_.humanWhite = twoPlayer_ ? true : o.humanWhite;
    snap_.level = o.level < 0 ? 0 : (o.level >= LEVEL_COUNT ? LEVEL_COUNT - 1 : o.level);
    coach_ = twoPlayer_ ? false : o.coach;      // sin avisos ni adaptación entre dos personas
    adaptive_ = twoPlayer_ ? false : o.adaptive;
    name_sanitize(name1_, o.name1);
    name_sanitize(name2_, o.name2);
    awaiting_ = false;
    ended_ = false;
    fromFen_ = false;
    resigned_ = 0;
    event_ = 0;
    ad_.cooldown = 0;  // el ajuste (adj) se conserva entre partidas
    // Recordar lo elegido para el próximo arranque
    prefs_.humanWhite = o.humanWhite;
    prefs_.level = (uint8_t)snap_.level;
    prefs_.coach = o.coach;
    prefs_.adaptive = o.adaptive;
    prefs_.twoPlayer = o.twoPlayer;
    memcpy(prefs_.name1, name1_, NAME_LEN);
    memcpy(prefs_.name2, name2_, NAME_LEN);
    uint8_t buf[4 + 2 * NAME_LEN + 2];
    size_t n = prefs_encode(prefs_, buf, sizeof buf);
    if (store_ && n) store_->write(PREFS_SLOT, buf, n);
    refresh();
}

void GameCore::loadPrefs() {
    uint8_t buf[4 + 2 * NAME_LEN + 2];
    GamePrefs p;
    int n = store_ ? store_->read(PREFS_SLOT, buf, sizeof buf) : -1;
    if (n > 0 && prefs_decode(buf, n, p)) {
        prefs_ = p;
        memcpy(name1_, p.name1, NAME_LEN);   // los nombres vuelven a estar disponibles tras reiniciar
        memcpy(name2_, p.name2, NAME_LEN);
    }
}

bool GameCore::resign(bool white) {
    if (over() || ce::plies() == 0) return false;
    resigned_ = white ? 1 : 2;
    awaiting_ = false;
    if (!ended_ && adaptive_ && !twoPlayer_) ad_.onGameEnd(white != snap_.humanWhite, white == snap_.humanWhite);
    ended_ = true;
    refresh();
    return true;
}

void GameCore::checkEnd() {
    if (ended_ || !over()) return;
    ended_ = true;
    if (snap_.status != ce::ST_MATE || twoPlayer_) return;
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
    if (coach_ && !twoPlayer_ && !over()) {
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
    int n = (twoPlayer_ || cpuToMove()) ? 1 : 2;
    resigned_ = 0;
    while (n-- > 0 && ce::plies() > 0) ce::undoPly();
    awaiting_ = false;
    ended_ = false;
    refresh();
    return true;
}

bool GameCore::hint() {
    if (!humanToMove() || twoPlayer_) return false;
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
            slots_[i].twoPlayer = r.header().twoPlayer;
            memcpy(slots_[i].name1, r.header().name1, NAME_LEN);
            memcpy(slots_[i].name2, r.header().name2, NAME_LEN);
            slots_[i].level = r.header().level;
            slots_[i].plies = r.header().plies;
        }
    }
    snap_.seq++;
    snap_.prefs = prefs_;  // al asignar el almacén ya están leídas las preferencias
    for (int i = 0; i < SAVE_SLOTS; i++) snap_.slots[i] = slots_[i];
}

bool GameCore::saveSlot(int slot) {
    static uint8_t buf[SAVE_MAX_BYTES];
    bool ok = store_ && slot >= 0 && slot < SAVE_SLOTS && !fromFen_ && ce::plies() > 0 && resigned_ == 0;
    if (ok) {
        SaveHeader h;
        h.humanWhite = snap_.humanWhite;
        h.coach = coach_;
        h.adaptive = adaptive_;
        h.level = (uint8_t)snap_.level;
        h.adj = (int8_t)ad_.adj;
        h.twoPlayer = twoPlayer_;
        memcpy(h.name1, name1_, NAME_LEN);
        memcpy(h.name2, name2_, NAME_LEN);
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
            twoPlayer_ = h.twoPlayer;
            coach_ = h.coach && !twoPlayer_;
            adaptive_ = h.adaptive && !twoPlayer_;
            memcpy(name1_, h.name1, NAME_LEN);
            memcpy(name2_, h.name2, NAME_LEN);
            resigned_ = 0;
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
