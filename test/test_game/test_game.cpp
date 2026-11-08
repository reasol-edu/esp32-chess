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

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_new_game_snapshot);
    RUN_TEST(test_cpu_replies_legally_at_every_level);
    RUN_TEST(test_human_black_cpu_opens);
    RUN_TEST(test_undo_returns_turn_to_human);
    RUN_TEST(test_captured_pieces_counted);
    RUN_TEST(test_hint_is_legal);
    RUN_TEST(test_level_table_is_monotonic);
    return UNITY_END();
}
