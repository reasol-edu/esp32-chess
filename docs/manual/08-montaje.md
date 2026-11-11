# Montar y programar el aparato

Este anexo es para **personas adultas** que quieran fabricar el aparato o modificar el programa. Para jugar no hace falta nada de esto.

## Qué necesitas

- Una placa **Guition JC1060P470** (ESP32-P4, pantalla táctil de 7" a 1024×600).
- Un cable USB-C para conectarla al ordenador.
- [PlatformIO](https://platformio.org) (en macOS suele estar en `~/.platformio/penv/bin/pio`).
- Un entorno de Python para instalar una versión concreta de `esptool` (ver más abajo).

## Compilar y probar

PlatformIO no suele estar en el `PATH`: `~/.platformio/penv/bin/pio`.

```bash
pio run -e jc1060p470                         # compila (con PPA)
ESPTOOL=<venv>/bin/esptool tools/flash.sh     # sube a la placa (ver «La placa: revisiones»)
pio run -e jc1060p470_noppa                   # misma app sin PPA, para comparar
pio test -e native                            # tests del motor y de la lógica de partida en el PC
tools/sim/shot.sh mid                         # captura de la UI en el PC (sin placa): ver tools/sim/shot.sh
```

Al arrancar, el monitor serie debe mostrar `[PPA] 24 sprites generados con SRM en … us` y `LVGL 9.6.0 PPA=1 sprites_ppa=1`.
Si el PPA falla, la UI escala por software y lo indica (`sprites_ppa=0`). Cada jugada de la CPU registra su tiempo (`[chess] CPU nivel N: X ms`).

## La placa: revisiones

| | Placa actual (lote 2026) | Placa original |
|---|---|---|
| Chip | ESP32-P4 rev v3.x | ESP32-P4 «ES pre rev.300» |
| Panel | «V2» (etiqueta en la trasera) | Antiguo |
| Entorno de PlatformIO | `jc1060p470` | `jc1060p470_v1` |
| Estado | Probado | **Sin probar en hardware** (compila) |

Para grabar la placa original: `pio run -e jc1060p470_v1` y `PIO_ENV=jc1060p470_v1 ESPTOOL=... tools/flash.sh`. Las releases incluyen su firmware con el sufijo `-placa-original`. Chip y panel son independientes: si tu combinación es otra (por ejemplo chip v3 con panel antiguo), añade `-DPANEL_JC1060P470_V1` al entorno que corresponda.

- **Chip ESP32-P4 rev v3.x** (el de los lotes nuevos): hay que compilar con `board = esp32-p4_r3-evboard`. La variante
  `esp32-p4-evboard` ("ES pre rev.300") es para el silicio antiguo y en un v3 no arranca (pantalla negra, sin puerto USB).
- **Subir el firmware:** `pio run -t upload` falla con el esptool 5.2 que trae PlatformIO (el stub se cuelga con
  "Guru Meditation Error"; [FAQ de Espressif](https://github.com/espressif/esp-faq/blob/master/docs/en/development-environment/firmware-update.rst)).
  Instala `esptool==5.3.dev3` en un venv y usa `ESPTOOL=.../esptool tools/flash.sh`.
- **Panel "V2"** (lote 2026, etiqueta V2 en la trasera): otros tiempos, otra secuencia de inicio (`src/jd9165_v2.h`) y reset del
  panel en GPIO0. Es el predeterminado; para el panel antiguo compila con `-DPANEL_JC1060P470_V1`.
  Ajustes que quedan: 52 MHz de reloj de píxel, 750 Mbps por carril y RGB565 (COLMOD 0x55 en la secuencia de inicio).
  Durante la puesta a punto se vieron rayas verticales tenues en grises medios que desaparecieron sin que se aislara la causa
  (se probó carril 600/900, reloj 40/48 MHz y `disable_lp`, sin efecto o peor); si reaparecen, empezar por ahí.

## Estructura

```
src/main.cpp               Arranque: pantalla DSI, táctil, LVGL, PPA y UI
src/game_service.*         Tarea del motor (núcleo 1) y cola de órdenes
src/game/                  Lógica de partida (GameCore), niveles, aviso de peligro, modo adaptable y formato de guardado; sin LVGL ni FreeRTOS
src/save_store_nvs.*       Almacén de partidas guardadas en NVS
src/engine/                Motor de ajedrez adaptado como librería (namespace ce)
src/ui/ui_game.cpp         Pantalla de juego: tablero, paneles (1 y 2 jugadores), diálogos, teclado de nombres, confeti
src/ui/ui_avatar.cpp       Mascotas dibujadas con objetos LVGL
src/ui/ppa_fx.cpp          Sprites levantados y miniaturas escalados con el PPA
src/ui/pieces_data.c       Sprites cburnett 64x64 (generado por tools/gen_pieces.py)
src/psram_alloc.cpp        Montón de LVGL en PSRAM
src/ppa_cache_shim.cpp     Adaptador de caché que necesita el PPA de LVGL
include/                   lv_conf.h, touch.h, ppa_cache_shim.h
tools/sim/                 Simulador de la UI en el PC (capturas sin hardware)
test/                      Tests nativos del motor y de la lógica de partida
assets/pieces/             SVG de las piezas y su licencia
```

La inicialización de pantalla, táctil y PPA procede del proyecto
[deye-monitor](https://github.com/lrlopez/deye-monitor) (rama `exp/ppa-bench`).

## El motor

Port del motor de Sergey Urusov a través de `hpsaturn/esp32-chess-engine`. Cambios respecto al original:

- Es una librería: envuelta en `namespace ce`, sin `Serial`, `setup`, `loop` ni pruebas ELO/WAC.
- La tarea auxiliar, que giraba al 100 % de CPU esperando trabajo, es ahora una llamada síncrona.
- Tiene un tope de profundidad configurable, además del límite de tiempo.

API (`src/engine/chess_api.h`): `newGame`, `setFen`, `legalMoves`, `play`, `undoPly`, `status`
(mate, ahogado, material insuficiente, repetición triple, 50 jugadas) y `search`, que devuelve la mejor jugada sin jugarla.
No es reentrante: debe usarse siempre desde una sola tarea.

Los 13 tests cubren perft (20/400/8902 desde el inicio y 48/2039 en Kiwipete), enroque, captura al paso,
coronación, mate, ahogado, repetición, deshacer y tiempos por profundidad.


## Licencia y créditos

El motor de ajedrez es obra de **Sergey Urusov**, en la versión de [hpsaturn/esp32-chess-engine](https://github.com/hpsaturn/esp32-chess-engine), con licencia **GPL v3**. Por eso el firmware completo se distribuye también bajo GPL v3. Las piezas son el set *cburnett* de Colin M. L. Burnett (GPLv2+). La lista completa de créditos y licencias de terceros está en [`NOTICE.md`](https://github.com/reasol-edu/esp32-chess/blob/main/NOTICE.md).

## Firmware ya compilado

Cada versión publicada incluye en [Releases](https://github.com/reasol-edu/esp32-chess/releases) el firmware compilado por una GitHub Action:

- `esp32-chess-<versión>-app.bin`: solo el programa; se graba en `0x10000` y **conserva** las partidas guardadas.
- `esp32-chess-<versión>-completo.bin`: imagen única (cargador, particiones y programa); se graba en `0x0` y **borra** las partidas guardadas.

```bash
esptool --chip esp32p4 write-flash 0x10000 esp32-chess-<versión>-app.bin
```

Con una placa ESP32-P4 rev v3.2 hace falta esptool 5.3 o posterior (ver «La placa: revisiones»).
