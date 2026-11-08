#include "ui_avatar.h"

static lv_obj_t *blob(lv_obj_t *p, int x, int y, int w, int h, lv_color_t c, int radius = LV_RADIUS_CIRCLE, int rot10 = 0) {
    lv_obj_t *o = lv_obj_create(p);
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, c, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(o, radius, 0);
    if (rot10) {
        lv_obj_set_style_transform_pivot_x(o, w / 2, 0);
        lv_obj_set_style_transform_pivot_y(o, h / 2, 0);
        lv_obj_set_style_transform_rotation(o, rot10, 0);
    }
    lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
    return o;
}

lv_color_t avatar_color(int level) {
    static const uint32_t C[6] = {0xFFD54A, 0x7BC96F, 0xF1E7DC, 0xF28C28, 0x9C7A66, 0x8E6CD8};
    return lv_color_hex(C[level < 0 ? 0 : (level > 5 ? 5 : level)]);
}

Avatar avatar_create(lv_obj_t *parent, int level, int S) {
    Avatar a{};
    a.size = S;
    lv_obj_t *r = lv_obj_create(parent);
    lv_obj_remove_style_all(r);
    lv_obj_set_size(r, S, S);
    lv_obj_remove_flag(r, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(r, LV_OBJ_FLAG_SCROLLABLE);
    a.root = r;
    lv_color_t body = avatar_color(level);
    lv_color_t dark = lv_color_darken(body, 40);
    lv_color_t white = lv_color_white();
    auto P = [&](int pct) { return S * pct / 100; };

    // Detalles que van DETRÁS de la cabeza (orejas, cuernos, tuft)
    switch (level) {
        case 0: blob(r, P(44), P(2), P(12), P(16), dark, LV_RADIUS_CIRCLE); break;                       // pelusa
        case 2:                                                                                          // orejas largas
            blob(r, P(20), P(0), P(20), P(44), body, P(10)); blob(r, P(25), P(6), P(10), P(30), lv_color_hex(0xF4A6B8), P(6));
            blob(r, P(60), P(0), P(20), P(44), body, P(10)); blob(r, P(65), P(6), P(10), P(30), lv_color_hex(0xF4A6B8), P(6));
            break;
        case 3:                                                                                          // orejas de zorro
            blob(r, P(14), P(2), P(26), P(26), body, P(4), 450); blob(r, P(60), P(2), P(26), P(26), body, P(4), 450);
            break;
        case 4:                                                                                          // orejitas de búho
            blob(r, P(14), P(4), P(22), P(22), dark, P(4), 450); blob(r, P(64), P(4), P(22), P(22), dark, P(4), 450);
            break;
        case 5:                                                                                          // cuernos
            blob(r, P(18), P(2), P(16), P(26), lv_color_hex(0xFFF1C1), P(4), -250); blob(r, P(66), P(2), P(16), P(26), lv_color_hex(0xFFF1C1), P(4), 250);
            break;
    }
    // Cabeza
    lv_obj_t *head = blob(r, P(6), P(14), P(88), P(80), body);
    lv_obj_set_style_border_color(head, dark, 0);
    lv_obj_set_style_border_width(head, S >= 100 ? 4 : 3, 0);
    // Detalles sobre la cabeza
    if (level == 1) {  // manchas de caparazón
        blob(head, P(22), P(6), P(14), P(12), dark); blob(head, P(52), P(4), P(14), P(12), dark); blob(head, P(36), P(14), P(12), P(10), dark);
    }
    if (level == 3) {  // mofletes blancos
        blob(head, P(2), P(40), P(30), P(30), white); blob(head, P(52), P(40), P(30), P(30), white);
    }
    if (level == 4) {  // discos faciales
        blob(head, P(8), P(14), P(32), P(32), lv_color_hex(0xD7C2A8)); blob(head, P(44), P(14), P(32), P(32), lv_color_hex(0xD7C2A8));
    }
    blob(head, P(10), P(48), P(14), P(9), lv_color_hex(0xFF8FA3));  // coloretes
    blob(head, P(60), P(48), P(14), P(9), lv_color_hex(0xFF8FA3));
    lv_obj_set_style_bg_opa(lv_obj_get_child(head, -1), LV_OPA_70, 0);
    lv_obj_set_style_bg_opa(lv_obj_get_child(head, -2), LV_OPA_70, 0);

    // Ojos
    int ew = level == 4 ? P(26) : P(20), ey = level == 4 ? P(14) : P(22);
    int ex1 = level == 4 ? P(9) : P(16), ex2 = level == 4 ? P(47) : P(48);
    lv_obj_t *e1 = blob(head, ex1, ey, ew, ew, white), *e2 = blob(head, ex2, ey, ew, ew, white);
    if (level == 4) {
        lv_obj_set_style_border_color(e1, lv_color_hex(0xF5B800), 0); lv_obj_set_style_border_width(e1, 3, 0);
        lv_obj_set_style_border_color(e2, lv_color_hex(0xF5B800), 0); lv_obj_set_style_border_width(e2, 3, 0);
    }
    int pw = ew * 5 / 10;
    a.pupilL = blob(e1, (ew - pw) / 2, (ew - pw) / 2 + 1, pw, pw, lv_color_hex(0x1B1B2F));
    a.pupilR = blob(e2, (ew - pw) / 2, (ew - pw) / 2 + 1, pw, pw, lv_color_hex(0x1B1B2F));
    blob(a.pupilL, pw * 5 / 10, pw * 1 / 10, pw * 3 / 10, pw * 3 / 10, white);  // brillo
    blob(a.pupilR, pw * 5 / 10, pw * 1 / 10, pw * 3 / 10, pw * 3 / 10, white);

    // Pico / morro
    if (level == 0) blob(head, P(34), P(44), P(18), P(18), lv_color_hex(0xFF8A1F), P(3), 450);
    else if (level == 5) { blob(head, P(25), P(50), P(7), P(7), dark); blob(head, P(50), P(50), P(7), P(7), dark); }
    else if (level == 2) blob(head, P(34), P(44), P(16), P(11), lv_color_hex(0xF4A6B8));
    else if (level == 3) blob(head, P(34), P(46), P(16), P(12), lv_color_hex(0x2B2B2B));

    // Boca (arco)
    lv_obj_t *m = lv_arc_create(head);
    int mw = P(26);
    lv_obj_set_size(m, mw, mw);
    lv_obj_set_pos(m, P(30), level == 0 ? P(54) : P(50));
    lv_obj_remove_style(m, nullptr, LV_PART_KNOB);
    lv_obj_remove_flag(m, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_opa(m, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(m, lv_color_hex(0x3B1F1F), LV_PART_MAIN);
    lv_obj_set_style_arc_width(m, S >= 100 ? 5 : 3, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(m, true, LV_PART_MAIN);
    a.mouth = m;
    avatar_set_mood(a, Mood::Normal);
    return a;
}

void avatar_set_mood(Avatar &a, Mood m) {
    if (!a.mouth) return;
    int s = 20, e = 160;           // sonrisa (mitad inferior)
    int dx = 0, dy = 0;
    switch (m) {
        case Mood::Normal:    s = 35;  e = 145; break;
        case Mood::Thinking:  s = 60;  e = 120; dy = 1; break;   // boca pequeña torcida
        case Mood::Happy:     s = 10;  e = 170; break;
        case Mood::Sad:       s = 200; e = 340; dy = 4; break;   // mitad superior = triste
        case Mood::Surprised: s = 0;   e = 359; break;
    }
    lv_arc_set_bg_angles(a.mouth, s, e);
    lv_obj_set_style_translate_y(a.mouth, dy, 0);
    lv_obj_set_style_translate_x(a.mouth, dx, 0);
    // Las pupilas miran: pensando → arriba a un lado; triste → abajo
    int px = 0, py = 0;
    if (m == Mood::Thinking) { px = 3; py = -3; } else if (m == Mood::Sad) py = 3; else if (m == Mood::Happy) py = -1;
    lv_obj_set_style_translate_x(a.pupilL, px, 0); lv_obj_set_style_translate_y(a.pupilL, py, 0);
    lv_obj_set_style_translate_x(a.pupilR, px, 0); lv_obj_set_style_translate_y(a.pupilR, py, 0);
}
