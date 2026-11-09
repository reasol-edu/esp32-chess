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

static NewGameOpts two(const char *n1, const char *n2) {
    NewGameOpts o;
    o.twoPlayer = true;
    strcpy(o.name1, n1);
    strcpy(o.name2, n2);
    return o;
}

void test_sanitize_names() {
    char o[NAME_LEN];
    name_sanitize(o, "  Ana  ");            TEST_ASSERT_EQUAL_STRING("Ana", o);
    name_sanitize(o, "Mar\x01ía");          TEST_ASSERT_EQUAL_STRING("Mar\xC3\xAD" "a", o);   // sin control, con acento
    name_sanitize(o, "ABCDEFGHIJKLMNOP");   TEST_ASSERT_EQUAL_STRING("ABCDEFGHIJ", o);       // 10 caracteres
    name_sanitize(o, "\xC3\xB1\xC3\xB1\xC3\xB1\xC3\xB1\xC3\xB1\xC3\xB1\xC3\xB1\xC3\xB1\xC3\xB1\xC3\xB1\xC3\xB1");
    TEST_ASSERT_EQUAL(20, (int)strlen(o));                                                   // 10 ñ = 20 bytes, sin cortar
    name_sanitize(o, "Ab\xC3");             TEST_ASSERT_EQUAL_STRING("Ab", o);                // UTF-8 truncado
    name_sanitize(o, nullptr);              TEST_ASSERT_EQUAL_STRING("", o);
    name_sanitize(o, "   ");                TEST_ASSERT_EQUAL_STRING("", o);
}

void test_both_sides_move_and_no_cpu() {
    GameCore g; g.newGame(two("Ana", "Luis"));
    TEST_ASSERT_TRUE(g.snap().twoPlayer);
    TEST_ASSERT_TRUE(g.humanToMove());
    TEST_ASSERT_FALSE(g.cpuToMove());
    TEST_ASSERT_FALSE(g.cpuMayMove());
    TEST_ASSERT_TRUE(g.humanMove(sq("e2", "e4")));      // blancas
    TEST_ASSERT_TRUE(g.humanToMove());                   // ahora las negras, también una persona
    TEST_ASSERT_FALSE(g.humanMove(sq("e2", "e3")));      // jugada ilegal para el que mueve
    TEST_ASSERT_TRUE(g.humanMove(sq("e7", "e5")));
    TEST_ASSERT_EQUAL(2, g.snap().plies);
    TEST_ASSERT_EQUAL_STRING("Ana", g.snap().name1);
    TEST_ASSERT_EQUAL_STRING("Luis", g.snap().name2);
}

void test_no_coach_hint_or_adaptation_in_two_player() {
    GameCore g; g.newGame(two("A", "B"));
    g.loadFen("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq -", true, 3);   // Dg4?? en 1 jugador avisaría
    TEST_ASSERT_TRUE(g.humanMove(sq("d1", "g4")));
    TEST_ASSERT_FALSE(g.snap().awaiting);
    TEST_ASSERT_FALSE(g.snap().coach);
    TEST_ASSERT_FALSE(g.snap().adaptive);
    TEST_ASSERT_FALSE(g.hint());
}

void test_undo_removes_one_ply_in_two_player() {
    GameCore g; g.newGame(two("A", "B"));
    g.humanMove(sq("e2", "e4")); g.humanMove(sq("e7", "e5"));
    TEST_ASSERT_TRUE(g.undo());
    TEST_ASSERT_EQUAL(1, g.snap().plies);
    TEST_ASSERT_TRUE(g.snap().whiteToMove == false);
}

void test_mate_and_resign_in_two_player() {
    GameCore g; g.newGame(two("Ana", "Luis"));
    g.humanMove(sq("f2", "f3")); g.humanMove(sq("e7", "e5")); g.humanMove(sq("g2", "g4")); g.humanMove(sq("d8", "h4"));
    TEST_ASSERT_EQUAL(ST_MATE, g.snap().status);
    TEST_ASSERT_TRUE(g.snap().over);
    TEST_ASSERT_FALSE(g.humanToMove());
    GameCore h; h.newGame(two("Ana", "Luis"));
    TEST_ASSERT_FALSE(h.resign(true));                   // sin jugadas no se puede rendir
    h.humanMove(sq("e2", "e4"));
    TEST_ASSERT_TRUE(h.resign(false));                   // se rinden las negras
    TEST_ASSERT_TRUE(h.snap().over);
    TEST_ASSERT_EQUAL(2, h.snap().resigned);
    TEST_ASSERT_EQUAL(0, h.snap().nLegal);
    TEST_ASSERT_FALSE(h.humanMove(sq("e7", "e5")));
    TEST_ASSERT_TRUE(h.undo());                          // deshacer reabre la partida
    TEST_ASSERT_FALSE(h.snap().over);
}

void test_single_player_still_uses_cpu() {
    GameCore g; g.seed(1);
    NewGameOpts o; o.level = 0; o.coach = false;
    g.newGame(o);
    TEST_ASSERT_FALSE(g.snap().twoPlayer);
    g.humanMove(sq("e2", "e4"));
    TEST_ASSERT_TRUE(g.cpuMayMove());
    TEST_ASSERT_TRUE(g.cpuMove());
}

void test_prefs_persist_across_instances() {
    MemSaveStore st;
    { GameCore g; g.setStore(&st);
      NewGameOpts o = two("Ana", "Ni\xC3\xB1o"); o.level = 4; o.humanWhite = false; o.coach = false; o.adaptive = false;
      g.newGame(o); }
    GameCore h; h.setStore(&st);                         // "reinicio": se leen las preferencias
    TEST_ASSERT_TRUE(h.snap().prefs.twoPlayer);
    TEST_ASSERT_EQUAL(4, h.snap().prefs.level);
    TEST_ASSERT_FALSE(h.snap().prefs.humanWhite);
    TEST_ASSERT_FALSE(h.snap().prefs.coach);
    TEST_ASSERT_FALSE(h.snap().prefs.adaptive);
    TEST_ASSERT_EQUAL_STRING("Ana", h.snap().prefs.name1);
    TEST_ASSERT_EQUAL_STRING("Ni\xC3\xB1o", h.snap().prefs.name2);
}

void test_corrupt_prefs_are_ignored() {
    MemSaveStore st;
    { GameCore g; g.setStore(&st); g.newGame(two("Ana", "Luis")); }
    st.d[PREFS_SLOT][5] ^= 0x55;
    GameCore h; h.setStore(&st);
    TEST_ASSERT_FALSE(h.snap().prefs.twoPlayer);         // valores por defecto
    TEST_ASSERT_EQUAL_STRING("", h.snap().prefs.name1);
}

void test_save_load_two_player_with_names() {
    MemSaveStore st; GameCore g; g.setStore(&st);
    g.newGame(two("Ana", "Luis"));
    g.humanMove(sq("e2", "e4")); g.humanMove(sq("e7", "e5")); g.humanMove(sq("g1", "f3"));
    TEST_ASSERT_TRUE(g.saveSlot(2));
    TEST_ASSERT_TRUE(g.snap().slots[2].twoPlayer);
    TEST_ASSERT_EQUAL_STRING("Luis", g.snap().slots[2].name2);
    NewGameOpts o; o.level = 5; g.newGame(o);            // una partida distinta, de 1 jugador
    TEST_ASSERT_FALSE(g.snap().twoPlayer);
    TEST_ASSERT_TRUE(g.loadSlot(2));
    TEST_ASSERT_TRUE(g.snap().twoPlayer);
    TEST_ASSERT_EQUAL(3, g.snap().plies);
    TEST_ASSERT_EQUAL_STRING("Ana", g.snap().name1);
    TEST_ASSERT_EQUAL_STRING("Luis", g.snap().name2);
    TEST_ASSERT_FALSE(g.cpuMayMove());
    TEST_ASSERT_TRUE(g.humanMove(sq("b8", "c6")));       // le toca a las negras, una persona
}

void test_cannot_save_resigned_game() {
    MemSaveStore st; GameCore g; g.setStore(&st);
    g.newGame(two("A", "B")); g.humanMove(sq("e2", "e4")); g.resign(false);
    TEST_ASSERT_FALSE(g.saveSlot(0));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_sanitize_names);
    RUN_TEST(test_both_sides_move_and_no_cpu);
    RUN_TEST(test_no_coach_hint_or_adaptation_in_two_player);
    RUN_TEST(test_undo_removes_one_ply_in_two_player);
    RUN_TEST(test_mate_and_resign_in_two_player);
    RUN_TEST(test_single_player_still_uses_cpu);
    RUN_TEST(test_prefs_persist_across_instances);
    RUN_TEST(test_corrupt_prefs_are_ignored);
    RUN_TEST(test_save_load_two_player_with_names);
    RUN_TEST(test_cannot_save_resigned_game);
    return UNITY_END();
}
