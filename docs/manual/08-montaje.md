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

## Grabar el firmware ya compilado

No hace falta instalar PlatformIO ni compilar nada: cada versión publicada incluye en [Releases](https://github.com/reasol-edu/esp32-chess/releases) el firmware compilado por una GitHub Action (para la placa original, con el sufijo `-placa-original`):

| Fichero | Qué es | Dirección | Partidas guardadas |
|---|---|---|---|
| `esp32-chess-<versión>-completo.bin` | Imagen única: cargador, particiones y programa | `0x0` | **Se borran** |
| `esp32-chess-<versión>-app.bin` | Solo el programa | `0x10000` | Se conservan |

**La primera vez** (o si la placa no arranca) usa el fichero `completo`. El `app` solo sirve para **actualizar** una placa que ya lo tenía.

Hay dos formas de grabarlo, con la placa conectada por USB al conector que sirve para programar. Comprueba antes la versión de tu placa en [La placa: revisiones](#la-placa-revisiones).

### Opción A: desde el navegador (sin instalar nada)

Con [esptool-js](https://espressif.github.io/esptool-js/), la herramienta web de Espressif:

1. Abre <https://espressif.github.io/esptool-js/> en **Google Chrome o Microsoft Edge** (usan *Web Serial*; **Safari y Firefox no funcionan**).
2. Pulsa **Connect** y elige el puerto de la placa en la ventana del navegador.
3. En *Program*, pon la dirección `0x0` y elige el fichero `…-completo.bin` (o `0x10000` y el `…-app.bin` para solo actualizar).
4. Deja la velocidad en `921600` (o baja a `460800` si falla) y pulsa **Program**.
5. Al acabar, pulsa **Reset** o desenchufa y vuelve a enchufar la placa.

!!! warning "Sin probar con esta placa"
    La herramienta web de Espressif declara soporte para el ESP32-P4 y, desde su versión 0.7.0, corrige un fallo con las revisiones v3.1 y v3.2 del chip. Pero **no la he probado** con esta placa: la grabación que sí he verificado es la de la opción B. Si la web falla, usa la B.

### Opción B: desde la línea de comandos (Windows, macOS y Linux)

Funciona igual en los tres sistemas con **esptool**. Con un chip ESP32-P4 rev v3.2 hace falta **esptool 5.3 o posterior** (la 5.2 se cuelga al grabar; ver «La placa: revisiones»). Necesitas Python 3.10 o superior.

#### Windows

En *PowerShell*:

```powershell
py -m venv esp
esp\Scripts\activate
pip install --pre "esptool==5.3.dev3"
```

Mira el puerto en el *Administrador de dispositivos › Puertos (COM y LPT)*: será algo como `COM5`. Windows 10 y 11 no necesitan controlador. Después:

```powershell
esptool --chip esp32p4 --port COM5 --baud 921600 write-flash 0x0 esp32-chess-<versión>-completo.bin
```

#### macOS

En *Terminal*:

```bash
python3 -m venv esp
source esp/bin/activate
pip install --pre "esptool==5.3.dev3"
ls /dev/cu.usbmodem*          # el puerto de la placa
esptool --chip esp32p4 --port /dev/cu.usbmodemXXXX --baud 921600 write-flash 0x0 esp32-chess-<versión>-completo.bin
```

#### Linux

```bash
python3 -m venv esp
source esp/bin/activate
pip install --pre "esptool==5.3.dev3"
ls /dev/ttyACM*               # el puerto de la placa, normalmente /dev/ttyACM0
esptool --chip esp32p4 --port /dev/ttyACM0 --baud 921600 write-flash 0x0 esp32-chess-<versión>-completo.bin
```

Si da «Permission denied», añade tu usuario al grupo que controla los puertos y vuelve a iniciar sesión: `sudo usermod -aG dialout $USER` (en Arch, el grupo es `uucp`).

Para **solo actualizar** conservando las partidas, cambia `0x0` por `0x10000` y usa el fichero `…-app.bin`.

!!! tip "Si no conecta"
    - Cierra cualquier programa que tenga abierto el puerto (monitor serie de PlatformIO o del IDE de Arduino): el puerto no se puede compartir, y esptool dirá que está ocupado.
    - Prueba otro cable: algunos USB-C son solo de carga.
    - Si sigue sin conectar, entra a mano en modo de grabación: mantén pulsado el botón **BOOT** de la placa (si la tiene), pulsa y suelta **RESET**, y suelta BOOT.
    - Al abrir el puerto la placa se reinicia: es normal. El nombre del puerto puede cambiar entre conexiones; vuelve a mirarlo si falla.
