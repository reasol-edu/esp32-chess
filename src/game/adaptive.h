// Modo adaptable: ajusta la dificultad efectiva de la CPU según el balance de material y los resultados.
// adj > 0 = más difícil, adj < 0 = más fácil; rango ±2 niveles sobre el nivel elegido. Lógica pura.
#pragma once
#include "difficulty.h"

struct Adaptive {
    static constexpr int MAX_ADJ = 2;
    static constexpr int MATERIAL_GAP = 300;  // centipeones: una pieza menor de diferencia
    static constexpr int COOLDOWN = 4;        // jugadas de la CPU entre dos ajustes

    int adj = 0;
    int cooldown = 0;

    // humanMinusCpu: material del humano menos el de la CPU. Devuelve −1 (afloja), 0 o +1 (aprieta).
    int onMaterial(int humanMinusCpu) {
        if (cooldown > 0) { cooldown--; return 0; }
        if (humanMinusCpu <= -MATERIAL_GAP && adj > -MAX_ADJ) { adj--; cooldown = COOLDOWN; return -1; }
        if (humanMinusCpu >= MATERIAL_GAP && adj < MAX_ADJ) { adj++; cooldown = COOLDOWN; return +1; }
        return 0;
    }
    // Al acabar la partida el ajuste pasa a la siguiente: si gana el humano, sube; si pierde, baja.
    void onGameEnd(bool humanWon, bool humanLost) {
        if (humanWon && adj < MAX_ADJ) adj++;
        else if (humanLost && adj > -MAX_ADJ) adj--;
        cooldown = 0;
    }
    int effective(int baseLevel) const {
        int l = baseLevel + adj;
        return l < 0 ? 0 : (l >= LEVEL_COUNT ? LEVEL_COUNT - 1 : l);
    }
};
