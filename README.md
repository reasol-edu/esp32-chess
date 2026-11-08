# esp32-chess

Ajedrez táctil para la **Guition JC1060P470** (ESP32-P4, pantalla 1024×600), pensado para que puedan jugar niños.
Usa **LVGL 9.6** con la unidad de dibujo **PPA** del ESP32-P4 y el motor de
[hpsaturn/esp32-chess-engine](https://github.com/hpsaturn/esp32-chess-engine).

> **Estado: en desarrollo.** El esqueleto de pantalla y el motor están hechos; la interfaz del juego todavía no.

## Estado

| Fase | Contenido | Estado |
|---|---|---|
| 1 | Esqueleto: pantalla DSI, táctil GT911, LVGL 9.6, PPA | Compila; **sin probar en la placa** |
| 2 | Motor de ajedrez como librería, con tests en el PC | Hecho (13 tests) |
| 3 | UI: tablero, piezas, jugadas legales, coronación | Pendiente |
| 4 | Partida contra la CPU (motor en otra tarea) | Pendiente |
| 5 | Animaciones, mascota, medición y uso del PPA | Pendiente |
| 6 | Niveles de dificultad para niños, pistas, avisos | Pendiente |
| 7 | Guardar partida, 2 jugadores | Opcional |

Hoy `src/main.cpp` es solo una pantalla de prueba: un tablero 8×8 que indica la casilla tocada.

## Plan de juego

- **Dificultad por animales**, de Pollito (movimientos casi al azar) a Dragón (motor a máxima profundidad). Los niveles se ajustarán con medidas reales en la placa.
- **Ayudas para niños:** puntos en las casillas legales, pistas, deshacer ilimitado y aviso de pieza en peligro.
- **Muy visual:** tablero de madera, piezas animadas, mascota que reacciona y confeti al ganar.
- **PPA** para rellenos, mezclas y escalado de sprites. Su beneficio se medirá comparando con el entorno sin PPA.

## Compilar y probar

PlatformIO no suele estar en el `PATH`: `~/.platformio/penv/bin/pio`.

```bash
pio run -e jc1060p470 -t upload -t monitor    # con PPA
pio run -e jc1060p470_noppa                   # misma app sin PPA, para comparar
pio test -e native                            # tests del motor en el PC
```

Al arrancar, el monitor serie debe mostrar `LVGL 9.6.0 PPA=1`.

## Estructura

```
src/main.cpp               Pantalla de prueba (DSI + táctil + LVGL)
src/engine/engine_core.cpp Motor de ajedrez adaptado como librería
src/engine/chess_api.h     API pública del motor (namespace ce)
src/psram_alloc.cpp        Montón de LVGL en PSRAM
src/ppa_cache_shim.cpp     Adaptador de caché que necesita el PPA de LVGL
src/fonts/                 Montserrat con glifos españoles
include/                   lv_conf.h, touch.h, ppa_cache_shim.h
scripts/patch_gfx_p4.py    Parche de GFX Library (doble framebuffer DMA2D)
test/                      Tests nativos del motor y shim de Arduino
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

## Licencia

El motor es **GPL v3** (ver `LICENSE-engine-GPLv3`), así que el firmware que lo incluya también debe distribuirse bajo GPL v3.
