# esp32-chess

Ajedrez táctil para la **Guition JC1060P470** (ESP32-P4, pantalla 1024×600), pensado para que puedan jugar niños.
Usa **LVGL 9.6** con la unidad de dibujo **PPA** del ESP32-P4 y el motor de
[hpsaturn/esp32-chess-engine](https://github.com/hpsaturn/esp32-chess-engine).

> **Estado: probado en la placa (JC1060P470, ESP32-P4 rev v3.2, panel V2):** arranca, el PPA funciona (24 sprites con SRM en 18,7 ms)
> y el táctil responde. Falta medir el rendimiento (PPA frente a sin PPA) y probar partidas largas, guardado y dos jugadores.

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
- **Pasar el aparato** (botón de modo en el diálogo de dos jugadores, *Cara a cara* por defecto): no se gira nada, ni paneles,
  carteles, coronación ni piezas; los dos juegan desde el mismo lado y se pasan la pantalla. El tablero no cambia de orientación.
  Se recuerda entre arranques.
- **Reloj de ajedrez** (opcional, solo en dos jugadores; botón *Reloj* del diálogo: no, 3, 5, 10 o 15 minutos por jugador). Corre
  para quien tiene el turno desde la primera jugada de las blancas, se detiene con cualquier diálogo abierto y, al agotarse,
  pierde ese jugador (cartel "Se acabó el tiempo de …"). Los últimos 20 s se marcan en rojo. Se recuerda entre arranques,
  pero no va dentro de la partida guardada: una partida cargada empieza con el reloj lleno.
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

## La placa: revisiones

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

## Licencia

El motor es **GPL v3** (ver `LICENSE-engine-GPLv3`), así que el firmware que lo incluya también debe distribuirse bajo GPL v3.
