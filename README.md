# esp32-chess

Ajedrez táctil para la **Guition JC1060P470** (ESP32-P4, pantalla 1024×600), pensado para que puedan jugar niños.
Usa **LVGL 9.6** con la unidad de dibujo **PPA** del ESP32-P4 y el motor de
[hpsaturn/esp32-chess-engine](https://github.com/hpsaturn/esp32-chess-engine).

> **Estado: listo para probar en la placa.** Todo compila (con y sin PPA) y la UI se ha revisado con capturas del simulador,
> pero **todavía no se ha ejecutado en el hardware real**.

## Estado

| Fase | Contenido | Estado |
|---|---|---|
| 1 | Esqueleto: pantalla DSI, táctil GT911, LVGL 9.6, PPA | Compila; sin probar en la placa |
| 2 | Motor de ajedrez como librería, con tests en el PC | Hecho (13 tests) |
| 3 | UI: tablero, piezas, jugadas legales, coronación | Hecho; revisada en el simulador |
| 4 | Partida contra la CPU (motor en el núcleo 1) | Hecho; sin probar en la placa |
| 5 | Animaciones, mascotas, confeti, sprites con el PPA | Hecho; falta medir en la placa |
| 6 | Dificultad: 6 niveles, deshacer, pista, aviso de pieza en peligro y modo adaptable | Hecho; sin probar en la placa |
| 7 | Guardar y cargar partidas (3 huecos) | Hecho; sin probar en la placa |
| 8 | Modo 2 jugadores cara a cara y nombres | Hecho; sin probar en la placa |

## Plan de juego

- **Dificultad por animales**, de Pollito (movimientos casi al azar) a Dragón (motor a máxima profundidad). Los niveles se ajustarán con medidas reales en la placa.
- **Ayudas para niños:** puntos en las casillas legales, pistas y deshacer ilimitado.
- **Aviso de pieza en peligro** (desactivable): si tras mover la CPU puede ganar al menos una pieza menor neta, la CPU espera y
  la pantalla ofrece *Deshacer* o *Seguir*. Un peón suelto o un cambio igualado no avisan.
- **Modo adaptable** (desactivable): afloja o aprieta hasta ±2 niveles según el balance de material y arrastra el resultado a la
  siguiente partida. Los puntos bajo la mascota muestran el nivel efectivo (en cian si difiere del elegido).
  Ambos se eligen en el diálogo de nueva partida y empiezan activados.
- **Dos jugadores cara a cara** (*Dos jugadores* en el diálogo de nueva partida): el aparato se deja en la mesa entre los dos.
  Quien juega con negras se sienta al otro lado: su panel (nombre, turno, capturas y *Rendirse*), el cartel de fin y el diálogo de
  coronación salen girados 180°, y sus piezas se dibujan giradas (con el PPA, SRM con rotación de 180°; si falla, por software).
  Sin CPU, sin avisos y sin modo adaptable; *Deshacer* retira una jugada y *Rendirse* pide confirmación.
- **Nombres** (opcionales, hasta 10 caracteres): se escriben con un teclado en pantalla con ñ y vocales con tilde. Se recuerdan
  entre arranques junto con la última elección (modo, nivel, color, avisos, adaptable) y se guardan con cada partida. Solo
  viven en la memoria del aparato; no se envían a ningún sitio. Sin nombre, se muestra "Blancas"/"Negras" (o "Tú" en una inicial).
- **Guardar y cargar** (botón *Partidas*): 3 huecos en la memoria NVS del chip, con la mascota, las jugadas y el color. Sobrescribir,
  borrar o cargar sobre una partida en curso piden confirmación con un segundo toque. Se guardan los ajustes de la partida (nivel,
  avisos, adaptable) y se rejuega desde el inicio, así que *Deshacer* sigue funcionando; un hueco dañado (CRC) o con jugadas ilegales
  se rechaza sin tocar la partida actual. Una partida de 96 jugadas ocupa unos 200 bytes. No hay guardado automático.
- **Muy visual:** tablero de madera, piezas animadas, mascota que reacciona y confeti al ganar.
- **PPA** para rellenos, mezclas y escalado de sprites. Su beneficio se medirá comparando con el entorno sin PPA.

## Compilar y probar

PlatformIO no suele estar en el `PATH`: `~/.platformio/penv/bin/pio`.

```bash
pio run -e jc1060p470 -t upload -t monitor    # con PPA
pio run -e jc1060p470_noppa                   # misma app sin PPA, para comparar
pio test -e native                            # tests del motor y de la lógica de partida en el PC
tools/sim/shot.sh mid                         # captura de la UI en el PC (sin placa): ver tools/sim/shot.sh
```

Al arrancar, el monitor serie debe mostrar `[PPA] 24 sprites generados con SRM en … us` y `LVGL 9.6.0 PPA=1 sprites_ppa=1`.
Si el PPA falla, la UI escala por software y lo indica (`sprites_ppa=0`). Cada jugada de la CPU registra su tiempo (`[chess] CPU nivel N: X ms`).

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

## Licencia

El motor es **GPL v3** (ver `LICENSE-engine-GPLv3`), así que el firmware que lo incluya también debe distribuirse bajo GPL v3.
