#!/usr/bin/env python3
"""Rasteriza assets/pieces/cburnett/*.svg a sprites ARGB8888 de LVGL (src/ui/pieces_data.c).

Requiere rsvg-convert y Pillow.  uso: python3 tools/gen_pieces.py [tamaño=64]
Orden de los sprites: P N B R Q K blancas (0..5) y P N B R Q K negras (6..11).
Memoria: B,G,R,A por pixel (ARGB8888 little endian de LVGL y del PPA), sin premultiplicar.
"""
import io, os, subprocess, sys
from PIL import Image

SIZE = int(sys.argv[1]) if len(sys.argv) > 1 else 64
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "assets/pieces/cburnett")
OUT = os.path.join(ROOT, "src/ui/pieces_data.c")
NAMES = [c + p for c in "wb" for p in "PNBRQK"]

lines = [
    "// GENERADO por tools/gen_pieces.py — no editar. Piezas cburnett (GPLv2+), %dx%d ARGB8888." % (SIZE, SIZE),
    '#include "pieces.h"',
    "",
]
for i, n in enumerate(NAMES):
    png = subprocess.run(["rsvg-convert", "-w", str(SIZE), "-h", str(SIZE), os.path.join(SRC, n + ".svg")],
                         check=True, capture_output=True).stdout
    im = Image.open(io.BytesIO(png)).convert("RGBA")
    data = bytearray()
    for r, g, b, a in im.getdata():
        data += bytes((b, g, r, a))
    lines.append("static const uint8_t LV_ATTRIBUTE_MEM_ALIGN px_%s[] = {" % n)
    for k in range(0, len(data), 24):
        lines.append("  " + ",".join(str(x) for x in data[k:k + 24]) + ",")
    lines.append("};")
lines += ["", "const lv_image_dsc_t piece_sprites[12] = {"]
for n in NAMES:
    lines.append("  { .header = { .magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_ARGB8888, .flags = 0, "
                 ".w = %d, .h = %d, .stride = %d }, .data_size = sizeof(px_%s), .data = px_%s }," % (SIZE, SIZE, SIZE * 4, n, n))
lines.append("};")
open(OUT, "w").write("\n".join(lines) + "\n")
print("escrito", OUT, os.path.getsize(OUT) // 1024, "KB")
