#include "ui_game.h"
#include "ui_avatar.h"
#include "pieces.h"
#include "ppa_fx.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <initializer_list>

LV_FONT_DECLARE(lv_font_montserrat_14);
LV_FONT_DECLARE(lv_font_montserrat_28);
LV_FONT_DECLARE(lv_font_montserrat_32);
LV_FONT_DECLARE(lv_font_montserrat_42);
LV_FONT_DECLARE(lv_font_montserrat_48);

// ── Geometría (pantalla 1024x600) ─────────────────────────────────────────
static const int SQ = 70;                 // casilla
static const int BX = 20, BY = 20;        // esquina del tablero
static const int PX = 612, PW = 392;      // panel derecho
static const int PIECE_OFF = (SQ - PIECE_PX) / 2;

// ── Paleta ────────────────────────────────────────────────────────────────
static lv_color_t C(uint32_t h) { return lv_color_hex(h); }
static const uint32_t COL_LIGHT = 0xF0D9B5, COL_DARK = 0xB58863;
static const uint32_t COL_LAST_L = 0xD9DE72, COL_LAST_D = 0xB5B445;
static const uint32_t COL_SEL_L = 0x8BE07A, COL_SEL_D = 0x62BE52;
static const uint32_t COL_CHECK = 0xE8554E;
static const uint32_t COL_HINT_L = 0x8CCBF5, COL_HINT_D = 0x5AA4DC;
static const uint32_t COL_DANGER = 0xFF8A3D;

// ── Estado ────────────────────────────────────────────────────────────────
static GameHost *host;
static Snapshot snap, prev;
static bool have_snap;
static lv_obj_t *scr, *board, *frame;
static lv_obj_t *sq_obj[64], *dot_obj[64], *ring_obj[64];
static lv_obj_t *p_obj[64];               // imagen de la pieza mostrada en cada casilla
static int8_t p_type[64];                 // pieza mostrada (código del motor)
static lv_obj_t *file_lbl[8], *rank_lbl[8];
static bool flipped;
static int sel = -1;
static bool target[64];

static lv_obj_t *card, *status_lbl, *speech_lbl, *name_lbl, *spinner;
static Avatar avatar;
static int avatar_level = -1;
static lv_obj_t *tray_cont;
static lv_obj_t *btn_undo, *btn_hint, *btn_new, *btn_slots;
static lv_obj_t *dlg_new, *dlg_promo, *dlg_slots, *banner;
static lv_obj_t *slot_name[SAVE_SLOTS], *slot_detail[SAVE_SLOTS], *slot_av[SAVE_SLOTS], *slot_btn[SAVE_SLOTS][3], *slot_toast;
enum { K_SAVE, K_LOAD, K_DEL };
static int armed_slot = -1, armed_kind = -1;
static lv_timer_t *armed_timer, *toast_timer;
static lv_obj_t *lvl_card[LEVEL_COUNT];
static lv_obj_t *col_btn[2], *coach_btn, *adapt_btn;
static lv_obj_t *warn_card, *warn_title, *warn_sub, *lvl_dot[LEVEL_COUNT];
static bool dlg_coach = true, dlg_adapt = true;
static int dlg_level = 1;
static bool dlg_white = true;
static int promo_from = -1, promo_to = -1;
static bool banner_shown;

// Modo dos jugadores
static bool s_two;                          // modo que muestra la pantalla ahora mismo
struct SidePanel { lv_obj_t *root, *dot, *name, *status, *tray, *resign; };
static SidePanel side[2];                   // 0 = blancas (abajo), 1 = negras (arriba, girado 180°)
static lv_obj_t *mid_strip, *mid_undo, *status_pill, *banner2;
static int resign_armed = -1;
static lv_timer_t *resign_timer;

// Diálogo de nueva partida: modo, nombres y editor de nombres con teclado
static bool dlg_two;
static char dlg_n1[NAME_LEN], dlg_n2[NAME_LEN];
static lv_obj_t *grp_one, *grp_two, *mode_btn[2], *name_btn_1p, *name_btn_2p[2];
static lv_obj_t *dlg_name, *name_ta, *name_title, *name_kb;
static int name_target;
static bool kb_upper = true;

static void paint_dialog();
static void refresh_dialog();
static void update_buttons();
static void set_btn_text(lv_obj_t *b, const char *txt);
static void open_name_editor(int target);

// ── Utilidades ────────────────────────────────────────────────────────────
static inline int sq_col(int s) { return s & 7; }
static inline int sq_row(int s) { return s >> 3; }
static int disp_x(int s) { return (flipped ? 7 - sq_col(s) : sq_col(s)) * SQ; }
static int disp_y(int s) { return (flipped ? 7 - sq_row(s) : sq_row(s)) * SQ; }
static bool is_light(int s) { return ((sq_col(s) + sq_row(s)) & 1) == 0; }

static lv_obj_t *mk(lv_obj_t *parent, int x, int y, int w, int h, uint32_t col, int radius = 0, int opa = 255) {
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, C(col), 0);
    lv_obj_set_style_bg_opa(o, (lv_opa_t)opa, 0);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

static lv_obj_t *label(lv_obj_t *parent, const char *txt, const lv_font_t *font, uint32_t col, int x, int y) {
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, C(col), 0);
    lv_obj_set_pos(l, x, y);
    return l;
}

static lv_obj_t *button(lv_obj_t *parent, const char *txt, uint32_t col, int x, int y, int w, int h, lv_event_cb_t cb, void *ud = nullptr, const lv_font_t *font = nullptr) {
    lv_obj_t *b = mk(parent, x, y, w, h, col, 22);
    lv_obj_set_style_bg_grad_color(b, lv_color_darken(C(col), 35), 0);
    lv_obj_set_style_bg_grad_dir(b, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_shadow_width(b, 14, 0);
    lv_obj_set_style_shadow_opa(b, LV_OPA_40, 0);
    lv_obj_set_style_shadow_offset_y(b, 5, 0);
    lv_obj_set_style_shadow_color(b, C(0x000000), 0);
    lv_obj_set_style_translate_y(b, 4, LV_STATE_PRESSED);
    lv_obj_set_style_opa(b, LV_OPA_50, LV_STATE_DISABLED);
    lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_font(l, font ? font : &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(l, C(0xFFFFFF), 0);
    lv_obj_center(l);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, ud);
    return b;
}

static void anim_x(void *o, int32_t v) { lv_obj_set_x((lv_obj_t *)o, v); }
static void anim_y(void *o, int32_t v) { lv_obj_set_y((lv_obj_t *)o, v); }
static void anim_opa(void *o, int32_t v) { lv_obj_set_style_opa((lv_obj_t *)o, (lv_opa_t)v, 0); }
static void anim_del_cb(lv_anim_t *a) { lv_obj_delete_async((lv_obj_t *)a->var); }

static void run_anim(lv_obj_t *o, lv_anim_exec_xcb_t cb, int from, int to, int ms, lv_anim_path_cb_t path, int delay = 0, bool del_end = false) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, o);
    lv_anim_set_exec_cb(&a, cb);
    lv_anim_set_values(&a, from, to);
    lv_anim_set_duration(&a, ms);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_path_cb(&a, path);
    if (del_end) lv_anim_set_completed_cb(&a, anim_del_cb);
    lv_anim_start(&a);
}

// ── Piezas ────────────────────────────────────────────────────────────────
// En el modo cara a cara las negras se dibujan giradas 180° para que las vea derechas quien juega desde arriba.
static const lv_image_dsc_t *sprite(int8_t p) {
    int i = piece_index(p);
    return (s_two && p < 0) ? piece_rot180(i) : &piece_sprites[i];
}

static lv_obj_t *new_piece(int s, int8_t p) {
    lv_obj_t *im = lv_image_create(board);
    lv_image_set_src(im, sprite(p));
    lv_obj_set_pos(im, disp_x(s) + PIECE_OFF, disp_y(s) + PIECE_OFF);
    lv_obj_remove_flag(im, LV_OBJ_FLAG_CLICKABLE);
    return im;
}

static void drop_piece(int s, bool fade) {
    lv_obj_t *o = p_obj[s];
    p_obj[s] = nullptr;
    p_type[s] = 0;
    if (!o) return;
    if (fade) run_anim(o, anim_opa, 255, 0, 260, lv_anim_path_ease_out, 0, true);
    else lv_obj_delete(o);
}

static void unlift(int s) {
    if (s < 0 || !p_obj[s]) return;
    lv_image_set_src(p_obj[s], sprite(p_type[s]));
    lv_image_set_scale(p_obj[s], 256);
    lv_obj_set_pos(p_obj[s], disp_x(s) + PIECE_OFF, disp_y(s) + PIECE_OFF);
}

static void lift(int s) {
    if (s < 0 || !p_obj[s]) return;
    const lv_image_dsc_t *big = (s_two && p_type[s] < 0) ? nullptr : ppa_fx_lifted(piece_index(p_type[s]));  // el levantado del PPA no está girado
    if (big) {  // sprite escalado por el PPA al arrancar
        lv_image_set_src(p_obj[s], big);
        lv_obj_set_pos(p_obj[s], disp_x(s) + (SQ - big->header.w) / 2, disp_y(s) + (SQ - big->header.h) / 2 - 4);
    } else {    // respaldo: escala por software
        lv_image_set_pivot(p_obj[s], PIECE_PX / 2, PIECE_PX / 2);
        lv_image_set_scale(p_obj[s], 320);
        lv_obj_set_pos(p_obj[s], disp_x(s) + PIECE_OFF, disp_y(s) + PIECE_OFF - 4);
    }
    lv_obj_move_foreground(p_obj[s]);
}

static void move_piece(int from, int to, int8_t shown, bool animate) {
    lv_obj_t *o = p_obj[from];
    if (!o) return;
    if (p_obj[to]) drop_piece(to, true);
    p_obj[to] = o;
    p_type[to] = shown;
    p_obj[from] = nullptr;
    p_type[from] = 0;
    lv_image_set_src(o, sprite(shown));
    lv_image_set_scale(o, 256);
    lv_obj_move_foreground(o);
    int tx = disp_x(to) + PIECE_OFF, ty = disp_y(to) + PIECE_OFF;
    if (animate) {
        run_anim(o, anim_x, lv_obj_get_x(o), tx, 280, lv_anim_path_ease_out);
        run_anim(o, anim_y, lv_obj_get_y(o), ty, 280, lv_anim_path_ease_out);
    } else {
        lv_obj_set_pos(o, tx, ty);
    }
}

static void sync_pieces(bool animate) {
    if (animate && snap.hasLast && p_obj[snap.last.from]) {
        int f = snap.last.from, t = snap.last.to;
        int8_t mover = p_type[f];
        move_piece(f, t, snap.board[t], true);
        if (abs(mover) == 6 && abs(sq_col(t) - sq_col(f)) == 2) {  // enroque: mueve también la torre
            int row = sq_row(t), rf = row * 8 + (t > f ? 7 : 0), rt = row * 8 + (t > f ? 5 : 3);
            move_piece(rf, rt, snap.board[rt], true);
        }
    }
    for (int s = 0; s < 64; s++) {
        if (p_type[s] == snap.board[s]) continue;
        if (p_obj[s]) drop_piece(s, false);
        if (snap.board[s]) {
            p_obj[s] = new_piece(s, snap.board[s]);
            p_type[s] = snap.board[s];
        }
    }
}

static void relayout() {
    for (int s = 0; s < 64; s++) {
        lv_obj_set_pos(sq_obj[s], disp_x(s), disp_y(s));
        if (p_obj[s]) lv_obj_set_pos(p_obj[s], disp_x(s) + PIECE_OFF, disp_y(s) + PIECE_OFF);
    }
    for (int i = 0; i < 8; i++) {
        char t[2] = {(char)('a' + (flipped ? 7 - i : i)), 0};
        lv_label_set_text(file_lbl[i], t);
        char r[2] = {(char)('1' + (flipped ? i : 7 - i)), 0};
        lv_label_set_text(rank_lbl[i], r);
    }
}

// ── Casillas ──────────────────────────────────────────────────────────────
static int king_square(bool white) {
    for (int s = 0; s < 64; s++)
        if (snap.board[s] == (white ? 6 : -6)) return s;
    return -1;
}

static void paint_squares() {
    int checked = (snap.status == ce::ST_CHECK || snap.status == ce::ST_MATE) ? king_square(snap.whiteToMove) : -1;
    for (int s = 0; s < 64; s++) {
        bool l = is_light(s);
        uint32_t col = l ? COL_LIGHT : COL_DARK;
        if (snap.hasLast && (s == snap.last.from || s == snap.last.to)) col = l ? COL_LAST_L : COL_LAST_D;
        if (snap.hasHint && (s == snap.hint.from || s == snap.hint.to)) col = l ? COL_HINT_L : COL_HINT_D;
        if (s == sel) col = l ? COL_SEL_L : COL_SEL_D;
        if (s == checked) col = COL_CHECK;
        bool danger = snap.awaiting && s == snap.dangerSq;
        if (danger) col = COL_DANGER;
        lv_obj_set_style_bg_color(sq_obj[s], C(col), 0);
        lv_obj_set_style_border_color(ring_obj[s], C(danger ? 0xD32F2F : 0x2E7D32), 0);
        bool cap = (target[s] && snap.board[s] != 0) || danger;
        if (target[s] && !cap) lv_obj_remove_flag(dot_obj[s], LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(dot_obj[s], LV_OBJ_FLAG_HIDDEN);
        if (cap) lv_obj_remove_flag(ring_obj[s], LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(ring_obj[s], LV_OBJ_FLAG_HIDDEN);
    }
}

static void select_square(int s) {
    unlift(sel);
    sel = s;
    memset(target, 0, sizeof target);
    if (s >= 0) {
        for (int i = 0; i < snap.nLegal; i++)
            if (snap.legal[i].from == s) target[snap.legal[i].to] = true;
        lift(s);
    }
    paint_squares();
}

// ── Mascota, estado y bandeja ─────────────────────────────────────────────
static const char *pick(const char *const *v, int n) { return v[lv_rand(0, n - 1)]; }

static void set_speech(const char *t, Mood m) {
    lv_label_set_text(speech_lbl, t);
    avatar_set_mood(avatar, m);
}

static void build_avatar(int level) {
    if (avatar_level == level) return;
    avatar_level = level;
    if (avatar.root) lv_obj_delete(avatar.root);
    avatar = avatar_create(card, level, 124);
    lv_obj_set_pos(avatar.root, 14, 26);
    const Level &lv = level_info(level);
    lv_label_set_text(name_lbl, lv.name);
}

static bool human_turn() { return have_snap && !snap.over && !snap.awaiting && (snap.twoPlayer || snap.whiteToMove == snap.humanWhite); }
static bool game_over() { return have_snap && snap.over; }
// Nombre mostrado de cada bando en dos jugadores
static const char *pname(int i) {
    const char *n = i == 0 ? snap.name1 : snap.name2;
    return n[0] ? n : (i == 0 ? "Blancas" : "Negras");
}
static bool dialogs_closed() {
    return lv_obj_has_flag(dlg_new, LV_OBJ_FLAG_HIDDEN) && lv_obj_has_flag(dlg_promo, LV_OBJ_FLAG_HIDDEN) &&
           lv_obj_has_flag(dlg_slots, LV_OBJ_FLAG_HIDDEN) && lv_obj_has_flag(dlg_name, LV_OBJ_FLAG_HIDDEN);
}
static bool interactive() { return human_turn() && dialogs_closed(); }

// Fila de piezas capturadas (dama ... peón) con su contador; devuelve la x final.
static int tray_row(lv_obj_t *parent, int x, int y, const uint8_t *miss, bool white_pieces) {
    for (int t = 4; t >= 0; t--) {
        if (!miss[t]) continue;
        int idx = (white_pieces ? 0 : 6) + t;
        lv_obj_t *im = lv_image_create(parent);
        const lv_image_dsc_t *mini = ppa_fx_mini(idx);
        if (mini) {
            lv_image_set_src(im, mini);
        } else {
            lv_image_set_src(im, &piece_sprites[idx]);
            lv_image_set_pivot(im, 0, 0);
            lv_image_set_scale(im, 128);
        }
        lv_obj_set_pos(im, x, y);
        x += 32;
        if (miss[t] > 1) {
            char b[8];
            snprintf(b, sizeof b, "x%d", miss[t]);
            label(parent, b, &lv_font_montserrat_14, 0xFFFFFF, x, y + 22);
            x += 24;
        }
        x += 6;
    }
    return x;
}

static void rebuild_tray() {
    lv_obj_clean(tray_cont);
    // Insignia del jugador: su inicial si puso nombre, "Tú" si no
    char ini[8] = "Tú";
    if (snap.name1[0]) {
        size_t n = 1;
        while ((snap.name1[n] & 0xC0) == 0x80) n++;   // primer carácter UTF-8
        memcpy(ini, snap.name1, n);
        ini[n] = 0;
    }
    lv_obj_t *me = mk(tray_cont, 4, 8, 50, 50, 0x3F8CE6, LV_RADIUS_CIRCLE);
    lv_obj_t *me_l = label(me, ini, &lv_font_montserrat_28, 0xFFFFFF, 0, 0);
    lv_obj_center(me_l);
    Avatar cpu = avatar_create(tray_cont, snap.level, 50);                    // y la mascota del rival
    lv_obj_set_pos(cpu.root, 4, 68);
    for (int row = 0; row < 2; row++) {
        const uint8_t *miss = (row == 0) == snap.humanWhite ? snap.missingBlack : snap.missingWhite;  // lo que falta al otro bando
        tray_row(tray_cont, 66, 14 + row * 60, miss, !((row == 0) == snap.humanWhite));
    }
}

static const char *const S_THINK[] = {"Hmm... déjame pensar", "Veamos qué hago...", "Estoy pensando..."};
static const char *const S_TURN[] = {"¡Te toca!", "¡Tu turno!", "¿Qué jugarás?"};
static const char *const S_CAPTURE_ME[] = {"¡Ñam! Me la comí", "¡Gracias por la pieza!"};
static const char *const S_CAPTURE_YOU[] = {"¡Ay, mi pieza!", "¡Buena captura!"};
static const char *const S_CHECK_HUMAN[] = {"¡Jaque! Cuida tu rey", "¡Cuidado con tu rey!"};
static const char *const S_CHECK_CPU[] = {"¡Uy, mi rey está en peligro!", "¡Me has dado jaque!"};

static void update_status(bool cap_by_cpu, bool cap_by_human) {
    const Level &lv = level_info(snap.level);
    char buf[64];
    bool human_on_move = snap.whiteToMove == snap.humanWhite;
    if (game_over()) {
        lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_x(status_lbl, 24);
        switch (snap.status) {
            case ce::ST_MATE:
                if (human_on_move) { snprintf(buf, sizeof buf, "¡Jaque mate! Perdiste"); set_speech("¡Gané! ¿Revancha?", Mood::Happy); }
                else { snprintf(buf, sizeof buf, "¡Jaque mate! ¡Ganaste!"); set_speech("¡Qué bien jugaste!", Mood::Sad); }
                break;
            case ce::ST_STALEMATE: snprintf(buf, sizeof buf, "Tablas: rey ahogado"); set_speech("¡Quedamos empate!", Mood::Surprised); break;
            case ce::ST_DRAW_REPETITION: snprintf(buf, sizeof buf, "Tablas: repetición"); set_speech("¡Quedamos empate!", Mood::Surprised); break;
            case ce::ST_DRAW_50: snprintf(buf, sizeof buf, "Tablas: 50 jugadas"); set_speech("¡Quedamos empate!", Mood::Surprised); break;
            default: snprintf(buf, sizeof buf, "Tablas: sin material"); set_speech("¡Quedamos empate!", Mood::Surprised); break;
        }
        lv_label_set_text(status_lbl, buf);
        return;
    }
    if (snap.awaiting) {  // aviso de pieza en peligro: la CPU espera la decisión
        static const char *const NAME[] = {"", "el peón", "el caballo", "el alfil", "la torre", "la dama", "el rey"};
        int pc = snap.board[snap.dangerSq];
        pc = pc < 0 ? -pc : pc;
        lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_x(status_lbl, 24);
        lv_label_set_text(status_lbl, "¡Pieza en peligro!");
        snprintf(buf, sizeof buf, "¡Ojo! Pueden comerte %s", NAME[pc > 6 ? 0 : pc]);
        set_speech(buf, Mood::Surprised);
        return;
    }
    if (!human_on_move) {  // piensa la CPU
        lv_obj_remove_flag(spinner, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_x(status_lbl, 64);
        snprintf(buf, sizeof buf, "Piensa el %s...", lv.name);
        lv_label_set_text(status_lbl, buf);
        if (snap.status == ce::ST_CHECK) set_speech(pick(S_CHECK_CPU, 2), Mood::Surprised);
        else if (cap_by_human) set_speech(pick(S_CAPTURE_YOU, 2), Mood::Surprised);
        else set_speech(pick(S_THINK, 3), Mood::Thinking);
    } else {
        lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_x(status_lbl, 24);
        if (snap.adaptEvent < 0) { lv_label_set_text(status_lbl, "Tu turno"); set_speech("¡Voy más despacio!", Mood::Happy); }
        else if (snap.adaptEvent > 0) { lv_label_set_text(status_lbl, "Tu turno"); set_speech("¡Ahora me esfuerzo más!", Mood::Normal); }
        else if (snap.status == ce::ST_CHECK) { lv_label_set_text(status_lbl, "¡Estás en jaque!"); set_speech(pick(S_CHECK_HUMAN, 2), Mood::Happy); }
        else {
            lv_label_set_text(status_lbl, "Tu turno");
            set_speech(cap_by_cpu ? pick(S_CAPTURE_ME, 2) : pick(S_TURN, 3), cap_by_cpu ? Mood::Happy : Mood::Normal);
        }
    }
}

static void set_btn_text(lv_obj_t *b, const char *txt) { lv_label_set_text(lv_obj_get_child(b, 0), txt); }

static void update_dots() {
    for (int i = 0; i < LEVEL_COUNT; i++)
        lv_obj_set_style_bg_color(lvl_dot[i], C(i <= snap.effLevel ? (snap.adaptive && snap.effLevel != snap.level ? 0x4DD0E1 : 0xFFB300) : 0x4A4780), 0);
}

static void set_enabled(lv_obj_t *b, bool on) { if (on) lv_obj_remove_state(b, LV_STATE_DISABLED); else lv_obj_add_state(b, LV_STATE_DISABLED); }

static void update_buttons() {
    bool dlg_closed = dialogs_closed();
    if (s_two) {  // dos jugadores: deshacer (cualquiera) y rendirse (cada uno)
        set_enabled(mid_undo, dlg_closed && snap.plies > 0);
        for (int i = 0; i < 2; i++) set_enabled(side[i].resign, dlg_closed && !snap.over && snap.plies > 0);
        return;
    }
    if (snap.awaiting) {  // aviso: "Deshacer" retira la jugada, "Seguir" deja que la CPU responda
        set_btn_text(btn_undo, "Deshacer");
        set_btn_text(btn_hint, "Seguir");
        lv_obj_remove_flag(warn_card, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_state(btn_undo, LV_STATE_DISABLED);
        if (dlg_closed) lv_obj_remove_state(btn_hint, LV_STATE_DISABLED);
        return;
    }
    set_btn_text(btn_hint, "Pista");
    lv_obj_add_flag(warn_card, LV_OBJ_FLAG_HIDDEN);
    bool can = human_turn() && dlg_closed;
    set_enabled(btn_undo, can && snap.plies > 0);
    set_enabled(btn_hint, can);
}

// ── Confeti y banner de fin ───────────────────────────────────────────────
static void confetti() {
    static const uint32_t cols[] = {0xFF5C5C, 0xFFD23F, 0x4CD964, 0x4DB5FF, 0xC07CFF, 0xFF8FD0};
    for (int i = 0; i < 44; i++) {
        lv_obj_t *c = mk(lv_layer_top(), lv_rand(0, 1000), -30, lv_rand(8, 16), lv_rand(12, 22), cols[lv_rand(0, 5)], 3);
        lv_obj_set_style_transform_rotation(c, lv_rand(0, 3500), 0);
        lv_obj_remove_flag(c, LV_OBJ_FLAG_CLICKABLE);
        run_anim(c, anim_y, -30, 620, lv_rand(1800, 3200), lv_anim_path_ease_in, lv_rand(0, 900), true);
    }
}

static void banner_cb(lv_event_t *) {
    lv_obj_add_flag(banner, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(banner2, LV_OBJ_FLAG_HIDDEN);
}

static void show_banner(const char *title, const char *sub, uint32_t col) {
    for (lv_obj_t *b : {banner, banner2}) {
        lv_obj_t *t = lv_obj_get_child(b, 0), *s = lv_obj_get_child(b, 1);
        lv_label_set_text(t, title);
        lv_label_set_text(s, sub);
        lv_obj_set_style_text_font(t, strlen(title) > 14 ? &lv_font_montserrat_42 : &lv_font_montserrat_48, 0);
        lv_obj_set_style_text_color(t, C(col), 0);
        if (b == banner || s_two) { lv_obj_remove_flag(b, LV_OBJ_FLAG_HIDDEN); lv_obj_move_foreground(b); }
    }
}

static void end_effects() {
    if (banner_shown) return;
    banner_shown = true;
    char title[64], sub[64];
    if (s_two) {  // gana un nombre, no "tú"
        int win = -1;                                  // 0 blancas, 1 negras, -1 tablas
        if (snap.resigned) win = snap.resigned == 1 ? 1 : 0;
        else if (snap.status == ce::ST_MATE) win = snap.whiteToMove ? 1 : 0;
        if (win >= 0) {
            snprintf(title, sizeof title, "¡Ganó %s!", pname(win));
            snprintf(sub, sizeof sub, snap.resigned ? "%s se ha rendido" : "¡Jaque mate!", pname(1 - win));
            show_banner(title, sub, 0xFFD23F);
            confetti();
        } else {
            show_banner("¡Tablas!", "Nadie ha ganado", 0x9BD3FF);
        }
        return;
    }
    bool human_on_move = snap.whiteToMove == snap.humanWhite;
    if (snap.status == ce::ST_MATE && !human_on_move) { show_banner("¡Ganaste!", "¡Jaque mate! Toca para cerrar", 0xFFD23F); confetti(); }
    else if (snap.status == ce::ST_MATE) show_banner("Casi...", "Esta vez ganó el rival. ¡Otra!", 0xFF8F8F);
    else show_banner("¡Tablas!", "Nadie ha ganado. Toca para cerrar", 0x9BD3FF);
}

// ── Acciones ──────────────────────────────────────────────────────────────
static void send_move(int from, int to, char promo) {
    ce::Move m{(int8_t)from, (int8_t)to, promo};
    select_square(-1);
    host->post_human_move(m);
}

static void on_square(lv_event_t *e) {
    int s = (int)(intptr_t)lv_event_get_user_data(e);
    ui_game_debug_tap_square(s);
}

static void promo_choose(char p) {
    lv_obj_add_flag(dlg_promo, LV_OBJ_FLAG_HIDDEN);
    send_move(promo_from, promo_to, p);
}

static void open_promo(int from, int to) {
    promo_from = from;
    promo_to = to;
    bool w = s_two ? snap.whiteToMove : snap.humanWhite;   // en dos jugadores promociona quien tiene el turno
    lv_obj_set_style_transform_pivot_x(dlg_promo, 512, 0);
    lv_obj_set_style_transform_pivot_y(dlg_promo, 300, 0);
    lv_obj_set_style_transform_rotation(dlg_promo, (s_two && !w) ? 1800 : 0, 0);   // las negras lo ven derecho desde arriba
    lv_obj_t *row = lv_obj_get_child(dlg_promo, 1);
    static const char ps[4] = {'q', 'r', 'b', 'n'};
    static const int type[4] = {5, 4, 3, 2};
    for (int i = 0; i < 4; i++) {
        lv_obj_t *c = lv_obj_get_child(row, i);
        lv_obj_t *im = lv_obj_get_child(c, 0);
        lv_image_set_src(im, &piece_sprites[(w ? 0 : 6) + type[i] - 1]);
        (void)ps;
    }
    lv_obj_remove_flag(dlg_promo, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(dlg_promo);
}

static void toggle_style(lv_obj_t *b, bool on);

static void paint_dialog() {
    for (int i = 0; i < 2; i++) toggle_style(mode_btn[i], (i == 1) == dlg_two);
    if (dlg_two) { lv_obj_add_flag(grp_one, LV_OBJ_FLAG_HIDDEN); lv_obj_remove_flag(grp_two, LV_OBJ_FLAG_HIDDEN); }
    else         { lv_obj_remove_flag(grp_one, LV_OBJ_FLAG_HIDDEN); lv_obj_add_flag(grp_two, LV_OBJ_FLAG_HIDDEN); }
    for (int i = 0; i < LEVEL_COUNT; i++) {
        lv_obj_set_style_border_color(lvl_card[i], C(i == dlg_level ? 0xFFB300 : 0x4A4780), 0);
        lv_obj_set_style_border_width(lvl_card[i], i == dlg_level ? 6 : 2, 0);
    }
    for (int i = 0; i < 2; i++) toggle_style(col_btn[i], (i == 0) == dlg_white);
    toggle_style(coach_btn, dlg_coach);
    toggle_style(adapt_btn, dlg_adapt);
    set_btn_text(coach_btn, dlg_coach ? "Avisos: Sí" : "Avisos: No");
    set_btn_text(adapt_btn, dlg_adapt ? "Adaptable: Sí" : "Adaptable: No");
    set_btn_text(name_btn_1p, dlg_n1[0] ? dlg_n1 : "Mi nombre");
    for (int i = 0; i < 2; i++) {
        const char *n = i == 0 ? dlg_n1 : dlg_n2;
        lv_obj_t *l = lv_obj_get_child(name_btn_2p[i], 0);
        lv_label_set_text(l, n[0] ? n : "Toca para escribir el nombre");
        lv_obj_set_style_text_font(l, n[0] ? &lv_font_montserrat_42 : &lv_font_montserrat_28, 0);
        lv_obj_set_style_text_color(l, C(n[0] ? 0xFFFFFF : 0xB8B4DD), 0);
        lv_obj_center(l);
    }
}

static void refresh_dialog() {
    paint_dialog();
    lv_obj_remove_flag(dlg_new, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(dlg_new);
    update_buttons();
}

static void open_new_dialog() { refresh_dialog(); }

static void start_game() {
    lv_obj_add_flag(dlg_new, LV_OBJ_FLAG_HIDDEN);
    banner_cb(nullptr);
    banner_shown = false;
    select_square(-1);
    have_snap = false;
    NewGameOpts o;
    o.humanWhite = dlg_white;
    o.level = dlg_level;
    o.coach = dlg_coach;
    o.adaptive = dlg_adapt;
    o.twoPlayer = dlg_two;
    name_sanitize(o.name1, dlg_n1);
    name_sanitize(o.name2, dlg_n2);
    host->post_new(o);
}

static void on_new(lv_event_t *) { host->stop_search(); open_new_dialog(); }
static void on_undo(lv_event_t *) { if (human_turn() || snap.awaiting || (snap.twoPlayer && snap.plies > 0)) { select_square(-1); host->post_undo(); } }
static void on_hint(lv_event_t *) {
    if (snap.awaiting) host->post_confirm();
    else if (human_turn()) host->post_hint();
}
static void on_play(lv_event_t *) { start_game(); }
static void on_level(lv_event_t *e) { dlg_level = (int)(intptr_t)lv_event_get_user_data(e); paint_dialog(); }
static void on_mode(lv_event_t *e) { dlg_two = (intptr_t)lv_event_get_user_data(e) == 1; paint_dialog(); }
static void on_name_btn(lv_event_t *e) { open_name_editor((int)(intptr_t)lv_event_get_user_data(e)); }
static void on_color(lv_event_t *e) { dlg_white = (intptr_t)lv_event_get_user_data(e) == 0; paint_dialog(); }
static void on_promo(lv_event_t *e) { promo_choose("qrbn"[(intptr_t)lv_event_get_user_data(e)]); }

static void on_slots_open(lv_event_t *);

// ── Construcción ──────────────────────────────────────────────────────────
static void build_board() {
    frame = mk(scr, 4, 4, 592, 592, 0x5A3A22, 22);
    lv_obj_set_style_bg_grad_color(frame, C(0x3E2614), 0);
    lv_obj_set_style_bg_grad_dir(frame, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_shadow_width(frame, 30, 0);
    lv_obj_set_style_shadow_opa(frame, LV_OPA_50, 0);
    lv_obj_set_style_shadow_offset_y(frame, 8, 0);
    lv_obj_set_style_shadow_color(frame, C(0x000000), 0);
    lv_obj_set_style_border_width(frame, 2, 0);
    lv_obj_set_style_border_color(frame, C(0x7A5233), 0);

    board = mk(scr, BX, BY, 8 * SQ, 8 * SQ, 0x000000, 4, 0);
    for (int s = 0; s < 64; s++) {
        sq_obj[s] = mk(board, disp_x(s), disp_y(s), SQ, SQ, is_light(s) ? COL_LIGHT : COL_DARK);
        lv_obj_add_flag(sq_obj[s], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(sq_obj[s], on_square, LV_EVENT_CLICKED, (void *)(intptr_t)s);
    }
    for (int s = 0; s < 64; s++) {  // marcas de jugadas legales (hijas de la casilla)
        dot_obj[s] = mk(sq_obj[s], 24, 24, 22, 22, 0x2E7D32, LV_RADIUS_CIRCLE, 150);
        lv_obj_add_flag(dot_obj[s], LV_OBJ_FLAG_HIDDEN);
        ring_obj[s] = mk(sq_obj[s], 3, 3, 64, 64, 0x000000, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(ring_obj[s], 7, 0);
        lv_obj_set_style_border_color(ring_obj[s], C(0x2E7D32), 0);
        lv_obj_set_style_border_opa(ring_obj[s], LV_OPA_60, 0);
        lv_obj_add_flag(ring_obj[s], LV_OBJ_FLAG_HIDDEN);
    }
    for (int i = 0; i < 8; i++) {  // coordenadas sobre el marco
        file_lbl[i] = label(scr, "a", &lv_font_montserrat_14, 0xE6D3B3, BX + i * SQ + SQ / 2 - 4, BY + 8 * SQ + 1);
        rank_lbl[i] = label(scr, "1", &lv_font_montserrat_14, 0xE6D3B3, 7, BY + i * SQ + SQ / 2 - 8);
    }
    relayout();
}

static void build_panel() {
    card = mk(scr, PX, 8, PW, 188, 0x2D2A5A, 28);
    lv_obj_set_style_bg_grad_color(card, C(0x221F47), 0);
    lv_obj_set_style_bg_grad_dir(card, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_color(card, C(0x4A4780), 0);
    name_lbl = label(card, "", &lv_font_montserrat_42, 0xFFFFFF, 160, 14);
    for (int i = 0; i < LEVEL_COUNT; i++)  // nivel efectivo de la CPU (cambia en el modo adaptable)
        lvl_dot[i] = mk(card, 18 + i * 21, 164, 14, 14, 0x4A4780, LV_RADIUS_CIRCLE);
    // Bocadillo
    lv_obj_t *bub = mk(card, 152, 70, 226, 104, 0xFFFFFF, 20);
    speech_lbl = label(bub, "", &lv_font_montserrat_28, 0x2D2A5A, 12, 10);
    lv_obj_set_width(speech_lbl, 202);
    lv_label_set_long_mode(speech_lbl, LV_LABEL_LONG_WRAP);

    lv_obj_t *pill = status_pill = mk(scr, PX, 206, PW, 62, 0x1B1940, 31);
    spinner = lv_spinner_create(pill);
    lv_spinner_set_anim_params(spinner, 1000, 200);
    lv_obj_set_size(spinner, 40, 40);
    lv_obj_set_pos(spinner, 12, 11);
    lv_obj_set_style_arc_width(spinner, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_width(spinner, 6, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spinner, C(0x35336B), LV_PART_MAIN);
    lv_obj_set_style_arc_color(spinner, C(0xFFB300), LV_PART_INDICATOR);
    lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
    status_lbl = label(pill, "", &lv_font_montserrat_28, 0xFFFFFF, 64, 14);
    lv_obj_set_width(status_lbl, PW - 40);
    lv_label_set_long_mode(status_lbl, LV_LABEL_LONG_CLIP);

    tray_cont = mk(scr, PX, 278, PW, 124, 0x000000, 0, 0);

    warn_card = mk(scr, PX, 278, PW, 124, 0xE8641B, 26);
    lv_obj_set_style_bg_grad_color(warn_card, C(0xC0392B), 0);
    lv_obj_set_style_bg_grad_dir(warn_card, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(warn_card, 3, 0);
    lv_obj_set_style_border_color(warn_card, C(0xFFD23F), 0);
    warn_title = label(warn_card, "¡Cuidado!", &lv_font_montserrat_42, 0xFFFFFF, 18, 8);
    warn_sub = label(warn_card, "¿Deshacer o seguir?", &lv_font_montserrat_28, 0xFFF1C1, 18, 70);
    lv_obj_add_flag(warn_card, LV_OBJ_FLAG_HIDDEN);

    int bw = (PW - 16) / 2;
    btn_undo = button(scr, "Deshacer", 0x3F8CE6, PX, 410, bw, 82, [](lv_event_t *e) { on_undo(e); });
    btn_hint = button(scr, "Pista", 0x2FB67C, PX + bw + 16, 410, bw, 82, [](lv_event_t *e) { on_hint(e); });
    btn_new = button(scr, "Nueva", 0xF08A24, PX, 508, bw, 82, [](lv_event_t *e) { on_new(e); });
    btn_slots = button(scr, "Partidas", 0x8E6CD8, PX + bw + 16, 508, bw, 82, [](lv_event_t *e) { on_slots_open(e); });
}

static void toggle_style(lv_obj_t *b, bool on) {
    lv_obj_set_style_bg_color(b, C(on ? 0xFFB300 : 0x4A4780), 0);
    lv_obj_set_style_bg_grad_color(b, C(on ? 0xE08F00 : 0x35336B), 0);
}

static void group_box(lv_obj_t **out, lv_obj_t *parent) {
    *out = mk(parent, 0, 0, 976, 584, 0x000000, 0, 0);
    lv_obj_remove_flag(*out, LV_OBJ_FLAG_CLICKABLE);
}

static void build_new_dialog() {
    dlg_new = mk(lv_layer_top(), 0, 0, 1024, 600, 0x0A0820, 0, 215);
    lv_obj_add_flag(dlg_new, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *p = mk(dlg_new, 24, 8, 976, 584, 0x2D2A5A, 36);
    lv_obj_set_style_border_width(p, 3, 0);
    lv_obj_set_style_border_color(p, C(0x4A4780), 0);
    mode_btn[0] = button(p, "Contra la máquina", 0xFFB300, 38, 12, 440, 60, on_mode, (void *)0);
    mode_btn[1] = button(p, "Dos jugadores", 0x4A4780, 498, 12, 440, 60, on_mode, (void *)1);

    // — Un jugador: mascota, color, nombre y ayudas —
    group_box(&grp_one, p);
    for (int i = 0; i < LEVEL_COUNT; i++) {
        int cx = 38 + (i % 3) * 308, cy = 84 + (i / 3) * 158;
        lv_obj_t *c = mk(grp_one, cx, cy, 292, 146, 0x221F47, 26);
        lv_obj_set_style_border_width(c, 2, 0);
        lv_obj_set_style_border_color(c, C(0x4A4780), 0);
        lv_obj_add_flag(c, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(c, on_level, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        Avatar a = avatar_create(c, i, 92);
        lv_obj_set_pos(a.root, 14, 26);
        lv_obj_remove_flag(a.root, LV_OBJ_FLAG_CLICKABLE);
        const Level &lv = level_info(i);
        label(c, lv.name, &lv_font_montserrat_32, 0xFFFFFF, 124, 14);
        for (int k = 0; k < 5; k++)  // puntos de dificultad
            mk(c, 126 + k * 26, 64, 18, 18, k <= i * 4 / 5 ? 0xFFB300 : 0x4A4780, LV_RADIUS_CIRCLE);
        lv_obj_t *tg = label(c, lv.tagline, &lv_font_montserrat_14, 0xB8B4DD, 124, 94);
        lv_obj_set_width(tg, 160);
        lvl_card[i] = c;
    }
    label(grp_one, "Juego con:", &lv_font_montserrat_28, 0xB8B4DD, 38, 414);
    col_btn[0] = button(grp_one, "Blancas", 0xFFB300, 224, 400, 180, 58, on_color, (void *)0);
    col_btn[1] = button(grp_one, "Negras", 0x4A4780, 416, 400, 180, 58, on_color, (void *)1);
    name_btn_1p = button(grp_one, "", 0x3F8CE6, 616, 400, 322, 58, on_name_btn, (void *)0);
    coach_btn = button(grp_one, "", 0xFFB300, 38, 470, 296, 64, [](lv_event_t *) { dlg_coach = !dlg_coach; paint_dialog(); });
    adapt_btn = button(grp_one, "", 0xFFB300, 346, 470, 316, 64, [](lv_event_t *) { dlg_adapt = !dlg_adapt; paint_dialog(); });
    label(grp_one, "Avisos: te avisa si dejas una pieza en peligro.  Adaptable: se ajusta a tu juego.", &lv_font_montserrat_14, 0xB8B4DD, 38, 548);

    // — Dos jugadores: un nombre para cada bando —
    group_box(&grp_two, p);
    for (int i = 0; i < 2; i++) {
        lv_obj_t *c = mk(grp_two, 38, 96 + i * 160, 900, 148, 0x221F47, 26);
        lv_obj_set_style_border_width(c, 2, 0);
        lv_obj_set_style_border_color(c, C(0x4A4780), 0);
        lv_obj_t *king = lv_image_create(c);
        lv_image_set_src(king, &piece_sprites[i == 0 ? 5 : 11]);
        lv_image_set_pivot(king, 0, 0);
        lv_image_set_scale(king, 360);
        lv_obj_set_pos(king, 30, 26);
        lv_obj_remove_flag(king, LV_OBJ_FLAG_CLICKABLE);
        label(c, i == 0 ? "Juegan las blancas" : "Juegan las negras", &lv_font_montserrat_28, 0xB8B4DD, 150, 14);
        name_btn_2p[i] = button(c, "", 0x3F8CE6, 150, 60, 724, 72, on_name_btn, (void *)(intptr_t)i, &lv_font_montserrat_42);
    }
    label(grp_two, "El aparato en la mesa, cada uno a un lado.", &lv_font_montserrat_28, 0xB8B4DD, 38, 420);
    lv_obj_add_flag(grp_two, LV_OBJ_FLAG_HIDDEN);

    button(p, "¡A jugar!", 0x2FB67C, 700, 470, 238, 64, on_play);
    lv_obj_add_flag(dlg_new, LV_OBJ_FLAG_HIDDEN);
}

// ── Editor de nombres: teclado en pantalla con ñ y vocales con tilde ──────
static const char *const KM_LOWER[] = {"q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
                                       "a", "s", "d", "f", "g", "h", "j", "k", "l", "ñ", "\n",
                                       "Mayús", "z", "x", "c", "v", "b", "n", "m", LV_SYMBOL_BACKSPACE, "\n",
                                       "Acentos", "Espacio", "Aceptar", ""};
static const char *const KM_UPPER[] = {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "\n",
                                       "A", "S", "D", "F", "G", "H", "J", "K", "L", "Ñ", "\n",
                                       "Mayús", "Z", "X", "C", "V", "B", "N", "M", LV_SYMBOL_BACKSPACE, "\n",
                                       "Acentos", "Espacio", "Aceptar", ""};
static const char *const KM_ACCENT[] = {"á", "é", "í", "ó", "ú", "ü", "\n",
                                        "Á", "É", "Í", "Ó", "Ú", "Ü", "\n",
                                        "Letras", LV_SYMBOL_BACKSPACE, "\n",
                                        "Espacio", "Aceptar", ""};
// Anchos relativos de cada tecla (el máximo de LVGL es 15); se convierten a lv_buttonmatrix_ctrl_t al construir.
static const uint8_t KW_MAIN[] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  3, 2, 2, 2, 2, 2, 2, 2, 3,  4, 10, 6};
static const uint8_t KW_ACCENT[] = {3, 3, 3, 3, 3, 3,  3, 3, 3, 3, 3, 3,  10, 6,  10, 6};
static lv_buttonmatrix_ctrl_t kc_main[sizeof KW_MAIN], kc_accent[sizeof KW_ACCENT];

static void kb_set_letters() { lv_keyboard_set_mode(name_kb, kb_upper ? LV_KEYBOARD_MODE_USER_2 : LV_KEYBOARD_MODE_USER_1); }

static void name_accept() {
    char tmp[NAME_LEN];
    name_sanitize(tmp, lv_textarea_get_text(name_ta));
    memcpy(name_target == 0 ? dlg_n1 : dlg_n2, tmp, NAME_LEN);
    lv_obj_add_flag(dlg_name, LV_OBJ_FLAG_HIDDEN);
    paint_dialog();
}

static void kb_press(const char *tx) {
    if (!tx) return;
    if (!strcmp(tx, "Mayús")) { kb_upper = !kb_upper; kb_set_letters(); return; }
    if (!strcmp(tx, "Acentos")) { lv_keyboard_set_mode(name_kb, LV_KEYBOARD_MODE_USER_3); return; }
    if (!strcmp(tx, "Letras")) { kb_set_letters(); return; }
    if (!strcmp(tx, "Aceptar")) { name_accept(); return; }
    if (!strcmp(tx, LV_SYMBOL_BACKSPACE)) {
        lv_textarea_delete_char(name_ta);
        if (!lv_textarea_get_text(name_ta)[0]) kb_upper = true;   // vacío: vuelve la mayúscula inicial
        kb_set_letters();
        return;
    }
    if (!strcmp(tx, "Espacio")) tx = " ";
    lv_textarea_add_text(name_ta, tx);
    kb_upper = false;                                            // mayúscula solo en la primera letra
    kb_set_letters();
}

static void kb_event(lv_event_t *) {
    uint32_t id = lv_buttonmatrix_get_selected_button(name_kb);
    if (id == LV_BUTTONMATRIX_BUTTON_NONE) return;
    kb_press(lv_buttonmatrix_get_button_text(name_kb, id));
}

static void open_name_editor(int target) {
    name_target = target;
    const char *cur = target == 0 ? dlg_n1 : dlg_n2;
    lv_label_set_text(name_title, !dlg_two ? "¿Cómo te llamas?" : (target == 0 ? "Jugador de blancas" : "Jugador de negras"));
    lv_textarea_set_text(name_ta, cur);
    kb_upper = cur[0] == 0;
    kb_set_letters();
    lv_obj_remove_flag(dlg_name, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(dlg_name);
}

static void build_name_dialog() {
    dlg_name = mk(lv_layer_top(), 0, 0, 1024, 600, 0x0A0820, 0, 245);
    lv_obj_add_flag(dlg_name, LV_OBJ_FLAG_CLICKABLE);
    name_title = label(dlg_name, "", &lv_font_montserrat_42, 0xFFFFFF, 0, 14);
    lv_obj_set_width(name_title, 1024);
    lv_obj_set_style_text_align(name_title, LV_TEXT_ALIGN_CENTER, 0);
    name_ta = lv_textarea_create(dlg_name);
    lv_obj_set_size(name_ta, 640, 84);
    lv_obj_set_pos(name_ta, 192, 84);
    lv_textarea_set_one_line(name_ta, true);
    lv_textarea_set_max_length(name_ta, NAME_MAX_CHARS);
    lv_textarea_set_placeholder_text(name_ta, "Escribe aquí");
    lv_obj_set_style_text_font(name_ta, &lv_font_montserrat_42, 0);
    lv_obj_set_style_bg_color(name_ta, C(0xF4EFFF), 0);
    lv_obj_set_style_text_color(name_ta, C(0x2D2A5A), 0);
    lv_obj_set_style_radius(name_ta, 22, 0);
    lv_obj_set_style_border_width(name_ta, 4, 0);
    lv_obj_set_style_border_color(name_ta, C(0xFFB300), 0);
    lv_obj_set_style_pad_left(name_ta, 22, 0);
    label(dlg_name, "Un nombre corto o un apodo. Solo se guarda aquí.", &lv_font_montserrat_28, 0xB8B4DD, 192, 184);
    button(dlg_name, "Cancelar", 0x4A4780, 30, 20, 170, 56, [](lv_event_t *) { lv_obj_add_flag(dlg_name, LV_OBJ_FLAG_HIDDEN); }, nullptr, &lv_font_montserrat_28);
    name_kb = lv_keyboard_create(dlg_name);
    lv_obj_set_size(name_kb, 1024, 340);
    lv_obj_align(name_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    for (size_t i = 0; i < sizeof KW_MAIN; i++) kc_main[i] = (lv_buttonmatrix_ctrl_t)KW_MAIN[i];
    for (size_t i = 0; i < sizeof KW_ACCENT; i++) kc_accent[i] = (lv_buttonmatrix_ctrl_t)KW_ACCENT[i];
    lv_keyboard_set_map(name_kb, LV_KEYBOARD_MODE_USER_1, KM_LOWER, kc_main);
    lv_keyboard_set_map(name_kb, LV_KEYBOARD_MODE_USER_2, KM_UPPER, kc_main);
    lv_keyboard_set_map(name_kb, LV_KEYBOARD_MODE_USER_3, KM_ACCENT, kc_accent);
    lv_obj_set_style_text_font(name_kb, &lv_font_montserrat_32, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(name_kb, C(0x15122E), 0);
    lv_obj_set_style_bg_color(name_kb, C(0x3A3680), LV_PART_ITEMS);
    lv_obj_set_style_text_color(name_kb, C(0xFFFFFF), LV_PART_ITEMS);
    lv_obj_set_style_radius(name_kb, 16, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(name_kb, C(0xFFB300), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_add_event_cb(name_kb, kb_event, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_flag(dlg_name, LV_OBJ_FLAG_HIDDEN);
}

// ── Dos jugadores: un panel por jugador (el de arriba, girado 180°) ──────
static void resign_labels() {
    for (int i = 0; i < 2; i++) set_btn_text(side[i].resign, resign_armed == i ? "¿Seguro?" : "Rendirse");
}
static void disarm_resign() {
    resign_armed = -1;
    if (resign_timer) { lv_timer_delete(resign_timer); resign_timer = nullptr; }
    resign_labels();
}
static void resign_timeout(lv_timer_t *) { resign_timer = nullptr; resign_armed = -1; resign_labels(); }
static void resign_action(int i) {
    if (!snap.twoPlayer || snap.over || snap.plies == 0) return;
    if (resign_armed != i) {  // primer toque: pedir confirmación
        disarm_resign();
        resign_armed = i;
        resign_timer = lv_timer_create(resign_timeout, 3000, nullptr);
        lv_timer_set_repeat_count(resign_timer, 1);
        resign_labels();
        return;
    }
    disarm_resign();
    host->post_resign(i == 0);
}
static void on_resign(lv_event_t *e) { resign_action((int)(intptr_t)lv_event_get_user_data(e)); }

static void build_two_panels() {
    for (int i = 0; i < 2; i++) {
        SidePanel &s = side[i];
        s.root = mk(scr, PX, i == 0 ? 344 : 6, PW, 250, 0x221F47, 30);
        lv_obj_set_style_border_width(s.root, 3, 0);
        lv_obj_set_style_border_color(s.root, C(0x4A4780), 0);
        s.dot = mk(s.root, 18, 16, 38, 38, i == 0 ? 0xF5F5F5 : 0x1B1B1B, LV_RADIUS_CIRCLE);
        lv_obj_set_style_border_width(s.dot, 3, 0);
        lv_obj_set_style_border_color(s.dot, C(0x9A97C8), 0);
        s.name = label(s.root, "", &lv_font_montserrat_42, 0xFFFFFF, 70, 8);
        lv_obj_set_width(s.name, 300);
        lv_label_set_long_mode(s.name, LV_LABEL_LONG_CLIP);
        s.status = label(s.root, "", &lv_font_montserrat_28, 0xFFD23F, 18, 68);
        lv_obj_set_width(s.status, 356);
        lv_label_set_long_mode(s.status, LV_LABEL_LONG_CLIP);
        s.tray = mk(s.root, 12, 108, 368, 44, 0x000000, 0, 0);
        s.resign = button(s.root, "Rendirse", 0xD9534F, 18, 160, 356, 74, on_resign, (void *)(intptr_t)i);
        if (i == 1) {  // quien juega con negras está al otro lado de la mesa
            lv_obj_set_style_transform_pivot_x(s.root, PW / 2, 0);
            lv_obj_set_style_transform_pivot_y(s.root, 125, 0);
            lv_obj_set_style_transform_rotation(s.root, 1800, 0);
        }
    }
    mid_strip = mk(scr, PX, 262, PW, 76, 0x000000, 0, 0);
    mid_undo = button(mid_strip, "Deshacer", 0x3F8CE6, 0, 2, 142, 72, [](lv_event_t *e) { on_undo(e); }, nullptr, &lv_font_montserrat_28);
    button(mid_strip, "Partidas", 0x8E6CD8, 149, 2, 132, 72, [](lv_event_t *e) { on_slots_open(e); }, nullptr, &lv_font_montserrat_28);
    button(mid_strip, "Nueva", 0xF08A24, 288, 2, 104, 72, [](lv_event_t *e) { on_new(e); }, nullptr, &lv_font_montserrat_28);
}

static void rebuild_tray_two() {
    for (int i = 0; i < 2; i++) {
        lv_obj_clean(side[i].tray);
        tray_row(side[i].tray, 0, 4, i == 0 ? snap.missingBlack : snap.missingWhite, i == 1);  // cada uno muestra lo que ha capturado
    }
}

static void update_two_player() {
    bool wtm = snap.whiteToMove;
    for (int i = 0; i < 2; i++) {
        bool mine = !snap.over && (i == 0) == wtm;
        lv_label_set_text(side[i].name, pname(i));
        const char *st;
        if (snap.over) {
            if (snap.resigned) st = snap.resigned == (i == 0 ? 1 : 2) ? "Te has rendido" : "¡Has ganado!";
            else if (snap.status == ce::ST_MATE) st = (i == 0) == wtm ? "Jaque mate" : "¡Has ganado!";
            else st = "¡Tablas!";
        } else if (mine) st = snap.status == ce::ST_CHECK ? "¡Estás en jaque!" : "¡Tu turno!";
        else st = "Espera";
        lv_label_set_text(side[i].status, st);
        lv_obj_set_style_text_color(side[i].status, C(mine ? 0xFFD23F : 0xB8B4DD), 0);
        lv_obj_set_style_bg_color(side[i].root, C(mine ? 0x3A3680 : 0x221F47), 0);
        lv_obj_set_style_border_color(side[i].root, C(mine ? 0xFFB300 : 0x4A4780), 0);
        lv_obj_set_style_border_width(side[i].root, mine ? 6 : 3, 0);
    }
    disarm_resign();
}

static void apply_mode() {
    lv_obj_t *one[] = {card, status_pill, tray_cont, btn_undo, btn_hint, btn_new, btn_slots};
    for (lv_obj_t *o : one) { if (s_two) lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN); else lv_obj_remove_flag(o, LV_OBJ_FLAG_HIDDEN); }
    if (s_two) lv_obj_add_flag(warn_card, LV_OBJ_FLAG_HIDDEN);
    for (int i = 0; i < 2; i++) { if (s_two) lv_obj_remove_flag(side[i].root, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(side[i].root, LV_OBJ_FLAG_HIDDEN); }
    if (s_two) lv_obj_remove_flag(mid_strip, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(mid_strip, LV_OBJ_FLAG_HIDDEN);
    // Carteles de fin: uno en el centro, o dos (uno girado) en la mitad de cada jugador
    lv_obj_set_pos(banner, BX + 40, s_two ? BY + 300 : BY + 205);
    lv_obj_set_pos(banner2, BX + 40, BY + 110);
}

// ── Partidas guardadas ────────────────────────────────────────────────────
static bool needs_confirm(int kind, int slot) {
    if (kind == K_SAVE) return snap.slots[slot].used;                    // sobrescribir
    if (kind == K_DEL) return true;
    return snap.plies > 0 && !game_over();                               // cargar sustituye la partida en curso
}

static void refresh_slots() {
    static const char *const TXT[3] = {"Guardar", "Cargar", "Borrar"};
    for (int i = 0; i < SAVE_SLOTS; i++) {
        const SlotInfo &s = snap.slots[i];
        lv_obj_clean(slot_av[i]);
        if (s.used) {
            char nm[64], b[48];
            if (s.twoPlayer) {  // dos reyes en vez de mascota, y los dos nombres
                for (int k = 0; k < 2; k++) {
                    lv_obj_t *king = lv_image_create(slot_av[i]);
                    lv_image_set_src(king, &piece_sprites[k == 0 ? 5 : 11]);
                    lv_image_set_pivot(king, 0, 0);
                    lv_image_set_scale(king, 200);
                    lv_obj_set_pos(king, k * 38, 16 + k * 12);
                    lv_obj_remove_flag(king, LV_OBJ_FLAG_CLICKABLE);
                }
                snprintf(nm, sizeof nm, "%s vs %s", s.name1[0] ? s.name1 : "Blancas", s.name2[0] ? s.name2 : "Negras");
                snprintf(b, sizeof b, "%d jugadas", s.plies);
            } else {
                Avatar a = avatar_create(slot_av[i], s.level, 84);
                lv_obj_remove_flag(a.root, LV_OBJ_FLAG_CLICKABLE);
                if (s.name1[0]) snprintf(nm, sizeof nm, "%s vs %s", s.name1, level_info(s.level).name);
                else snprintf(nm, sizeof nm, "%s", level_info(s.level).name);
                snprintf(b, sizeof b, "%d jugadas, %s", s.plies, s.humanWhite ? "Blancas" : "Negras");
            }
            lv_label_set_text(slot_name[i], nm);
            lv_label_set_text(slot_detail[i], b);
        } else {
            char b[24];
            snprintf(b, sizeof b, "Hueco %d", i + 1);
            lv_label_set_text(slot_name[i], b);
            lv_label_set_text(slot_detail[i], "Vacío");
        }
        for (int k = 0; k < 3; k++) {
            bool armed = armed_slot == i && armed_kind == k;
            set_btn_text(slot_btn[i][k], armed ? "¿Seguro?" : TXT[k]);
            static const uint32_t COL[3] = {0x2FB67C, 0x3F8CE6, 0xD9534F};
            lv_obj_set_style_bg_color(slot_btn[i][k], C(armed ? 0xE8641B : COL[k]), 0);
            lv_obj_set_style_bg_grad_color(slot_btn[i][k], lv_color_darken(C(armed ? 0xE8641B : COL[k]), 35), 0);
            bool enabled = k == K_SAVE ? snap.plies > 0 : s.used;
            if (enabled) lv_obj_remove_state(slot_btn[i][k], LV_STATE_DISABLED); else lv_obj_add_state(slot_btn[i][k], LV_STATE_DISABLED);
        }
    }
}

static void disarm() {
    armed_slot = armed_kind = -1;
    if (armed_timer) { lv_timer_delete(armed_timer); armed_timer = nullptr; }
}
static void disarm_cb(lv_timer_t *) { armed_timer = nullptr; armed_slot = armed_kind = -1; refresh_slots(); }

static void toast_cb(lv_timer_t *) { toast_timer = nullptr; lv_label_set_text(slot_toast, ""); }
static void show_toast(const char *txt, uint32_t col) {
    lv_label_set_text(slot_toast, txt);
    lv_obj_set_style_text_color(slot_toast, C(col), 0);
    if (toast_timer) lv_timer_delete(toast_timer);
    toast_timer = lv_timer_create(toast_cb, 2800, nullptr);
    lv_timer_set_repeat_count(toast_timer, 1);
}

static void slot_action(int kind, int slot) {
    if (slot < 0 || slot >= SAVE_SLOTS) return;
    if (needs_confirm(kind, slot) && !(armed_slot == slot && armed_kind == kind)) {  // primer toque: pedir confirmación
        disarm();
        armed_slot = slot;
        armed_kind = kind;
        armed_timer = lv_timer_create(disarm_cb, 3000, nullptr);
        lv_timer_set_repeat_count(armed_timer, 1);
        refresh_slots();
        return;
    }
    disarm();
    if (kind == K_SAVE) host->post_save(slot);
    else if (kind == K_LOAD) host->post_load(slot);
    else host->post_delete(slot);
    refresh_slots();
}

static void on_slot_btn(lv_event_t *e) {
    int v = (int)(intptr_t)lv_event_get_user_data(e);
    slot_action(v & 3, v >> 2);
}

static void on_slots_open(lv_event_t *) {
    host->stop_search();
    disarm();
    refresh_slots();
    lv_label_set_text(slot_toast, "");
    lv_obj_remove_flag(dlg_slots, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(dlg_slots);
    update_buttons();
}
static void on_slots_close(lv_event_t *) { disarm(); lv_obj_add_flag(dlg_slots, LV_OBJ_FLAG_HIDDEN); update_buttons(); }

static void build_slots_dialog() {
    dlg_slots = mk(lv_layer_top(), 0, 0, 1024, 600, 0x0A0820, 0, 215);
    lv_obj_add_flag(dlg_slots, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *p = mk(dlg_slots, 24, 8, 976, 584, 0x2D2A5A, 36);
    lv_obj_set_style_border_width(p, 3, 0);
    lv_obj_set_style_border_color(p, C(0x4A4780), 0);
    lv_obj_t *t = label(p, "Mis partidas", &lv_font_montserrat_42, 0xFFFFFF, 0, 12);
    lv_obj_set_width(t, 976);
    lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_CENTER, 0);
    for (int i = 0; i < SAVE_SLOTS; i++) {
        lv_obj_t *c = mk(p, 38, 76 + i * 136, 900, 124, 0x221F47, 26);
        lv_obj_set_style_border_width(c, 2, 0);
        lv_obj_set_style_border_color(c, C(0x4A4780), 0);
        slot_av[i] = mk(c, 14, 18, 88, 88, 0x000000, 0, 0);
        slot_name[i] = label(c, "", &lv_font_montserrat_32, 0xFFFFFF, 116, 20);
        lv_obj_set_width(slot_name[i], 284);
        lv_label_set_long_mode(slot_name[i], LV_LABEL_LONG_CLIP);
        slot_detail[i] = label(c, "", &lv_font_montserrat_28, 0xB8B4DD, 116, 70);
        lv_obj_set_width(slot_detail[i], 284);
        lv_label_set_long_mode(slot_detail[i], LV_LABEL_LONG_CLIP);
        static const char *const TXT[3] = {"Guardar", "Cargar", "Borrar"};
        static const int X[3] = {404, 580, 744}, W[3] = {160, 150, 140};
        for (int k = 0; k < 3; k++)
            slot_btn[i][k] = button(c, TXT[k], 0x2FB67C, X[k], 26, W[k], 72, on_slot_btn, (void *)(intptr_t)(i * 4 + k));
    }
    slot_toast = label(p, "", &lv_font_montserrat_28, 0xFFD23F, 0, 488);
    lv_obj_set_width(slot_toast, 976);
    lv_obj_set_style_text_align(slot_toast, LV_TEXT_ALIGN_CENTER, 0);
    button(p, "Cerrar", 0x4A4780, 338, 520, 300, 56, on_slots_close);
    lv_obj_add_flag(dlg_slots, LV_OBJ_FLAG_HIDDEN);
}

static void build_promo_dialog() {
    dlg_promo = mk(lv_layer_top(), 0, 0, 1024, 600, 0x0A0820, 0, 215);
    lv_obj_add_flag(dlg_promo, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *t = label(dlg_promo, "¡Tu peón llegó! ¿En qué lo conviertes?", &lv_font_montserrat_42, 0xFFFFFF, 0, 130);
    lv_obj_set_width(t, 1024);
    lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *row = mk(dlg_promo, 112, 230, 800, 190, 0x000000, 0, 0);
    for (int i = 0; i < 4; i++) {
        lv_obj_t *c = mk(row, i * 204, 0, 188, 190, 0xF0D9B5, 28);
        lv_obj_set_style_border_width(c, 4, 0);
        lv_obj_set_style_border_color(c, C(0xB58863), 0);
        lv_obj_set_style_bg_color(c, C(0xD9DE72), LV_STATE_PRESSED);
        lv_obj_add_flag(c, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(c, on_promo, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_obj_t *im = lv_image_create(c);
        lv_image_set_src(im, &piece_sprites[4]);
        lv_image_set_pivot(im, 32, 32);
        lv_image_set_scale(im, 512);
        lv_obj_set_pos(im, 62, 62);
        lv_obj_remove_flag(im, LV_OBJ_FLAG_CLICKABLE);
    }
    lv_obj_add_flag(dlg_promo, LV_OBJ_FLAG_HIDDEN);
}

static lv_obj_t *make_banner(bool rotated) {
    const int W = 8 * SQ - 80;
    lv_obj_t *b = mk(scr, BX + 40, BY + 205, W, 150, 0x14123A, 30, 235);
    lv_obj_set_style_border_width(b, 3, 0);
    lv_obj_set_style_border_color(b, C(0xFFB300), 0);
    lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(b, banner_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *t = label(b, "", &lv_font_montserrat_48, 0xFFD23F, 0, 14);
    lv_obj_set_width(t, W);
    lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *s = label(b, "", &lv_font_montserrat_28, 0xFFFFFF, 12, 88);
    lv_obj_set_width(s, W - 24);
    lv_label_set_long_mode(s, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(s, LV_TEXT_ALIGN_CENTER, 0);
    if (rotated) {  // para quien juega desde el lado de arriba
        lv_obj_set_style_transform_pivot_x(b, W / 2, 0);
        lv_obj_set_style_transform_pivot_y(b, 75, 0);
        lv_obj_set_style_transform_rotation(b, 1800, 0);
    }
    lv_obj_add_flag(b, LV_OBJ_FLAG_HIDDEN);
    return b;
}

static void build_banner() {
    banner = make_banner(false);
    banner2 = make_banner(true);
}

// ── Actualización desde el snapshot ───────────────────────────────────────
static bool is_capture(const Snapshot &before, const Snapshot &after) {
    if (!after.hasLast || after.plies != before.plies + 1) return false;
    int cnt_b = 0, cnt_a = 0;
    for (int i = 0; i < 64; i++) { cnt_b += before.board[i] != 0; cnt_a += after.board[i] != 0; }
    return cnt_a < cnt_b;
}

static void handle_op() {
    switch (snap.lastOp) {
        case 1: show_toast("¡Partida guardada!", 0x7CE0A0); break;
        case 2:  // cargada: cerrar el diálogo y no repetir el confeti de una partida ya terminada
            disarm();
            lv_obj_add_flag(dlg_slots, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(banner, LV_OBJ_FLAG_HIDDEN);
            banner_shown = game_over();
            update_buttons();
            break;
        case 3: show_toast("Partida borrada", 0xFFD23F); break;
        case -1: show_toast("No se pudo guardar", 0xFF8F8F); break;
        case -2: show_toast("No se pudo cargar", 0xFF8F8F); break;
        case -3: show_toast("No se pudo borrar", 0xFF8F8F); break;
        default: break;
    }
}

static void apply_snapshot() {
    bool first = !have_snap;
    if (first) {  // recordar la última elección del diálogo (modo, nivel, color, avisos y nombres)
        dlg_level = snap.prefs.level;
        dlg_white = snap.prefs.humanWhite;
        dlg_coach = snap.prefs.coach;
        dlg_adapt = snap.prefs.adaptive;
        dlg_two = snap.prefs.twoPlayer;
        memcpy(dlg_n1, snap.prefs.name1, NAME_LEN);
        memcpy(dlg_n2, snap.prefs.name2, NAME_LEN);
        paint_dialog();
    }
    bool mode_changed = first || snap.twoPlayer != s_two;
    if (mode_changed) {  // las piezas negras cambian de sprite (giradas en dos jugadores): se recrean
        s_two = snap.twoPlayer;
        apply_mode();
        for (int i = 0; i < 64; i++) drop_piece(i, false);
    }
    bool structural_prev = !first && !mode_changed;
    bool animate = structural_prev && snap.plies == prev.plies + 1 && snap.lastOp != 2;
    bool cap = structural_prev && is_capture(prev, snap);
    bool mover_is_human = snap.whiteToMove != snap.humanWhite;  // quien acaba de mover es el contrario del que tiene turno
    bool new_flip = !snap.humanWhite && !snap.twoPlayer;
    if (!s_two) build_avatar(snap.level);
    if (new_flip != flipped || mode_changed) { flipped = new_flip; relayout(); }
    bool same_pos = structural_prev && snap.plies == prev.plies && memcmp(snap.board, prev.board, 64) == 0 &&
                    snap.awaiting == prev.awaiting && snap.level == prev.level && snap.humanWhite == prev.humanWhite &&
                    snap.effLevel == prev.effLevel && snap.status == prev.status && snap.adaptEvent == 0 && snap.lastOp != 2 &&
                    snap.resigned == prev.resigned && !strcmp(snap.name1, prev.name1) && !strcmp(snap.name2, prev.name2);
    if (!same_pos) {
        if (sel >= 0) { unlift(sel); sel = -1; memset(target, 0, sizeof target); }
        sync_pieces(animate);
        if (s_two) {
            rebuild_tray_two();
            update_two_player();
        } else {
            rebuild_tray();
            update_dots();
            update_status(cap && !mover_is_human, cap && mover_is_human);
        }
    }
    paint_squares();
    update_buttons();
    handle_op();
    if (!lv_obj_has_flag(dlg_slots, LV_OBJ_FLAG_HIDDEN)) refresh_slots();
    if (game_over()) end_effects(); else if (!animate && !same_pos) banner_shown = false;
    have_snap = true;
    prev = snap;
    prev.hasHint = false;
}

static void poll_cb(lv_timer_t *) {
    Snapshot s;
    if (host->poll(s)) {
        if (!have_snap)
            for (int i = 0; i < 64; i++) drop_piece(i, false);  // partida nueva: vaciar el tablero mostrado
        snap = s;
        apply_snapshot();
    }
}

void ui_game_create(lv_obj_t *screen, GameHost *h) {
    host = h;
    scr = screen;
    lv_obj_set_style_bg_color(scr, C(0x3A2F78), 0);
    lv_obj_set_style_bg_grad_color(scr, C(0x15122E), 0);
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    build_board();
    build_panel();
    build_two_panels();
    build_banner();
    build_new_dialog();
    build_name_dialog();
    build_slots_dialog();
    build_promo_dialog();
    // Hasta el primer snapshot: Tortuga, blancas, un jugador
    memset(&snap, 0, sizeof snap);
    snap.level = dlg_level;
    snap.humanWhite = true;
    s_two = false;
    apply_mode();
    build_avatar(dlg_level);
    open_new_dialog();
    lv_timer_create(poll_cb, 33, nullptr);
}

// ── API de pruebas ────────────────────────────────────────────────────────
void ui_game_debug_tap_square(int s) {
    if (!interactive()) return;
    int8_t pc = snap.board[s];
    bool own = pc != 0 && (pc > 0) == (s_two ? snap.whiteToMove : snap.humanWhite);   // en dos jugadores, las del que tiene el turno
    if (sel >= 0 && target[s]) {
        int from = sel;
        bool promo = false;
        for (int i = 0; i < snap.nLegal; i++)
            if (snap.legal[i].from == from && snap.legal[i].to == s && snap.legal[i].promo) promo = true;
        if (promo) { open_promo(from, s); return; }
        send_move(from, s, 0);
        return;
    }
    bool has = false;
    for (int i = 0; i < snap.nLegal; i++) if (snap.legal[i].from == s) has = true;
    if (own && has && s != sel) select_square(s); else select_square(-1);
}

void ui_game_debug_open_new_dialog() { open_new_dialog(); }

void ui_game_debug_tap(const char *w) {
    if (!strcmp(w, "undo")) on_undo(nullptr);
    else if (!strcmp(w, "hint")) on_hint(nullptr);
    else if (!strcmp(w, "new")) on_new(nullptr);
    else if (!strcmp(w, "play")) start_game();
    else if (!strcmp(w, "white")) { dlg_white = true; refresh_dialog(); }
    else if (!strcmp(w, "black")) { dlg_white = false; refresh_dialog(); }
    else if (!strncmp(w, "lvl:", 4)) { dlg_level = atoi(w + 4); refresh_dialog(); }
    else if (!strncmp(w, "promo:", 6)) promo_choose(w[6]);
    else if (!strcmp(w, "coach")) { dlg_coach = !dlg_coach; refresh_dialog(); }
    else if (!strcmp(w, "adapt")) { dlg_adapt = !dlg_adapt; refresh_dialog(); }
    else if (!strcmp(w, "confirm")) on_hint(nullptr);
    else if (!strcmp(w, "slots")) on_slots_open(nullptr);
    else if (!strncmp(w, "save:", 5)) slot_action(K_SAVE, atoi(w + 5));
    else if (!strncmp(w, "load:", 5)) slot_action(K_LOAD, atoi(w + 5));
    else if (!strncmp(w, "del:", 4)) slot_action(K_DEL, atoi(w + 4));
    else if (!strncmp(w, "mode:", 5)) { dlg_two = w[5] == '2'; paint_dialog(); }
    else if (!strncmp(w, "name:", 5)) { name_sanitize(w[5] == '0' ? dlg_n1 : dlg_n2, w + 7); paint_dialog(); }
    else if (!strncmp(w, "edit:", 5)) open_name_editor(atoi(w + 5));
    else if (!strncmp(w, "key:", 4)) kb_press(w + 4);
    else if (!strncmp(w, "resign:", 7)) resign_action(atoi(w + 7));
    else if (!strcmp(w, "banner")) banner_cb(nullptr);
}
