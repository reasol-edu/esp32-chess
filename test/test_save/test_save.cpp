#include <Arduino.h>
#include <unity.h>
#include <string.h>
#include "game_core.h"
using namespace ce;

static Move sq(const char *a, const char *b, char promo = 0) {
    return Move{(int8_t)((8 - (a[1] - '0')) * 8 + (a[0] - 'a')), (int8_t)((8 - (b[1] - '0')) * 8 + (b[0] - 'a')), promo};
}
void setUp() {}
void tearDown() {}

// Partida corta determinista con la CPU en nivel 0 (semilla fija)
static void play_some(GameCore &g, int rounds) {
    const char *mv[][2] = {{"e2", "e4"}, {"g1", "f3"}, {"d2", "d4"}, {"b1", "c3"}};
    for (int i = 0; i < rounds && !g.over(); i++) {
        if (g.humanToMove()) {
            // primera jugada legal razonable: la propuesta o la primera legal
            Move m = sq(mv[i][0], mv[i][1]);
            bool legal = false;
            for (int k = 0; k < g.snap().nLegal; k++) if (g.snap().legal[k] == m) legal = true;
            g.humanMove(legal ? m : g.snap().legal[0]);
        }
        if (g.cpuMayMove()) g.cpuMove();
    }
}

void test_format_roundtrip() {
    uint8_t buf[SAVE_MAX_BYTES];
    SaveHeader h; h.humanWhite = false; h.coach = false; h.adaptive = true; h.level = 4; h.adj = -2;
    h.twoPlayer = true; strcpy(h.name1, "Ana"); strcpy(h.name2, "Ni\xC3\xB1o");
    SaveWriter w(buf, sizeof buf);
    TEST_ASSERT_TRUE(w.begin(h));
    Move in[3] = {sq("e2", "e4"), sq("a7", "a8", 'q'), sq("h2", "h1", 'n')};
    for (auto &m : in) TEST_ASSERT_TRUE(w.add(m));
    size_t n = w.finish();
    TEST_ASSERT_EQUAL((int)SAVE_HEADER_BYTES + 6 + 2, (int)n);
    SaveReader r;
    TEST_ASSERT_TRUE(r.parse(buf, n));
    TEST_ASSERT_FALSE(r.header().humanWhite);
    TEST_ASSERT_FALSE(r.header().coach);
    TEST_ASSERT_TRUE(r.header().adaptive);
    TEST_ASSERT_EQUAL(4, r.header().level);
    TEST_ASSERT_EQUAL(-2, r.header().adj);
    TEST_ASSERT_EQUAL(3, r.header().plies);
    TEST_ASSERT_TRUE(r.header().twoPlayer);
    TEST_ASSERT_EQUAL_STRING("Ana", r.header().name1);
    TEST_ASSERT_EQUAL_STRING("Ni\xC3\xB1o", r.header().name2);
    for (int i = 0; i < 3; i++) TEST_ASSERT_TRUE(r.move(i) == in[i]);
}

void test_format_detects_corruption() {
    uint8_t buf[SAVE_MAX_BYTES];
    SaveWriter w(buf, sizeof buf);
    w.begin(SaveHeader());
    w.add(sq("e2", "e4"));
    size_t n = w.finish();
    SaveReader r;
    TEST_ASSERT_TRUE(r.parse(buf, n));
    uint8_t bad[SAVE_MAX_BYTES];
    memcpy(bad, buf, n); bad[SAVE_HEADER_BYTES] ^= 0x01;            // jugada alterada
    TEST_ASSERT_FALSE(r.parse(bad, n));
    TEST_ASSERT_FALSE(r.parse(buf, n - 1));          // truncado
    memcpy(bad, buf, n); bad[0] = 0;                 // marca incorrecta
    TEST_ASSERT_FALSE(r.parse(bad, n));
    memcpy(bad, buf, n); bad[1] = 99;                // versión desconocida
    TEST_ASSERT_FALSE(r.parse(bad, n));
    TEST_ASSERT_FALSE(r.parse(buf, 3));
}

void test_save_and_load_restores_position() {
    MemSaveStore st; GameCore g; g.setStore(&st); g.seed(11);
    g.newGame(true, 0, true, false);
    play_some(g, 3);
    int plies = g.snap().plies;
    int8_t board[64]; memcpy(board, g.snap().board, 64);
    TEST_ASSERT_TRUE(plies >= 4);
    TEST_ASSERT_TRUE(g.saveSlot(1));
    TEST_ASSERT_EQUAL(1, g.snap().lastOp);
    TEST_ASSERT_TRUE(g.snap().slots[1].used);
    TEST_ASSERT_FALSE(g.snap().slots[0].used);
    TEST_ASSERT_EQUAL(plies, g.snap().slots[1].plies);
    play_some(g, 4);                                  // seguir jugando después de guardar
    TEST_ASSERT_TRUE(g.snap().plies >= plies);
    TEST_ASSERT_TRUE(g.loadSlot(1));
    TEST_ASSERT_EQUAL(2, g.snap().lastOp);
    TEST_ASSERT_EQUAL(plies, g.snap().plies);
    TEST_ASSERT_EQUAL_MEMORY(board, g.snap().board, 64);
}

void test_load_restores_settings() {
    MemSaveStore st; GameCore g; g.setStore(&st); g.seed(3);
    g.newGame(false, 3, false, true);                 // humano con negras, nivel 3, sin avisos, adaptable
    play_some(g, 2);
    TEST_ASSERT_TRUE(g.saveSlot(0));
    g.newGame(true, 0, true, false);
    TEST_ASSERT_TRUE(g.loadSlot(0));
    TEST_ASSERT_FALSE(g.snap().humanWhite);
    TEST_ASSERT_EQUAL(3, g.snap().level);
    TEST_ASSERT_FALSE(g.snap().coach);
    TEST_ASSERT_TRUE(g.snap().adaptive);
}

void test_undo_works_after_load() {
    MemSaveStore st; GameCore g; g.setStore(&st); g.seed(8);
    g.newGame(true, 0, false, false);
    play_some(g, 3);
    int plies = g.snap().plies;
    g.saveSlot(2);
    g.newGame(true, 0, false, false);
    TEST_ASSERT_TRUE(g.loadSlot(2));
    TEST_ASSERT_TRUE(g.undo());
    TEST_ASSERT_EQUAL(plies - 2, g.snap().plies);
}

void test_corrupt_slot_keeps_current_game() {
    MemSaveStore st; GameCore g; g.setStore(&st); g.seed(5);
    g.newGame(true, 0, false, false);
    play_some(g, 2);
    g.saveSlot(0);
    st.d[0][SAVE_HEADER_BYTES] ^= 0x7F;                               // se corrompe el hueco
    play_some(g, 1);
    int plies = g.snap().plies; int8_t board[64]; memcpy(board, g.snap().board, 64);
    TEST_ASSERT_FALSE(g.loadSlot(0));
    TEST_ASSERT_EQUAL(-2, g.snap().lastOp);
    TEST_ASSERT_EQUAL(plies, g.snap().plies);
    TEST_ASSERT_EQUAL_MEMORY(board, g.snap().board, 64);
}

void test_illegal_moves_in_valid_crc_restore_current_game() {
    MemSaveStore st; GameCore g; g.setStore(&st); g.seed(5);
    uint8_t buf[SAVE_MAX_BYTES];
    SaveWriter w(buf, sizeof buf);
    w.begin(SaveHeader());
    w.add(sq("e2", "e5"));                            // jugada ilegal con CRC correcto
    size_t n = w.finish();
    st.write(1, buf, n);
    g.refreshSlotsForTest();
    g.newGame(true, 0, false, false);
    play_some(g, 2);
    int plies = g.snap().plies; int8_t board[64]; memcpy(board, g.snap().board, 64);
    TEST_ASSERT_FALSE(g.loadSlot(1));
    TEST_ASSERT_EQUAL(plies, g.snap().plies);         // la partida en curso se restaura entera
    TEST_ASSERT_EQUAL_MEMORY(board, g.snap().board, 64);
    TEST_ASSERT_TRUE(g.undo());                       // y su historial sigue siendo usable
}

void test_delete_and_empty_slot() {
    MemSaveStore st; GameCore g; g.setStore(&st); g.seed(1);
    g.newGame(true, 0);
    play_some(g, 1);
    g.saveSlot(0);
    TEST_ASSERT_TRUE(g.deleteSlot(0));
    TEST_ASSERT_EQUAL(3, g.snap().lastOp);
    TEST_ASSERT_FALSE(g.snap().slots[0].used);
    TEST_ASSERT_FALSE(g.loadSlot(0));
    TEST_ASSERT_EQUAL(-2, g.snap().lastOp);
}

void test_save_failures() {
    MemSaveStore st; GameCore g; g.setStore(&st); g.seed(1);
    g.newGame(true, 0);
    TEST_ASSERT_FALSE(g.saveSlot(0));                 // sin jugadas no se guarda
    TEST_ASSERT_EQUAL(-1, g.snap().lastOp);
    play_some(g, 1);
    st.failWrites = true;
    TEST_ASSERT_FALSE(g.saveSlot(0));                 // fallo del almacenamiento
    TEST_ASSERT_EQUAL(-1, g.snap().lastOp);
    st.failWrites = false;
    TEST_ASSERT_FALSE(g.saveSlot(7));                 // hueco inexistente
    GameCore h;                                       // sin almacén
    h.newGame(true, 0);
    TEST_ASSERT_FALSE(h.saveSlot(0));
}

void test_long_game_fits() {
    MemSaveStore st; GameCore g; g.setStore(&st); g.seed(21);
    g.newGame(true, 0, false, false);
    for (int i = 0; i < 300 && !g.over(); i++) {
        if (g.humanToMove()) g.humanMove(g.snap().legal[(i * 7) % g.snap().nLegal]);
        if (g.cpuMayMove()) g.cpuMove();
    }
    int plies = g.snap().plies;
    TEST_ASSERT_TRUE(g.saveSlot(0));
    g.newGame(true, 0);
    TEST_ASSERT_TRUE(g.loadSlot(0));
    TEST_ASSERT_EQUAL(plies, g.snap().plies);
    printf("  partida de %d jugadas: %d bytes\n", plies, (int)st.d[0].size());
}

void test_fen_positions_cannot_be_saved() {
    MemSaveStore st; GameCore g; g.setStore(&st); g.seed(2);
    g.loadFen("6k1/5ppp/8/8/8/8/8/R5K1 w - -", true, 2);   // mate en 1
    g.humanMove(sq("a1", "a8"));
    TEST_ASSERT_EQUAL(ce::ST_MATE, g.snap().status);
    // loadFen marca la posición inicial como no estándar: no se puede reconstruir desde cero, no se guarda
    TEST_ASSERT_FALSE(g.saveSlot(0));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_format_roundtrip);
    RUN_TEST(test_format_detects_corruption);
    RUN_TEST(test_save_and_load_restores_position);
    RUN_TEST(test_load_restores_settings);
    RUN_TEST(test_undo_works_after_load);
    RUN_TEST(test_corrupt_slot_keeps_current_game);
    RUN_TEST(test_illegal_moves_in_valid_crc_restore_current_game);
    RUN_TEST(test_delete_and_empty_slot);
    RUN_TEST(test_save_failures);
    RUN_TEST(test_long_game_fits);
    RUN_TEST(test_fen_positions_cannot_be_saved);
    return UNITY_END();
}
