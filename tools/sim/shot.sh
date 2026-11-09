#!/bin/sh
# uso: tools/sim/shot.sh <escena> [salida.png]   escenas: newgame newgame_black start zorro select mid capture check hint promo win lose black
set -e
HERE=$(cd "$(dirname "$0")" && pwd); P=$(cd "$HERE/../.." && pwd)
LVGL_DIR=${LVGL_DIR:-$P/.pio/libdeps/jc1060p470/lvgl}
"$HERE/build.sh" >/dev/null
cd "$HERE"
clang++ -std=gnu++17 -O1 -w -DLV_CONF_INCLUDE_SIMPLE -DBOARD_GUITION_JC1060P470 -DLV_LVGL_H_INCLUDE_SIMPLE -Imock -I"$LVGL_DIR" -I"$LVGL_DIR/include" -I"$LVGL_DIR/include/lvgl" \
  -I"$P/src" -I"$P/src/ui" -I"$P/src/engine" -I"$P/src/game" -I"$P/test/native_shim" \
  harness.cpp "$P/src/ui/ui_game.cpp" "$P/src/ui/ui_avatar.cpp" "$P/src/ui/pieces.cpp" "$P/src/game/game_core.cpp" "$P/src/game/danger.cpp" "$P/src/game/save_format.cpp" "$P/src/engine/engine_core.cpp" obj/*.o -o out/preview
OUT=${2:-$HERE/out/$1.png}
./out/preview "$1" out/tmp.ppm
python3 -I -c "
from PIL import Image; Image.open('out/tmp.ppm').save('$OUT'); print('$OUT')"
