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

// Opciones de una partida nueva.
struct NewGameOpts {
    bool humanWhite = true;
    int level = 1;
    bool coach = true, adaptive = true;
    bool twoPlayer = false;                        // dos jugadores en el mismo dispositivo, sin CPU
    char name1[NAME_LEN] = {0}, name2[NAME_LEN] = {0};  // jugador 1 (blancas en 2 jugadores) y jugador 2
};

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
    bool twoPlayer;
    bool over;       // fin de partida (mate, tablas o rendición)
    int resigned;    // 0 nadie, 1 se rindieron las blancas, 2 las negras
    char name1[NAME_LEN], name2[NAME_LEN];
    GamePrefs prefs; // últimas opciones elegidas (para precargar el diálogo)
    int lastOp;      // última operación de guardado: 1 guardada, 2 cargada, 3 borrada; −1/−2/−3 fallo de cada una
    bool hasHint;
    ce::Move hint;
    uint32_t seq;  // se incrementa con cada cambio
};

class GameCore {
public:
    GameCore();
    void seed(uint32_t s) { rng_ = s ? s : 1; }
    void newGame(const NewGameOpts &o);
    void newGame(bool humanWhite, int level, bool coach = true, bool adaptive = true);
    bool resign(bool white);            // rendición de un bando
    bool loadFen(const char *fen, bool humanWhite, int level);  // posición arbitraria (tests, problemas)
    bool humanToMove() const { return !over() && (twoPlayer_ || snap_.whiteToMove == snap_.humanWhite); }  // en 2 jugadores siempre le toca a una persona
    bool cpuToMove() const { return !twoPlayer_ && snap_.whiteToMove != snap_.humanWhite && !over(); }
    bool cpuMayMove() const { return cpuToMove() && !awaiting_; }  // turno de la CPU y sin aviso pendiente
    bool confirm();
    // Partidas guardadas (3 huecos). Sin almacén asignado, todo falla sin efecto.
    void setStore(SaveStore *s) { store_ = s; loadPrefs(); refreshSlots(); }
    void refreshSlotsForTest() { refreshSlots(); }  // si el almacén se modifica por fuera
    bool saveSlot(int slot);    // guarda la partida actual
    bool loadSlot(int slot);    // sustituye la actual; si el hueco es inválido, la actual no cambia
    bool deleteSlot(int slot);                     // el humano sigue adelante pese al aviso
    bool over() const { return snap_.status >= ce::ST_MATE || resigned_ != 0; }
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
    bool fromFen_ = false, twoPlayer_ = false;
    int resigned_ = 0;
    char name1_[NAME_LEN] = {0}, name2_[NAME_LEN] = {0};
    GamePrefs prefs_;
    void loadPrefs();  // posición inicial no estándar: no se puede guardar
    int op_ = 0;
};
