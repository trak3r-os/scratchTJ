#!/usr/bin/env bash
# Export every printable part of scratchtj_stepped.scad as its own STL.
# Usage: ./export_stl.sh            (needs OpenSCAD in PATH)
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p stl
declare -A PARTS=(
  [tray]=01_tray
  [high]=02_high_deck
  [low]=03_low_deck
  [carrier]=04_display_carrier
  [hub]=05_platter_hub
  [spacer]=06_hub_spacer
  [retainer]=07_bearing_retainer
  [clamp]=08_platter_cap
  [collar]=09_bearing_collar
)
for mode in "${!PARTS[@]}"; do
  out="stl/${PARTS[$mode]}.stl"
  echo "Exporting $mode -> $out"
  openscad -o "$out" -D "SHOW=\"$mode\"" scratchtj_stepped.scad &
done
wait
echo "Done."
