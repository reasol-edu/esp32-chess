#include "game_service.h"
#include <esp_random.h>

void GameService::begin() {
    q_ = xQueueCreate(8, sizeof(Cmd));
    mu_ = xSemaphoreCreateMutex();
    core_.seed(esp_random());
    core_.setStore(&nvs_);
    core_.newGame(true, 1);
    publish();
    // Núcleo 1: LVGL y la E/S corren en el 0. Pila generosa: la búsqueda es recursiva.
    xTaskCreatePinnedToCore(taskEntry, "chess", 24 * 1024, this, 1, nullptr, 1);
}

void GameService::post(const Cmd &c) { xQueueSend(q_, &c, 0); }
void GameService::post_new(bool w, int l, bool coach, bool adaptive) { post({NEW, w, l, coach, adaptive, {}, 0}); }
void GameService::post_human_move(const ce::Move &m) { post({MOVE, false, 0, false, false, m, 0}); }
void GameService::post_undo() { post({UNDO, false, 0, false, false, {}, 0}); }
void GameService::post_hint() { post({HINT, false, 0, false, false, {}, 0}); }
void GameService::post_confirm() { post({CONFIRM, false, 0, false, false, {}, 0}); }
void GameService::post_save(int s) { post({SAVE, false, 0, false, false, {}, s}); }
void GameService::post_load(int s) { post({LOAD, false, 0, false, false, {}, s}); }
void GameService::post_delete(int s) { post({DELETE_, false, 0, false, false, {}, s}); }

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
            case NEW: core_.newGame(c.white, c.level, c.coach, c.adaptive); break;
            case MOVE: core_.humanMove(c.move); break;
            case UNDO: core_.undo(); break;
            case HINT: core_.hint(); break;
            case CONFIRM: core_.confirm(); break;
            case SAVE: core_.saveSlot(c.slot); break;
            case LOAD: core_.loadSlot(c.slot); break;
            case DELETE_: core_.deleteSlot(c.slot); break;
        }
        publish();
        thinkIfNeeded();
    }
}
