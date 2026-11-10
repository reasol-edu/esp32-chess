# Créditos y licencias de terceros

Este proyecto se publica bajo **GPL v3** y se apoya en el trabajo de otras personas:

## Motor de ajedrez

- **Autor original:** Sergey Urusov, *ESP32 chess engine 1.0* (GPL v3).
- **Versión de partida:** [hpsaturn/esp32-chess-engine](https://github.com/hpsaturn/esp32-chess-engine) (GPL v3), de hpsaturn.
- **Adaptación en este repositorio** (`src/engine/`): convertido en librería dentro de `namespace ce`, sin `Serial`, `setup`, `loop`
  ni pruebas ELO/WAC, con la tarea auxiliar sustituida por una llamada síncrona, un tope de profundidad y una API pública
  (`chess_api.h`). Texto de la licencia: [`LICENSE-engine-GPLv3`](LICENSE-engine-GPLv3).

## Piezas

- Set **cburnett** de Colin M. L. Burnett (<https://en.wikipedia.org/wiki/User:Cburnett>), tomado de
  [lichess-org/lila](https://github.com/lichess-org/lila) (`public/piece/cburnett`). Licencia **GPLv2+**
  (compatible con GPL v3). Ver `assets/pieces/`.

## Bibliotecas y herramientas

- [LVGL](https://lvgl.io) 9.6 (MIT): interfaz gráfica.
- [GFX Library for Arduino](https://github.com/moononournation/Arduino_GFX) de moononournation (licencia en su repositorio): pantalla MIPI-DSI.
- [TAMC_GT911](https://github.com/tamctec/gt911-arduino) (Apache-2.0): controlador táctil.
- [Arduino-ESP32 / ESP-IDF](https://github.com/espressif/arduino-esp32) de Espressif (Apache-2.0): plataforma, vía
  [pioarduino](https://github.com/pioarduino/platform-espressif32).
- Los parámetros de pantalla y la secuencia de inicio del panel «V2» proceden de la definición `JC1060P470-V2` de
  [ESPHome](https://github.com/esphome/esphome) (`mipi_dsi/models/guition.py`), que a su vez toma la secuencia del
  controlador de referencia del fabricante.
- La inicialización de pantalla, táctil y PPA procede del proyecto del autor
  [deye-monitor](https://github.com/lrlopez/deye-monitor).
- La documentación se genera con [MkDocs Material](https://squidfunk.github.io/mkdocs-material/), [pandoc](https://pandoc.org)
  y [Paged.js](https://pagedjs.org).
