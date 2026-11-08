// Mascotas (un animal por nivel) dibujadas solo con objetos de LVGL: sin imágenes ni emojis.
#pragma once
#include <lvgl.h>

enum class Mood : uint8_t { Normal, Thinking, Happy, Sad, Surprised };

struct Avatar {
    lv_obj_t *root;
    lv_obj_t *mouth, *pupilL, *pupilR;
    int size;
};

// Crea el avatar del nivel `level` (0..5) dentro de `parent`; ocupa size x size píxeles.
Avatar avatar_create(lv_obj_t *parent, int level, int size);
void avatar_set_mood(Avatar &a, Mood m);
lv_color_t avatar_color(int level);
