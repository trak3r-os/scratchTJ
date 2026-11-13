#!/usr/bin/env bash
# Render the platter stack diagrams into ../images/.
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p tmp
for p in deck bearing retainer screws spacer hub platter topwasher bolt washer nut magnet pcb; do
  openscad -o "tmp/$p.stl" -D "PART=\"$p\"" platter_stack.scad &
done
wait
for v in exploded section; do
  openscad -o "../images/platter_$v.png" --imgsize=1800,1800 --camera=0,0,0,80,0,0,300 \
    --viewall --autocenter --colorscheme=Tomorrow -D "VIEW=\"$v\"" platter_stack.scad
done
rm -rf tmp
