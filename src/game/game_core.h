// Lógica de una partida humano–CPU sobre el motor (namespace ce). No usa LVGL ni FreeRTOS: en el
// dispositivo la envuelve GameService (una tarea), en el simulador y en los tests se llama directo.
// No es reentrante.
#pragma once
#include "chess_api.h"
#include "difficulty.h"
#include "adaptive.h"
#include "danger.h"
#include "save_format.h"
#include "save_store.h"

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
    int level;       // nivel elegido
    int effLevel;    // nivel efectivo (distinto si el modo adaptable ha ajustado)
    bool coach, adaptive;
    bool awaiting;   // aviso de peligro pendiente: la CPU espera la decisión del humano
    int dangerSq, dangerLoss;
    int adaptEvent;  // −1 afloja, +1 aprieta (solo en el snapshot que lo provoca)
    SlotInfo slots[SAVE_SLOTS];  // resumen de las partidas guardadas
    int lastOp;      // última operación de guardado: 1 guardada, 2 cargada, 3 borrada; −1/−2/−3 fallo de cada una
    bool hasHint;
    ce::Move hint;
    uint32_t seq;  // se incrementa con cada cambio
};

class GameCore {
public:
    GameCore();
    void seed(uint32_t s) { rng_ = s ? s : 1; }
    void newGame(bool humanWhite, int level, bool coach = true, bool adaptive = true);
    bool loadFen(const char *fen, bool humanWhite, int level);  // posición arbitraria (tests, problemas)
    bool humanToMove() const { return snap_.whiteToMove == snap_.humanWhite && !over(); }
    bool cpuToMove() const { return snap_.whiteToMove != snap_.humanWhite && !over(); }
    bool cpuMayMove() const { return cpuToMove() && !awaiting_; }  // turno de la CPU y sin aviso pendiente
    bool confirm();
    // Partidas guardadas (3 huecos). Sin almacén asignado, todo falla sin efecto.
    void setStore(SaveStore *s) { store_ = s; refreshSlots(); }
    void refreshSlotsForTest() { refreshSlots(); }  // si el almacén se modifica por fuera
    bool saveSlot(int slot);    // guarda la partida actual
    bool loadSlot(int slot);    // sustituye la actual; si el hueco es inválido, la actual no cambia
    bool deleteSlot(int slot);                     // el humano sigue adelante pese al aviso
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
    bool coach_ = true, adaptive_ = true, awaiting_ = false, ended_ = false;
    int event_ = 0;
    Danger danger_;
    Adaptive ad_;
    void checkEnd();
    void refreshSlots();
    SaveStore *store_ = nullptr;
    SlotInfo slots_[SAVE_SLOTS];
    bool fromFen_ = false;  // posición inicial no estándar: no se puede guardar
    int op_ = 0;
};
