#include "ui_game.h"
#include "ui_avatar.h"
#include "pieces.h"
#include "ppa_fx.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
static lv_obj_t *btn_undo, *btn_hint, *btn_new;
static lv_obj_t *dlg_new, *dlg_promo, *banner;
static lv_obj_t *lvl_card[LEVEL_COUNT];
static lv_obj_t *col_btn[2];
static int dlg_level = 1;
static bool dlg_white = true;
static int promo_from = -1, promo_to = -1;
static bool banner_shown;

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

static lv_obj_t *button(lv_obj_t *parent, const char *txt, uint32_t col, int x, int y, int w, int h, lv_event_cb_t cb, void *ud = nullptr) {
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
    lv_obj_set_style_text_font(l, &lv_font_montserrat_32, 0);
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
static const lv_image_dsc_t *sprite(int8_t p) { return &piece_sprites[piece_index(p)]; }

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
    const lv_image_dsc_t *big = ppa_fx_lifted(piece_index(p_type[s]));
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
        lv_obj_set_style_bg_color(sq_obj[s], C(col), 0);
        bool cap = target[s] && snap.board[s] != 0;
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

static bool human_turn() { return have_snap && snap.whiteToMove == snap.humanWhite && !(snap.status >= ce::ST_MATE); }
static bool game_over() { return have_snap && snap.status >= ce::ST_MATE; }
static bool interactive() { return human_turn() && lv_obj_has_flag(dlg_new, LV_OBJ_FLAG_HIDDEN) && lv_obj_has_flag(dlg_promo, LV_OBJ_FLAG_HIDDEN); }

static void rebuild_tray() {
    lv_obj_clean(tray_cont);
    lv_obj_t *me = mk(tray_cont, 4, 8, 50, 50, 0x3F8CE6, LV_RADIUS_CIRCLE);  // insignia del jugador
    lv_obj_t *me_l = label(me, "Tú", &lv_font_montserrat_28, 0xFFFFFF, 0, 0);
    lv_obj_center(me_l);
    Avatar cpu = avatar_create(tray_cont, snap.level, 50);                    // y la mascota del rival
    lv_obj_set_pos(cpu.root, 4, 68);
    for (int row = 0; row < 2; row++) {
        const uint8_t *miss = (row == 0) == snap.humanWhite ? snap.missingBlack : snap.missingWhite;  // lo que falta al otro bando
        int x = 66;
        bool white_pieces = !((row == 0) == snap.humanWhite);
        for (int t = 4; t >= 0; t--) {  // dama ... peón
            if (!miss[t]) continue;
            int idx = (white_pieces ? 0 : 6) + t;
            lv_obj_t *im = lv_image_create(tray_cont);
            const lv_image_dsc_t *mini = ppa_fx_mini(idx);
            if (mini) {
                lv_image_set_src(im, mini);
                lv_obj_set_pos(im, x, 14 + row * 60);
            } else {
                lv_image_set_src(im, &piece_sprites[idx]);
                lv_image_set_pivot(im, 0, 0);
                lv_image_set_scale(im, 128);
                lv_obj_set_pos(im, x, 14 + row * 60);
            }
            x += 32;
            if (miss[t] > 1) {
                char b[8];
                snprintf(b, sizeof b, "x%d", miss[t]);
                label(tray_cont, b, &lv_font_montserrat_14, 0xFFFFFF, x, 36 + row * 60);
                x += 24;
            }
            x += 6;
        }
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
        if (snap.status == ce::ST_CHECK) { lv_label_set_text(status_lbl, "¡Estás en jaque!"); set_speech(pick(S_CHECK_HUMAN, 2), Mood::Happy); }
        else {
            lv_label_set_text(status_lbl, "Tu turno");
            set_speech(cap_by_cpu ? pick(S_CAPTURE_ME, 2) : pick(S_TURN, 3), cap_by_cpu ? Mood::Happy : Mood::Normal);
        }
    }
}

static void update_buttons() {
    bool can = human_turn() && lv_obj_has_flag(dlg_new, LV_OBJ_FLAG_HIDDEN);
    auto set = [](lv_obj_t *b, bool on) { if (on) lv_obj_remove_state(b, LV_STATE_DISABLED); else lv_obj_add_state(b, LV_STATE_DISABLED); };
    set(btn_undo, can && snap.plies > 0);
    set(btn_hint, can);
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

static void banner_cb(lv_event_t *) { lv_obj_add_flag(banner, LV_OBJ_FLAG_HIDDEN); }

static void show_banner(const char *title, const char *sub, uint32_t col) {
    lv_obj_t *t = lv_obj_get_child(banner, 0), *s = lv_obj_get_child(banner, 1);
    lv_label_set_text(t, title);
    lv_label_set_text(s, sub);
    lv_obj_set_style_text_color(t, C(col), 0);
    lv_obj_remove_flag(banner, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(banner);
}

static void end_effects() {
    if (banner_shown) return;
    banner_shown = true;
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
    bool w = snap.humanWhite;
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

static void refresh_dialog() {
    for (int i = 0; i < LEVEL_COUNT; i++) {
        lv_obj_set_style_border_color(lvl_card[i], C(i == dlg_level ? 0xFFB300 : 0x4A4780), 0);
        lv_obj_set_style_border_width(lvl_card[i], i == dlg_level ? 6 : 2, 0);
    }
    for (int i = 0; i < 2; i++) {
        bool on = (i == 0) == dlg_white;
        lv_obj_set_style_bg_color(col_btn[i], C(on ? 0xFFB300 : 0x4A4780), 0);
        lv_obj_set_style_bg_grad_color(col_btn[i], C(on ? 0xE08F00 : 0x35336B), 0);
    }
    lv_obj_remove_flag(dlg_new, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(dlg_new);
    update_buttons();
}

static void open_new_dialog() {
    dlg_level = snap.level;
    refresh_dialog();
}

static void start_game() {
    lv_obj_add_flag(dlg_new, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(banner, LV_OBJ_FLAG_HIDDEN);
    banner_shown = false;
    select_square(-1);
    have_snap = false;
    host->post_new(dlg_white, dlg_level);
}

static void on_new(lv_event_t *) { host->stop_search(); open_new_dialog(); }
static void on_undo(lv_event_t *) { if (human_turn()) { select_square(-1); host->post_undo(); } }
static void on_hint(lv_event_t *) { if (human_turn()) host->post_hint(); }
static void on_play(lv_event_t *) { start_game(); }
static void on_level(lv_event_t *e) {
    dlg_level = (int)(intptr_t)lv_event_get_user_data(e);
    for (int i = 0; i < LEVEL_COUNT; i++) {
        lv_obj_set_style_border_color(lvl_card[i], C(i == dlg_level ? 0xFFB300 : 0x4A4780), 0);
        lv_obj_set_style_border_width(lvl_card[i], i == dlg_level ? 6 : 2, 0);
    }
}
static void on_color(lv_event_t *e) {
    dlg_white = (intptr_t)lv_event_get_user_data(e) == 0;
    for (int i = 0; i < 2; i++) {
        bool on = (i == 0) == dlg_white;
        lv_obj_set_style_bg_color(col_btn[i], C(on ? 0xFFB300 : 0x4A4780), 0);
        lv_obj_set_style_bg_grad_color(col_btn[i], C(on ? 0xE08F00 : 0x35336B), 0);
    }
}
static void on_promo(lv_event_t *e) { promo_choose("qrbn"[(intptr_t)lv_event_get_user_data(e)]); }

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
    // Bocadillo
    lv_obj_t *bub = mk(card, 152, 70, 226, 104, 0xFFFFFF, 20);
    speech_lbl = label(bub, "", &lv_font_montserrat_28, 0x2D2A5A, 12, 10);
    lv_obj_set_width(speech_lbl, 202);
    lv_label_set_long_mode(speech_lbl, LV_LABEL_LONG_WRAP);

    lv_obj_t *pill = mk(scr, PX, 206, PW, 62, 0x1B1940, 31);
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

    int bw = (PW - 16) / 2;
    btn_undo = button(scr, "Deshacer", 0x3F8CE6, PX, 410, bw, 82, [](lv_event_t *e) { on_undo(e); });
    btn_hint = button(scr, "Pista", 0x2FB67C, PX + bw + 16, 410, bw, 82, [](lv_event_t *e) { on_hint(e); });
    btn_new = button(scr, "Nueva partida", 0xF08A24, PX, 508, PW, 82, [](lv_event_t *e) { on_new(e); });
}

static void build_new_dialog() {
    dlg_new = mk(lv_layer_top(), 0, 0, 1024, 600, 0x0A0820, 0, 215);
    lv_obj_add_flag(dlg_new, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *p = mk(dlg_new, 32, 24, 960, 552, 0x2D2A5A, 36);
    lv_obj_set_style_border_width(p, 3, 0);
    lv_obj_set_style_border_color(p, C(0x4A4780), 0);
    lv_obj_t *t = label(p, "¿Contra quién quieres jugar?", &lv_font_montserrat_42, 0xFFFFFF, 0, 16);
    lv_obj_set_width(t, 960);
    lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_CENTER, 0);
    for (int i = 0; i < LEVEL_COUNT; i++) {
        int cx = 30 + (i % 3) * 306, cy = 84 + (i / 3) * 184;
        lv_obj_t *c = mk(p, cx, cy, 290, 170, 0x221F47, 26);
        lv_obj_set_style_border_width(c, 2, 0);
        lv_obj_set_style_border_color(c, C(0x4A4780), 0);
        lv_obj_add_flag(c, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(c, on_level, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        Avatar a = avatar_create(c, i, 96);
        lv_obj_set_pos(a.root, 14, 34);
        lv_obj_remove_flag(a.root, LV_OBJ_FLAG_CLICKABLE);
        const Level &lv = level_info(i);
        label(c, lv.name, &lv_font_montserrat_32, 0xFFFFFF, 124, 22);
        for (int k = 0; k < 5; k++)  // estrellas de dificultad = puntos
            mk(c, 126 + k * 26, 74, 18, 18, k <= i * 4 / 5 ? 0xFFB300 : 0x4A4780, LV_RADIUS_CIRCLE);
        lv_obj_t *tg = label(c, lv.tagline, &lv_font_montserrat_14, 0xB8B4DD, 124, 104);
        lv_obj_set_width(tg, 160);
        lvl_card[i] = c;
    }
    label(p, "Juego con:", &lv_font_montserrat_28, 0xB8B4DD, 36, 462);
    col_btn[0] = button(p, "Blancas", 0xFFB300, 224, 446, 190, 64, on_color, (void *)0);
    col_btn[1] = button(p, "Negras", 0x4A4780, 424, 446, 190, 64, on_color, (void *)1);
    button(p, "¡A jugar!", 0x2FB67C, 664, 446, 260, 64, on_play);
    lv_obj_add_flag(dlg_new, LV_OBJ_FLAG_HIDDEN);
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

static void build_banner() {
    banner = mk(scr, BX + 40, BY + 190, 8 * SQ - 80, 180, 0x14123A, 30, 235);
    lv_obj_set_style_border_width(banner, 3, 0);
    lv_obj_set_style_border_color(banner, C(0xFFB300), 0);
    lv_obj_add_flag(banner, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(banner, banner_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *t = label(banner, "", &lv_font_montserrat_48, 0xFFD23F, 0, 28);
    lv_obj_set_width(t, 8 * SQ - 80);
    lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *s = label(banner, "", &lv_font_montserrat_28, 0xFFFFFF, 12, 108);
    lv_obj_set_width(s, 8 * SQ - 104);
    lv_label_set_long_mode(s, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(s, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_add_flag(banner, LV_OBJ_FLAG_HIDDEN);
}

// ── Actualización desde el snapshot ───────────────────────────────────────
static bool is_capture(const Snapshot &before, const Snapshot &after) {
    if (!after.hasLast || after.plies != before.plies + 1) return false;
    int cnt_b = 0, cnt_a = 0;
    for (int i = 0; i < 64; i++) { cnt_b += before.board[i] != 0; cnt_a += after.board[i] != 0; }
    return cnt_a < cnt_b;
}

static void apply_snapshot() {
    bool first = !have_snap;
    bool structural_prev = !first;
    bool animate = structural_prev && snap.plies == prev.plies + 1;
    bool cap = structural_prev && is_capture(prev, snap);
    bool mover_is_human = snap.whiteToMove != snap.humanWhite;  // quien acaba de mover es el contrario del que tiene turno
    bool new_flip = !snap.humanWhite;
    build_avatar(snap.level);
    if (new_flip != flipped || first) { flipped = new_flip; relayout(); }
    bool hint_only = structural_prev && snap.hasHint && snap.plies == prev.plies && memcmp(snap.board, prev.board, 64) == 0;
    if (!hint_only) {
        if (sel >= 0) { unlift(sel); sel = -1; memset(target, 0, sizeof target); }
        sync_pieces(animate);
        rebuild_tray();
        update_status(cap && !mover_is_human, cap && mover_is_human);
    }
    paint_squares();
    update_buttons();
    if (game_over()) end_effects(); else if (!animate && !hint_only) banner_shown = false;
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
    build_banner();
    build_new_dialog();
    build_promo_dialog();
    // Partida inicial hasta que el jugador elija en el diálogo: Tortuga, blancas
    snap = Snapshot();
    memset(&snap, 0, sizeof snap);
    snap.level = dlg_level;
    snap.humanWhite = true;
    build_avatar(dlg_level);
    open_new_dialog();
    lv_timer_create(poll_cb, 33, nullptr);
}

// ── API de pruebas ────────────────────────────────────────────────────────
void ui_game_debug_tap_square(int s) {
    if (!interactive()) return;
    int8_t pc = snap.board[s];
    bool own = pc != 0 && (pc > 0) == snap.humanWhite;
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
    else if (!strcmp(w, "banner")) banner_cb(nullptr);
}
