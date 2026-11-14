#!/usr/bin/env bash
# Render the platter stack diagrams into ../images/.
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p tmp
for p in deck bearing retainer screws spacer hub platter cap bolt collar nut magnet pcb; do
  openscad -o "tmp/$p.stl" -D "PART=\"$p\"" platter_stack.scad &
done
wait
for v in exploded section; do
  openscad -o "../images/platter_$v.png" --imgsize=1800,1800 --camera=0,0,0,80,0,0,300 \
    --viewall --autocenter --colorscheme=Tomorrow -D "VIEW=\"$v\"" platter_stack.scad
done
rm -rf tmp
# trim the empty margin around each image
python3 - <<'PY'
from PIL import Image, ImageChops
for v in ("exploded", "section"):
    f = f"../images/platter_{v}.png"
    im = Image.open(f).convert("RGB")
    bb = ImageChops.difference(im, Image.new("RGB", im.size, im.getpixel((2, 2)))).getbbox()
    m = 40
    im.crop((max(bb[0]-m, 0), max(bb[1]-m, 0), min(bb[2]+m, im.width), min(bb[3]+m, im.height))).save(f)
PY
