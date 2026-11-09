// Simulador de la UI en el PC: LVGL + UI real + motor real, volcado a PPM. uso: preview <escena> <salida.ppm>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "ui_game.h"
#include "ppa_fx.h"

// ── Stubs del PPA (el simulador escala por software) ──
void ppa_fx_init() {}
bool ppa_fx_active() { return false; }
const lv_image_dsc_t *ppa_fx_lifted(int) { return nullptr; }
const lv_image_dsc_t *ppa_fx_mini(int) { return nullptr; }
const lv_image_dsc_t *ppa_fx_rot180(int) { return nullptr; }

static uint32_t g_now = 0;
static uint16_t *fb;
static uint32_t tick_cb() { return g_now; }
static void flush_cb(lv_display_t *d, const lv_area_t *a, uint8_t *px) {
    const uint16_t *src = (const uint16_t *)px;
    int w = a->x2 - a->x1 + 1;
    for (int y = a->y1; y <= a->y2; y++) memcpy(&fb[y * 1024 + a->x1], src + (y - a->y1) * w, w * 2);
    lv_display_flush_ready(d);
}
// Puntero táctil simulado: comprueba que LVGL acierta los toques sobre objetos girados 180°
static int tx = -1, ty = -1;
static void touch_read(lv_indev_t *, lv_indev_data_t *d) {
    if (tx >= 0) { d->state = LV_INDEV_STATE_PRESSED; d->point.x = tx; d->point.y = ty; }
    else d->state = LV_INDEV_STATE_RELEASED;
}
static void run(int ms) { for (int i = 0; i < ms / 16; i++) { g_now += 16; lv_timer_handler(); } }

// Host síncrono: la respuesta de la CPU se calcula en el poll siguiente (da un fotograma de "pensando").
static MemSaveStore sim_store;
struct SimHost : GameHost {
    GameCore core;
    SimHost() { core.setStore(&sim_store); }
    bool dirty = false, cpu_pending = false;
    void post_new(const NewGameOpts &o) override { core.newGame(o); dirty = true; cpu_pending = core.cpuMayMove(); }
    void post_resign(bool w) override { core.resign(w); dirty = true; }
    void load(const char *fen, bool w, int l) { core.loadFen(fen, w, l); dirty = true; cpu_pending = core.cpuMayMove(); }
    void post_human_move(const ce::Move &m) override { if (core.humanMove(m)) { dirty = true; cpu_pending = core.cpuMayMove(); } }
    void post_undo() override { if (core.undo()) { dirty = true; cpu_pending = core.cpuMayMove(); } }
    void post_hint() override { if (core.hint()) dirty = true; }
    void post_confirm() override { if (core.confirm()) { dirty = true; cpu_pending = core.cpuMayMove(); } }
    void post_save(int s) override { core.saveSlot(s); dirty = true; }
    void post_load(int s) override { if (core.loadSlot(s)) cpu_pending = core.cpuMayMove(); dirty = true; }
    void post_delete(int s) override { core.deleteSlot(s); dirty = true; }
    void stop_search() override {}
    bool poll(Snapshot &out) override {
        if (dirty) { dirty = false; out = core.snap(); return true; }
        if (cpu_pending) { cpu_pending = false; core.cpuMove(); out = core.snap(); return true; }
        return false;
    }
};
static SimHost host;

static void click(int x, int y) { tx = x; ty = y; run(120); tx = -1; ty = -1; run(120); }
static int sq(const char *s) { return (8 - (s[1] - '0')) * 8 + (s[0] - 'a'); }
static void tap(const char *s) { ui_game_debug_tap_square(sq(s)); run(100); }
static void play(const char *a, const char *b) { tap(a); tap(b); run(1500); }

static void save(const char *path) {
    FILE *f = fopen(path, "wb");
    fprintf(f, "P6\n1024 600\n255\n");
    for (int i = 0; i < 1024 * 600; i++) {
        uint16_t c = fb[i];
        uint8_t rgb[3] = {(uint8_t)(((c >> 11) & 31) * 255 / 31), (uint8_t)(((c >> 5) & 63) * 255 / 63), (uint8_t)((c & 31) * 255 / 31)};
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}

int main(int argc, char **argv) {
    std::string scene = argc > 1 ? argv[1] : "start";
    const char *out = argc > 2 ? argv[2] : "out/shot.ppm";
    lv_init();
    lv_tick_set_cb(tick_cb);
    fb = (uint16_t *)calloc(1024 * 600, 2);
    lv_display_t *d = lv_display_create(1024, 600);
    static uint16_t *buf = (uint16_t *)malloc(1024 * 600 * 2);
    lv_display_set_buffers(d, buf, nullptr, 1024 * 600 * 2, LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(d, flush_cb);
    lv_indev_t *ind = lv_indev_create();
    lv_indev_set_type(ind, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(ind, touch_read);
    lv_obj_t *scr = lv_obj_create(nullptr);
    lv_screen_load(scr);
    ui_game_create(scr, &host);
    run(300);

    if (scene == "newgame_two") {
        ui_game_debug_tap("mode:2"); ui_game_debug_tap("name:0:Ana"); ui_game_debug_tap("name:1:Luis");
    } else if (scene == "newgame_clock") {
        ui_game_debug_tap("mode:2"); ui_game_debug_tap("name:0:Ana"); ui_game_debug_tap("name:1:Luis"); ui_game_debug_tap("clock"); ui_game_debug_tap("clock");
    } else if (scene == "names") {
        ui_game_debug_tap("mode:2"); ui_game_debug_tap("edit:0");
        ui_game_debug_tap("key:A"); ui_game_debug_tap("key:n"); ui_game_debug_tap("key:a");
    } else if (scene == "names_accents") {
        ui_game_debug_tap("mode:2"); ui_game_debug_tap("edit:1"); ui_game_debug_tap("key:Acentos");
    } else if (scene == "newgame") {
        ui_game_debug_tap("lvl:3"); ui_game_debug_tap("adapt");
    } else if (scene == "newgame_black") {
        ui_game_debug_tap("lvl:5"); ui_game_debug_tap("black");
    } else {
        ui_game_debug_tap(scene == "zorro" ? "lvl:3" : "lvl:1");
        ui_game_debug_tap("play");
        run(600);
        if (scene == "select") { tap("e2"); }
        else if (scene == "mid") {
            play("e2", "e4"); play("g1", "f3"); play("f1", "c4"); tap("f3");
        } else if (scene == "capture") {
            host.load("rnbqkbnr/ppp2ppp/8/3pp3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq -", true, 0);
            run(300); play("e4", "d5"); play("d8", "d5");
        } else if (scene == "check") {
            host.load("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq -", true, 0);
            run(300);
        } else if (scene == "hint") { ui_game_debug_tap("hint"); run(300); }
        else if (scene == "promo") {
            host.load("8/P6k/8/8/8/8/8/K7 w - -", true, 1);
            run(300); tap("a7"); tap("a8");
        } else if (scene == "danger") {
            host.load("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq -", true, 1);
            run(300); tap("d1"); tap("g4"); run(400);
        } else if (scene == "danger_undo") {
            host.load("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq -", true, 1);
            run(300); tap("d1"); tap("g4"); run(400); ui_game_debug_tap("undo"); run(600);
        } else if (scene == "adapt") {
            host.load("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNB1KBNR w KQkq -", true, 2);
            run(300); play("e2", "e4");
        } else if (scene == "slots" || scene == "slots_confirm" || scene == "slots_loaded") {
            play("e2", "e4"); play("g1", "f3");
            ui_game_debug_tap("slots"); run(200);
            ui_game_debug_tap("save:0"); run(300);                 // hueco vacío: guarda directo
            if (scene == "slots") { /* captura con el hueco 1 ocupado */ }
            if (scene == "slots_confirm") ui_game_debug_tap("save:0");   // sobrescribir pide confirmación
            if (scene == "slots_loaded") {
                ui_game_debug_tap("lvl:4"); ui_game_debug_tap("black"); ui_game_debug_tap("play"); run(800);
                ui_game_debug_tap("slots"); run(200);
                ui_game_debug_tap("save:1"); run(300);             // otra partida (negras, Búho) en el hueco 2
                ui_game_debug_tap("load:0"); run(200); ui_game_debug_tap("load:0"); run(1800);   // cargar pide confirmación
            }
        } else if (scene.rfind("two", 0) == 0) {
            // (la partida de 1 jugador ya empezó arriba; aquí se abre una de 2 jugadores)
            ui_game_debug_tap("new"); ui_game_debug_tap("mode:2");
            ui_game_debug_tap("name:0:Ana"); ui_game_debug_tap("name:1:Luis");
            if (scene == "two_clock" || scene == "two_flag") { ui_game_debug_tap("clock"); ui_game_debug_tap("clock"); }  // 5 min
            ui_game_debug_tap("play"); run(500);
            if (scene == "two") { play("e2", "e4"); play("e7", "e5"); play("g1", "f3"); }
            else if (scene == "two_black") { play("e2", "e4"); play("e7", "e5"); play("g1", "f3"); tap("g8"); }
            else if (scene == "two_white_sel") { play("e2", "e4"); play("e7", "e5"); tap("g1"); }
            else if (scene == "two_slots") {
                play("e2", "e4"); play("e7", "e5"); ui_game_debug_tap("slots"); run(200); ui_game_debug_tap("save:1"); run(400);
            }
            else if (scene == "two_touch") {
                // Toques reales (indev): botón Rendirse girado del jugador de arriba y una pieza negra
                play("e2", "e4");
                click(808, 59);          // centro del botón Rendirse girado: debe pasar a "¿Seguro?"
                click(475, 55);          // casilla g8 (caballo negro): debe seleccionarse
            }
            else if (scene == "two_win") { play("f2", "f3"); play("e7", "e5"); play("g2", "g4"); play("d8", "h4"); run(600); }
            else if (scene == "two_clock") { play("e2", "e4"); run(3000); ui_game_debug_tap("left:b:15"); run(300); }
            else if (scene == "two_flag") { play("e2", "e4"); ui_game_debug_tap("left:b:1"); run(2500); }
            else if (scene == "two_resign") { play("e2", "e4"); ui_game_debug_tap("resign:1"); run(100); ui_game_debug_tap("resign:1"); run(600); }
            else if (scene == "two_promo") { host.load("7k/8/8/8/8/8/p7/6K1 b - -", true, 1); run(300); tap("a2"); tap("a1"); }
            else if (scene == "two_capture") { host.load("rnbqkbnr/ppp2ppp/8/3pp3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq -", true, 1); run(300); play("e4", "d5"); play("d8", "d5"); }
        } else if (scene == "win") {
            host.load("6k1/5ppp/8/8/8/8/8/R5K1 w - -", true, 1);
            run(300); play("a1", "a8"); run(400);
        } else if (scene == "lose") {
            host.load("6k1/5ppp/8/8/8/8/5PPP/R5K1 b - -", false, 5);
            run(300);
        } else if (scene == "black") {
            ui_game_debug_tap("new"); ui_game_debug_tap("black"); ui_game_debug_tap("play"); run(2500);
        }
    }
    run(500);
    save(out);
    return 0;
}
