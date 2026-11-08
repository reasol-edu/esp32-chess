// Lógica de una partida humano–CPU sobre el motor (namespace ce). No usa LVGL ni FreeRTOS: en el
// dispositivo la envuelve GameService (una tarea), en el simulador y en los tests se llama directo.
// No es reentrante.
#pragma once
#include "chess_api.h"
#include "difficulty.h"

struct Snapshot {
    int8_t board[64];
    bool whiteToMove;
    ce::Status status;
    int plies;
    bool hasLast;
    ce::Move last;
    int nLegal;
    ce::Move legal[256];
    uint8_t missingWhite[6];  // piezas blancas capturadas por tipo (P N B R Q K=0)
    uint8_t missingBlack[6];
    bool humanWhite;
    int level;
    bool hasHint;
    ce::Move hint;
    uint32_t seq;  // se incrementa con cada cambio
};

class GameCore {
public:
    GameCore();
    void seed(uint32_t s) { rng_ = s ? s : 1; }
    void newGame(bool humanWhite, int level);
    bool loadFen(const char *fen, bool humanWhite, int level);  // posición arbitraria (tests, problemas)
    bool humanToMove() const { return snap_.whiteToMove == snap_.humanWhite && !over(); }
    bool cpuToMove() const { return snap_.whiteToMove != snap_.humanWhite && !over(); }
    bool over() const { return snap_.status >= ce::ST_MATE; }
    bool humanMove(const ce::Move &m);  // jugada del humano (debe ser legal)
    bool cpuMove(ce::Move *played = nullptr);  // calcula y juega la respuesta de la CPU
    bool undo();                        // deshace hasta devolver el turno al humano
    bool hint();                        // calcula una pista para el humano
    void stopSearch() { ce::stop(); }
    const Snapshot &snap() const { return snap_; }

private:
    uint32_t next();
    void refresh();
    Snapshot snap_;
    uint32_t rng_;
};
