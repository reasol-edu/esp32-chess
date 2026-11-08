#!/bin/sh
# Compila LVGL (una vez) y el simulador de la UI en el PC. uso: tools/sim/build.sh [clean]
# Requiere clang y haber compilado antes el proyecto (pio run -e jc1060p470) para tener LVGL en .pio/libdeps.
set -e
HERE=$(cd "$(dirname "$0")" && pwd); P=$(cd "$HERE/../.." && pwd)
export LVGL_DIR=${LVGL_DIR:-$P/.pio/libdeps/jc1060p470/lvgl}
[ -d "$LVGL_DIR/src" ] || { echo "No encuentro LVGL en $LVGL_DIR: ejecuta antes 'pio run -e jc1060p470'"; exit 1; }
[ "$1" = clean ] && rm -rf "$HERE/obj" "$HERE/mock" "$HERE/out"
mkdir -p "$HERE/obj" "$HERE/mock" "$HERE/out"
# lv_conf del simulador: el del proyecto, con malloc de C y sin FreeRTOS
sed -e 's/LV_STDLIB_CUSTOM/LV_STDLIB_CLIB/' -e 's/define LV_USE_OS   LV_OS_FREERTOS/define LV_USE_OS   LV_OS_NONE/' "$P/include/lv_conf.h" > "$HERE/mock/lv_conf.h"
if [ -z "$(ls "$HERE/obj"/*.o 2>/dev/null)" ]; then
  echo "Compilando LVGL (solo la primera vez)..."
  { find "$LVGL_DIR/src" -name '*.c' | grep -Ev '/(drivers|libs)/|/draw/(vg_lite|nxp|nema_gfx|renesas|eve|espressif|dma2d|opengles|sdl)/|/draw/convert/helium|/draw/sw/blend/(neon|helium|riscv_v)|/osal/(lv_cmsis|lv_freertos|lv_linux|lv_mqx|lv_pthread|lv_rtthread|lv_sdl2|lv_windows)|/others/(file_explorer|fragment|translation)|/draw/sw/lv_draw_sw_(helium|neon)'
    ls "$P"/src/fonts/*.c "$P"/src/ui/pieces_data.c; } | xargs -P 8 -n 1 "$HERE/cc1.sh"
  [ -f "$HERE/obj/build_fail.txt" ] && { echo "Fallos al compilar LVGL:"; cat "$HERE/obj/build_fail.txt"; exit 1; }
fi
echo "LVGL listo"
