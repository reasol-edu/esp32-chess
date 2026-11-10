# Registro de cambios

Todos los cambios relevantes del proyecto. El formato sigue [Keep a Changelog](https://keepachangelog.com/es-ES/1.1.0/)
y las versiones, [SemVer](https://semver.org/lang/es/).

## [0.1.0] - 2026-11-10

Primera versión probada en la placa Guition JC1060P470 (ESP32-P4 rev v3.2, panel V2).

### Añadido

- Ajedrez táctil con LVGL 9.6 y la unidad PPA del ESP32-P4: tablero de madera, piezas animadas, mascotas que reaccionan y confeti al ganar.
- Motor de ajedrez GPL v3 ([hpsaturn/esp32-chess-engine](https://github.com/hpsaturn/esp32-chess-engine)) como librería, ejecutado en el núcleo 1.
- Seis niveles de dificultad con animales (Pollito, Tortuga, Conejo, Zorro, Búho y Dragón), con despistes aleatorios en los bajos.
- Ayudas: puntos de movimientos legales, *Pista*, *Deshacer* ilimitado, aviso de pieza en peligro y modo adaptable (±2 niveles).
- Guardado de partidas en 3 huecos de la memoria NVS (con CRC) y recuerdo de las últimas preferencias.
- Modo de dos jugadores: *Cara a cara* (paneles, carteles, coronación y piezas negras girados 180°) y *Pasar el aparato*.
- Reloj de ajedrez opcional (3, 5, 10 o 15 minutos) para dos jugadores.
- Nombres opcionales (hasta 10 caracteres) con teclado en pantalla con ñ y vocales con tilde.
- Simulador de la interfaz en el PC (`tools/sim/shot.sh`) para capturas sin hardware.
- 54 tests nativos del motor y de la lógica de partida (`pio test -e native`).
- Documentación para niños, familias y docentes (`docs/manual/`), publicada como web en GitHub Pages y como PDF.
- `tools/flash.sh` para grabar la placa con esptool 5.3.

### Cambiado

- Los niveles Pollito, Tortuga y Conejo esperan un mínimo de 0,8 s antes de mover, para que parezca que piensan (en la placa contestan en milisegundos).
- El Dragón piensa como máximo 5 s (antes 8 s), medido en la placa.

### Corregido

- La placa con chip ESP32-P4 rev v3 no arrancaba con `esp32-p4-evboard`: ahora se usa `esp32-p4_r3-evboard`.
- Panel «V2» (lote 2026) con otros tiempos, secuencia de inicio y reset (`src/jd9165_v2.h`); el panel antiguo se elige con `-DPANEL_JC1060P470_V1`.
- Un comentario dentro de `build_flags` en `platformio.ini` cortaba la lista y los flags del PPA no se aplicaban.
- Tocar una casilla de destino no movía la pieza: los puntos y aros absorbían el toque. Ahora no son pulsables. La lectura del táctil va en su propia tarea.
- `pio run -t upload` falla con el esptool 5.2 que trae PlatformIO en chips v3.2; se documenta el uso de esptool 5.3.

[0.1.0]: https://github.com/reasol-edu/esp32-chess/releases/tag/v0.1.0
