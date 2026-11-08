#include "game_service.h"
#include <esp_random.h>

void GameService::begin() {
    q_ = xQueueCreate(8, sizeof(Cmd));
    mu_ = xSemaphoreCreateMutex();
    core_.seed(esp_random());
    core_.newGame(true, 1);
    publish();
    // Núcleo 1: LVGL y la E/S corren en el 0. Pila generosa: la búsqueda es recursiva.
    xTaskCreatePinnedToCore(taskEntry, "chess", 24 * 1024, this, 1, nullptr, 1);
}

void GameService::post(const Cmd &c) { xQueueSend(q_, &c, 0); }
void GameService::post_new(bool w, int l) { post({NEW, w, l, {}}); }
void GameService::post_human_move(const ce::Move &m) { post({MOVE, false, 0, m}); }
void GameService::post_undo() { post({UNDO, false, 0, {}}); }
void GameService::post_hint() { post({HINT, false, 0, {}}); }

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
    if (!core_.cpuToMove()) return;
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
            case NEW: core_.newGame(c.white, c.level); break;
            case MOVE: core_.humanMove(c.move); break;
            case UNDO: core_.undo(); break;
            case HINT: core_.hint(); break;
        }
        publish();
        thinkIfNeeded();
    }
}
