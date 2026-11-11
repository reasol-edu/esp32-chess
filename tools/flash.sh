#!/bin/bash
# Sube el firmware a una placa con ESP32-P4 rev v3.x. pio upload falla con su esptool 5.2 (el stub se cuelga):
# hace falta esptool >= 5.3.dev3 (pip install esptool==5.3.dev3 en un venv) y se indica con ESPTOOL=...
# Placa original (chip ES pre rev.300 y panel antiguo): PIO_ENV=jc1060p470_v1 (compilar antes ese entorno).
# Uso: ESPTOOL=/ruta/venv/bin/esptool tools/flash.sh [puerto]   (tras: pio run -e jc1060p470)
set -e
cd "$(dirname "$0")/.."
ESPTOOL=${ESPTOOL:-esptool}
PORT=${1:-$(ls /dev/cu.usbmodem* 2>/dev/null | head -1)}
B=.pio/build/${PIO_ENV:-jc1060p470}   # PIO_ENV=jc1060p470_v1 para la placa original
BOOT_APP0=$(ls ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin)
"$ESPTOOL" --chip esp32p4 --port "$PORT" --baud 921600 write-flash -z --flash-mode dio --flash-freq 80m --flash-size detect \
  0x2000 $B/bootloader.bin 0x8000 $B/partitions.bin 0xe000 "$BOOT_APP0" 0x10000 $B/firmware.bin
