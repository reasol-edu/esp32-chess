// Pantalla de juego: tablero, piezas, panel de la mascota, diálogos y confeti.
#pragma once
#include <lvgl.h>
#include "game_core.h"

// Puente con el motor de partida. En el dispositivo lo implementa GameService (tarea propia);
// en el simulador y en los tests, un host síncrono sobre GameCore.
struct GameHost {
    virtual ~GameHost() {}
    virtual void post_new(bool humanWhite, int level, bool coach, bool adaptive) = 0;
    virtual void post_human_move(const ce::Move &m) = 0;  // el host responde luego con la jugada de la CPU
    virtual void post_undo() = 0;
    virtual void post_hint() = 0;
    virtual void post_confirm() = 0;  // seguir adelante pese al aviso de peligro
    virtual void stop_search() = 0;
    // Copia el último snapshot publicado si es nuevo (distinto seq) y devuelve true.
    virtual bool poll(Snapshot &out) = 0;
};

void ui_game_create(lv_obj_t *screen, GameHost *host);

// Solo simulador/tests: acciones sintéticas.
void ui_game_debug_tap_square(int sq);
void ui_game_debug_open_new_dialog();
void ui_game_debug_tap(const char *what);  // "undo" | "hint" | "new" | "play" | "lvl:N" | "white" | "black" | "coach" | "adapt" | "promo:q|r|b|n"
