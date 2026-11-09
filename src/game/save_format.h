// Formato binario de una partida guardada: cabecera + jugadas (2 bytes c/u) + CRC16.
// La partida se reconstruye rejugando las jugadas desde la posición inicial con el motor, así que un
// guardado dañado o inválido se detecta (CRC y legalidad) sin tocar la partida en curso.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "chess_api.h"
#include "names.h"

constexpr int SAVE_MAX_PLIES = 1000;
constexpr size_t SAVE_HEADER_BYTES = 8 + 2 * NAME_LEN;
constexpr size_t SAVE_MAX_BYTES = SAVE_HEADER_BYTES + 2 * SAVE_MAX_PLIES + 2;

struct SaveHeader {
    bool humanWhite = true, coach = true, adaptive = true, twoPlayer = false;
    uint8_t level = 0;
    int8_t adj = 0;      // ajuste del modo adaptable
    uint16_t plies = 0;
    char name1[NAME_LEN] = {0}, name2[NAME_LEN] = {0};  // jugador 1 (blancas en 2 jugadores) y jugador 2
};

// Resumen de un hueco para mostrarlo en la UI.
struct SlotInfo {
    bool used = false;
    bool humanWhite = true;
    bool twoPlayer = false;
    uint8_t level = 0;
    uint16_t plies = 0;
    char name1[NAME_LEN] = {0}, name2[NAME_LEN] = {0};
};

// Minutos por jugador que ofrece el reloj de dos jugadores (0 = sin reloj).
constexpr uint8_t CLOCK_MINUTES[] = {0, 3, 5, 10, 15};
constexpr int CLOCK_CHOICES_N = sizeof CLOCK_MINUTES / sizeof CLOCK_MINUTES[0];

// Preferencias de la última partida: se recuerdan entre arranques.
struct GamePrefs {
    bool humanWhite = true, coach = true, adaptive = true, twoPlayer = false;
    bool faceToFace = true;   // dos jugadores: false = se pasan el aparato (sin giros)
    uint8_t clockMin = 0;     // reloj de dos jugadores
    uint8_t level = 1;
    char name1[NAME_LEN] = {0}, name2[NAME_LEN] = {0};
};
size_t prefs_encode(const GamePrefs &p, uint8_t *buf, size_t cap);   // longitud o 0
bool prefs_decode(const uint8_t *buf, size_t len, GamePrefs &out);

uint16_t save_crc16(const uint8_t *p, size_t n);

class SaveWriter {
public:
    SaveWriter(uint8_t *buf, size_t cap) : b_(buf), cap_(cap) {}
    bool begin(const SaveHeader &h);
    bool add(const ce::Move &m);
    size_t finish();  // añade el CRC; devuelve la longitud total o 0 si hubo error
private:
    uint8_t *b_;
    size_t cap_, len_ = 0;
    uint16_t n_ = 0;
    bool ok_ = true;
};

class SaveReader {
public:
    bool parse(const uint8_t *buf, size_t len);  // valida marca, versión, tamaño y CRC
    const SaveHeader &header() const { return h_; }
    ce::Move move(int i) const;
private:
    const uint8_t *b_ = nullptr;
    SaveHeader h_;
};
