#include <Arduino.h>
#include <unity.h>
#include "game_core.h"
using namespace ce;

static Move sq(const char *a, const char *b, char promo = 0) {
    return Move{(int8_t)((8 - (a[1] - '0')) * 8 + (a[0] - 'a')), (int8_t)((8 - (b[1] - '0')) * 8 + (b[0] - 'a')), promo};
}
void setUp() {}
void tearDown() {}

void test_new_game_snapshot() {
    GameCore g; g.newGame(true, 0);
    TEST_ASSERT_EQUAL(20, g.snap().nLegal);
    TEST_ASSERT_TRUE(g.humanToMove());
    TEST_ASSERT_EQUAL(0, g.snap().missingBlack[0]);
}

void test_cpu_replies_legally_at_every_level() {
    for (int lv = 0; lv < LEVEL_COUNT - 1; lv++) {  // el Dragón (8 s) se omite en el test
        GameCore g; g.seed(42 + lv); g.newGame(true, lv);
        TEST_ASSERT_TRUE(g.humanMove(sq("e2", "e4")));
        TEST_ASSERT_TRUE(g.cpuToMove());
        Move played;
        TEST_ASSERT_TRUE(g.cpuMove(&played));
        TEST_ASSERT_TRUE(g.humanToMove());
        TEST_ASSERT_EQUAL(2, g.snap().plies);
    }
}

void test_human_black_cpu_opens() {
    GameCore g; g.newGame(false, 1);
    TEST_ASSERT_TRUE(g.cpuToMove());
    TEST_ASSERT_FALSE(g.humanMove(sq("e7", "e5")));  // aún no es su turno
    TEST_ASSERT_TRUE(g.cpuMove());
    TEST_ASSERT_TRUE(g.humanToMove());
}

void test_undo_returns_turn_to_human() {
    GameCore g; g.seed(7); g.newGame(true, 2);
    g.humanMove(sq("e2", "e4")); g.cpuMove();
    g.humanMove(sq("d2", "d4")); g.cpuMove();
    TEST_ASSERT_EQUAL(4, g.snap().plies);
    TEST_ASSERT_TRUE(g.undo());
    TEST_ASSERT_EQUAL(2, g.snap().plies);
    TEST_ASSERT_TRUE(g.humanToMove());
}

void test_captured_pieces_counted() {
    GameCore g;
    TEST_ASSERT_TRUE(g.loadFen("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq -", true, 0));
    TEST_ASSERT_EQUAL(0, g.snap().missingBlack[0]);
    TEST_ASSERT_TRUE(g.humanMove(sq("e4", "d5")));  // el humano captura el peón
    TEST_ASSERT_EQUAL(1, g.snap().missingBlack[0]);
    TEST_ASSERT_EQUAL(0, g.snap().missingWhite[0]);
}

void test_hint_is_legal() {
    GameCore g; g.newGame(true, 3);
    TEST_ASSERT_TRUE(g.hint());
    TEST_ASSERT_TRUE(g.snap().hasHint);
    bool found = false;
    for (int i = 0; i < g.snap().nLegal; i++) if (g.snap().legal[i] == g.snap().hint) found = true;
    TEST_ASSERT_TRUE(found);
}

void test_level_table_is_monotonic() {
    for (int i = 1; i < LEVEL_COUNT; i++) {
        TEST_ASSERT_TRUE(level_info(i).depth >= level_info(i - 1).depth);
        TEST_ASSERT_TRUE(level_info(i).randomPct <= level_info(i - 1).randomPct);
    }
}

static const char *FEN_QG4 = "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq -";  // Dg4?? se pierde la dama

void test_danger_detects_hanging_queen() {
    GameCore g; g.seed(3);
    TEST_ASSERT_TRUE(g.loadFen(FEN_QG4, true, 3));
    TEST_ASSERT_TRUE(g.humanMove(sq("d1", "g4")));
    TEST_ASSERT_TRUE(g.snap().awaiting);
    TEST_ASSERT_EQUAL(sq("g4", "g4").from, g.snap().dangerSq);
    TEST_ASSERT_TRUE(g.snap().dangerLoss >= 800);
    TEST_ASSERT_FALSE(g.cpuMayMove());           // la CPU espera
    TEST_ASSERT_FALSE(g.cpuMove());
}

void test_danger_not_raised_for_safe_move() {
    GameCore g;
    TEST_ASSERT_TRUE(g.loadFen(FEN_QG4, true, 3));
    TEST_ASSERT_TRUE(g.humanMove(sq("g1", "f3")));
    TEST_ASSERT_FALSE(g.snap().awaiting);
    TEST_ASSERT_TRUE(g.cpuMayMove());
}

void test_danger_ignores_pawn_trade() {  // exd5 Dxd5: cambio de peones, se pierde 1 peon (<300): sin aviso
    GameCore g;
    TEST_ASSERT_TRUE(g.loadFen("rnbqkbnr/ppp2ppp/8/3pp3/3PP3/8/PPP2PPP/RNBQKBNR w KQkq -", true, 3));
    TEST_ASSERT_TRUE(g.humanMove(sq("e4", "d5")));
    TEST_ASSERT_FALSE(g.snap().awaiting);
}

void test_danger_detects_hanging_knight_for_pawn() {  // Cxe5?? Cxe5: un caballo por un peon -> aviso
    GameCore g;
    TEST_ASSERT_TRUE(g.loadFen("r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq -", true, 3));
    TEST_ASSERT_TRUE(g.humanMove(sq("f3", "e5")));
    TEST_ASSERT_TRUE(g.snap().awaiting);
}

void test_danger_respects_coach_off() {
    GameCore g;
    TEST_ASSERT_TRUE(g.loadFen(FEN_QG4, true, 3));
    g.newGame(true, 3, false, true);
    ce::setFen(FEN_QG4);
    g.loadFen(FEN_QG4, true, 3);  // loadFen conserva el coach elegido en newGame
    TEST_ASSERT_TRUE(g.humanMove(sq("d1", "g4")));
    TEST_ASSERT_FALSE(g.snap().awaiting);
}

void test_confirm_lets_cpu_move() {
    GameCore g; g.seed(5);
    g.loadFen(FEN_QG4, true, 3);
    g.humanMove(sq("d1", "g4"));
    TEST_ASSERT_TRUE(g.confirm());
    TEST_ASSERT_FALSE(g.snap().awaiting);
    TEST_ASSERT_TRUE(g.cpuMove());
}

void test_undo_while_awaiting_removes_only_human_move() {
    GameCore g;
    g.loadFen(FEN_QG4, true, 3);
    g.humanMove(sq("d1", "g4"));
    TEST_ASSERT_EQUAL(1, g.snap().plies);
    TEST_ASSERT_TRUE(g.undo());
    TEST_ASSERT_EQUAL(0, g.snap().plies);
    TEST_ASSERT_TRUE(g.humanToMove());
    TEST_ASSERT_FALSE(g.snap().awaiting);
}

void test_danger_analysis_is_fast_in_long_game() {
    GameCore g; g.seed(9);
    g.newGame(true, 0, false, false);
    for (int i = 0; i < 40 && !g.over(); i++) {   // partida larga con jugadas legales de ambos lados
        if (g.humanToMove()) { g.humanMove(g.snap().legal[i % g.snap().nLegal]); }
        if (g.cpuMayMove()) g.cpuMove();
    }
    unsigned long t0 = millis();
    Danger d = analyze_danger(true);
    (void)d;
    printf("  analisis de peligro con %d jugadas: %lu ms\n", g.snap().plies, millis() - t0);
    TEST_ASSERT_LESS_THAN(500, (int)(millis() - t0));
}

void test_adaptive_eases_and_hardens() {
    Adaptive a;
    TEST_ASSERT_EQUAL(0, a.onMaterial(100));
    TEST_ASSERT_EQUAL(-1, a.onMaterial(-400));       // el humano va perdiendo: afloja
    TEST_ASSERT_EQUAL(-1, a.adj);
    for (int i = 0; i < Adaptive::COOLDOWN; i++) TEST_ASSERT_EQUAL(0, a.onMaterial(-900));  // enfriamiento
    TEST_ASSERT_EQUAL(-1, a.onMaterial(-900));
    TEST_ASSERT_EQUAL(-2, a.adj);
    for (int i = 0; i < 10; i++) a.onMaterial(-900);
    TEST_ASSERT_EQUAL(-2, a.adj);                    // tope
    a = Adaptive();
    TEST_ASSERT_EQUAL(+1, a.onMaterial(500));
    TEST_ASSERT_EQUAL(1, a.adj);
}

void test_adaptive_effective_level_clamped() {
    Adaptive a; a.adj = 2;
    TEST_ASSERT_EQUAL(LEVEL_COUNT - 1, a.effective(LEVEL_COUNT - 1));
    a.adj = -2;
    TEST_ASSERT_EQUAL(0, a.effective(1));
    TEST_ASSERT_EQUAL(1, a.effective(3));
}

void test_adaptive_carries_to_next_game() {
    GameCore g; g.seed(2);
    g.loadFen("6k1/5ppp/8/8/8/8/8/R5K1 w - -", true, 2);   // mate en 1 del humano
    TEST_ASSERT_TRUE(g.humanMove(sq("a1", "a8")));
    TEST_ASSERT_EQUAL(ce::ST_MATE, g.snap().status);
    g.newGame(true, 2);
    TEST_ASSERT_EQUAL(3, g.snap().effLevel);                // ganó: la siguiente partida sube un nivel
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_new_game_snapshot);
    RUN_TEST(test_cpu_replies_legally_at_every_level);
    RUN_TEST(test_human_black_cpu_opens);
    RUN_TEST(test_undo_returns_turn_to_human);
    RUN_TEST(test_captured_pieces_counted);
    RUN_TEST(test_hint_is_legal);
    RUN_TEST(test_level_table_is_monotonic);
    RUN_TEST(test_danger_detects_hanging_queen);
    RUN_TEST(test_danger_not_raised_for_safe_move);
    RUN_TEST(test_danger_ignores_pawn_trade);
    RUN_TEST(test_danger_detects_hanging_knight_for_pawn);
    RUN_TEST(test_danger_respects_coach_off);
    RUN_TEST(test_confirm_lets_cpu_move);
    RUN_TEST(test_undo_while_awaiting_removes_only_human_move);
    RUN_TEST(test_danger_analysis_is_fast_in_long_game);
    RUN_TEST(test_adaptive_eases_and_hardens);
    RUN_TEST(test_adaptive_effective_level_clamped);
    RUN_TEST(test_adaptive_carries_to_next_game);
    return UNITY_END();
}
