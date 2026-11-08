#include <Arduino.h>
#include <unity.h>
#include "chess_api.h"
using namespace ce;

static int perft_like(int depth) {  // cuenta nodos usando solo la API (play/undo)
    if (depth == 0) return 1;
    Move mv[200];
    int n = legalMoves(mv, 200), total = 0;
    for (int i = 0; i < n; i++) {
        TEST_ASSERT_TRUE(play(mv[i]));
        total += perft_like(depth - 1);
        TEST_ASSERT_TRUE(undoPly());
    }
    return total;
}

void setUp() {}
void tearDown() {}

void test_start_has_20_moves() {
    newGame();
    Move mv[200];
    TEST_ASSERT_EQUAL(20, legalMoves(mv, 200));
    TEST_ASSERT_TRUE(whiteToMove());
}

void test_perft_start() {  // valores de referencia: 20, 400, 8902
    newGame();
    TEST_ASSERT_EQUAL(20, perft_like(1));
    TEST_ASSERT_EQUAL(400, perft_like(2));
    TEST_ASSERT_EQUAL(8902, perft_like(3));
}

void test_perft_kiwipete() {  // posición con enroques, al paso y coronaciones; referencia: 48, 2039
    TEST_ASSERT_TRUE(setFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"));
    TEST_ASSERT_EQUAL(48, perft_like(1));
    TEST_ASSERT_EQUAL(2039, perft_like(2));
}

void test_fools_mate() {
    newGame();
    const char *seq[][2] = {{"f2", "f3"}, {"e7", "e5"}, {"g2", "g4"}, {"d8", "h4"}};
    for (auto &m : seq) {
        Move mv{(int8_t)((8 - (m[0][1] - '0')) * 8 + (m[0][0] - 'a')), (int8_t)((8 - (m[1][1] - '0')) * 8 + (m[1][0] - 'a')), 0};
        TEST_ASSERT_TRUE(play(mv));
    }
    TEST_ASSERT_EQUAL(ST_MATE, status());
}

void test_stalemate() {
    TEST_ASSERT_TRUE(setFen("7k/5Q2/6K1/8/8/8/8/8 b - -"));
    TEST_ASSERT_EQUAL(ST_STALEMATE, status());
}

void test_search_finds_mate_in_one() {
    TEST_ASSERT_TRUE(setFen("6k1/5ppp/8/8/8/8/8/R5K1 w - -"));  // Ra8#
    Move best; int sc;
    TEST_ASSERT_TRUE(search({4, 2000}, best, sc));
    TEST_ASSERT_EQUAL(56, best.from);  // a1
    TEST_ASSERT_EQUAL(0, best.to);     // a8
}

void test_depth1_is_fast_and_legal() {
    newGame();
    Move best; int sc;
    unsigned long t0 = millis();
    TEST_ASSERT_TRUE(search({1, 5000}, best, sc));
    TEST_ASSERT_LESS_THAN(500, (int)(millis() - t0));
    TEST_ASSERT_TRUE(play(best));
}

static Move sq(const char *a, const char *b, char promo = 0) {
    return Move{(int8_t)((8 - (a[1] - '0')) * 8 + (a[0] - 'a')), (int8_t)((8 - (b[1] - '0')) * 8 + (b[0] - 'a')), promo};
}

void test_castling_and_undo() {
    TEST_ASSERT_TRUE(setFen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq -"));
    TEST_ASSERT_TRUE(play(sq("e1", "g1")));  // enroque corto
    TEST_ASSERT_EQUAL(6, board()[62] == 6 ? 6 : 0);
    TEST_ASSERT_EQUAL(4, board()[61]);       // torre en f1
    TEST_ASSERT_TRUE(play(sq("e8", "c8")));  // enroque largo negro
    TEST_ASSERT_EQUAL(-4, board()[3]);       // torre en d8
    TEST_ASSERT_TRUE(undoPly());
    TEST_ASSERT_TRUE(undoPly());
    TEST_ASSERT_EQUAL(6, board()[60]);       // rey blanco de vuelta en e1
    TEST_ASSERT_EQUAL(4, board()[63]);
    TEST_ASSERT_EQUAL(0, plies());
}

void test_en_passant() {
    TEST_ASSERT_TRUE(setFen("4k3/8/8/8/3p4/8/4P3/4K3 w - -"));
    TEST_ASSERT_TRUE(play(sq("e2", "e4")));
    TEST_ASSERT_TRUE(play(sq("d4", "e3")));  // captura al paso
    TEST_ASSERT_EQUAL(0, board()[36]);       // el peón blanco de e4 desaparece
    TEST_ASSERT_EQUAL(-1, board()[44]);
}

void test_promotion() {
    TEST_ASSERT_TRUE(setFen("8/P6k/8/8/8/8/8/K7 w - -"));
    Move mv[50];
    int n = legalMoves(mv, 50), promos = 0;
    for (int i = 0; i < n; i++) if (mv[i].promo) promos++;
    TEST_ASSERT_EQUAL(4, promos);
    TEST_ASSERT_TRUE(play(sq("a7", "a8", 'n')));
    TEST_ASSERT_EQUAL(2, board()[0]);
}

void test_illegal_move_rejected() {
    newGame();
    TEST_ASSERT_FALSE(play(sq("e2", "e5")));
    TEST_ASSERT_FALSE(play(sq("e7", "e5")));  // no es su turno
    TEST_ASSERT_EQUAL(0, plies());
}

void test_threefold_repetition() {
    newGame();
    const char *s[] = {"g1","f3","g8","f6","f3","g1","f6","g8"};
    for (int r = 0; r < 2; r++)
        for (int i = 0; i < 8; i += 2) TEST_ASSERT_TRUE(play(sq(s[i], s[i + 1])));
    TEST_ASSERT_EQUAL(ST_DRAW_REPETITION, status());
}

void test_search_time_by_level() {
    const int depths[] = {1, 2, 3, 4, 5, 6};
    for (int d : depths) {
        newGame();
        play(sq("e2", "e4")); play(sq("e7", "e5")); play(sq("g1", "f3")); play(sq("b8", "c6"));
        Move best; int sc;
        unsigned long t0 = millis();
        TEST_ASSERT_TRUE(search({d, 4000}, best, sc));
        printf("  profundidad %d: %lu ms\n", d, millis() - t0);
        TEST_ASSERT_TRUE(play(best));
    }
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_start_has_20_moves);
    RUN_TEST(test_perft_start);
    RUN_TEST(test_perft_kiwipete);
    RUN_TEST(test_fools_mate);
    RUN_TEST(test_stalemate);
    RUN_TEST(test_search_finds_mate_in_one);
    RUN_TEST(test_depth1_is_fast_and_legal);
    RUN_TEST(test_castling_and_undo);
    RUN_TEST(test_en_passant);
    RUN_TEST(test_promotion);
    RUN_TEST(test_illegal_move_rejected);
    RUN_TEST(test_threefold_repetition);
    RUN_TEST(test_search_time_by_level);
    return UNITY_END();
}
