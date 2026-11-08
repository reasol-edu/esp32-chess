#!/bin/sh
# Compila un .c a obj/ (lo usa build.sh). Variables de entorno: LVGL_DIR.
S=$(cd "$(dirname "$0")" && pwd); P=$(cd "$S/../.." && pwd)
o=$S/obj/$(echo "$1" | shasum | cut -c1-12).o
clang -O1 -w -c -DLV_CONF_INCLUDE_SIMPLE -DBOARD_GUITION_JC1060P470 -DLV_LVGL_H_INCLUDE_SIMPLE -I"$S/mock" -I"$LVGL_DIR" -I"$LVGL_DIR/include" -I"$LVGL_DIR/include/lvgl" -I"$P/src/ui" "$1" -o "$o" 2>>"$S/obj/build_lv.log" || echo "FAIL $1" >> "$S/obj/build_fail.txt"
