#include "game_service.h"
#include <esp_random.h>

void GameService::begin() {
    q_ = xQueueCreate(8, sizeof(Cmd));
    mu_ = xSemaphoreCreateMutex();
    core_.seed(esp_random());
    core_.setStore(&nvs_);
    core_.newGameFromPrefs();  // la última elección (modo, nivel, nombres…) sobrevive al reinicio
    publish();
    // Núcleo 1: LVGL y la E/S corren en el 0. Pila generosa: la búsqueda es recursiva.
    xTaskCreatePinnedToCore(taskEntry, "chess", 24 * 1024, this, 1, nullptr, 1);
}

void GameService::post(const Cmd &c) { xQueueSend(q_, &c, 0); }
void GameService::post_new(const NewGameOpts &o) { Cmd c{}; c.type = NEW; c.opts = o; post(c); }
void GameService::post_human_move(const ce::Move &m) { Cmd c{}; c.type = MOVE; c.move = m; post(c); }
void GameService::post_undo() { Cmd c{}; c.type = UNDO; post(c); }
void GameService::post_hint() { Cmd c{}; c.type = HINT; post(c); }
void GameService::post_confirm() { Cmd c{}; c.type = CONFIRM; post(c); }
void GameService::post_resign(bool white) { Cmd c{}; c.type = RESIGN; c.white = white; post(c); }
void GameService::post_save(int s) { Cmd c{}; c.type = SAVE; c.slot = s; post(c); }
void GameService::post_load(int s) { Cmd c{}; c.type = LOAD; c.slot = s; post(c); }
void GameService::post_delete(int s) { Cmd c{}; c.type = DELETE_; c.slot = s; post(c); }

void GameService::publish() {
    xSemaphoreTake(mu_, portMAX_DELAY);
    pub_ = core_.snap();
    pub_.seq = ++pubSeq_;
    xSemaphoreGive(mu_);
}

bool GameService::poll(Snapshot &out) {
    bool changed = false;
    xSemaphoreTake(mu_, portMAX_DELAY);
    if (pubSeq_ != seenSeq_) {
        out = pub_;
        seenSeq_ = pubSeq_;
        changed = true;
    }
    xSemaphoreGive(mu_);
    return changed;
}

// Si le toca a la CPU: el snapshot anterior ya mostró "piensa..."; ahora busca y publica la jugada.
void GameService::thinkIfNeeded() {
    if (!core_.cpuMayMove()) return;  // sin aviso de peligro pendiente
    uint32_t t0 = millis();
    core_.cpuMove();
    Serial.printf("[chess] CPU nivel %d: %lu ms\n", core_.snap().level, (unsigned long)(millis() - t0));
    publish();
}

void GameService::run() {
    Cmd c;
    for (;;) {
        if (xQueueReceive(q_, &c, portMAX_DELAY) != pdTRUE) continue;
        switch (c.type) {
            case NEW: core_.newGame(c.opts); break;
            case MOVE: core_.humanMove(c.move); break;
            case UNDO: core_.undo(); break;
            case HINT: core_.hint(); break;
            case CONFIRM: core_.confirm(); break;
            case RESIGN: core_.resign(c.white); break;
            case SAVE: core_.saveSlot(c.slot); break;
            case LOAD: core_.loadSlot(c.slot); break;
            case DELETE_: core_.deleteSlot(c.slot); break;
        }
        publish();
        thinkIfNeeded();
    }
}
