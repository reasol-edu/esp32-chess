// Aviso de pieza en peligro: tras la jugada del humano, ¿puede la CPU ganar material neto?
// Usa solo la API del motor (jugadas, capturas y deshacer) con una búsqueda de capturas acotada.
#pragma once
#include "chess_api.h"

struct Danger {
    bool found = false;
    int square = -1;  // casilla de la pieza del humano que se puede capturar con ganancia
    int loss = 0;     // material que se perdería (centipeones), ya descontadas las recapturas
};

constexpr int DANGER_MIN_LOSS = 300;  // una pieza menor; un peón suelto no avisa

// Precondición: le toca mover a la CPU. Deja el motor en la misma posición.
Danger analyze_danger(bool humanWhite);
