// Tarea del motor (núcleo 1): ejecuta las órdenes de la UI sobre GameCore y publica snapshots.
// El hilo de LVGL nunca toca el motor; solo llama a post_* y poll().
#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "ui_game.h"
#include "save_store_nvs.h"

class GameService : public GameHost {
public:
    void begin();
    void post_new(bool humanWhite, int level, bool coach, bool adaptive) override;
    void post_human_move(const ce::Move &m) override;
    void post_undo() override;
    void post_hint() override;
    void post_confirm() override;
    void post_save(int slot) override;
    void post_load(int slot) override;
    void post_delete(int slot) override;
    void stop_search() override { core_.stopSearch(); }
    bool poll(Snapshot &out) override;

private:
    enum Type : uint8_t { NEW, MOVE, UNDO, HINT, CONFIRM, SAVE, LOAD, DELETE_ };
    struct Cmd { Type type; bool white; int level; bool coach, adaptive; ce::Move move; int slot; };
    static void taskEntry(void *self) { ((GameService *)self)->run(); }
    void run();
    void post(const Cmd &c);
    void publish();
    void thinkIfNeeded();
    GameCore core_;
    NvsSaveStore nvs_;
    Snapshot pub_;
    uint32_t pubSeq_ = 0, seenSeq_ = 0;
    QueueHandle_t q_ = nullptr;
    SemaphoreHandle_t mu_ = nullptr;
};
