// Platter stack diagrams (exploded view + cross-section) for the docs.
// Run ./render_diagrams.sh: it renders each halved part to tmp/*.stl, then the PNGs.
//   VIEW = "exploded" | "section"   PART = "<name>" renders one halved part
include <../scratchtj_stepped.scad>
SHOW = "none";
VIEW = "";
PART = "";
$fn = 96;

NAMES  = ["deck", "bearing", "retainer", "screws", "spacer", "hub", "platter",
          "cap", "bolt", "collar", "nut", "magnet", "pcb", "wire"];
COLORS = ["#555555", "Gold", "Ivory", "Silver", "Orange", "CornflowerBlue", "#d0d0d0",
          "Ivory", "Silver", "Orange", "DimGray", "Red", "Green", "Chocolate"];

// assembled z of each part (deck coordinates)
function zc(n) =
    n == "bearing"   ? brg_bot :
    n == "retainer"  ? brg_top :
    n == "screws"    ? brg_top + ret_th :
    n == "spacer"    ? brg_top + spacer_len :
    n == "hub"       ? hub_bot :
    n == "platter"   ? plat_bot_d :
    n == "cap"       ? plat_top_d :
    n == "bolt"      ? head_bot :
    n == "collar"    ? brg_bot - collar_h :
    n == "nut"       ? nut_bot :
    n == "magnet"    ? mag_face :
    n == "pcb"       ? enc_pcb_top_d - enc_pcb_t : 0;

// exploded z
function ze(n) =
    n == "bearing"   ? 6 :
    n == "retainer"  ? 22 :
    n == "screws"    ? 31 :
    n == "spacer"    ? 39 + spacer_len :
    n == "hub"       ? 50 :
    n == "platter"   ? 64 :
    n == "cap"       ? 74 :
    n == "bolt"      ? 74 + cap_floor + 12 :
    n == "collar"    ? -30 :
    n == "nut"       ? -42 :
    n == "magnet"    ? -54 :
    n == "pcb"       ? -70 : 0;

// scene turned so the touch wire (270 deg) lies in the cut plane, on +X
module turned() rotate([0, 0, 90]) children();

module part(n) {
    if (n == "deck") turned() translate([-plat_cx, -plat_cy, 0]) intersection() {
        high_deck();
        translate([plat_cx - 30, plat_cy - 30, -30]) cube([60, 60, 40]);
    }
    if (n == "bearing") difference() {
        cylinder(d = brg_od, h = brg_h);
        translate([0, 0, -1]) cylinder(d = bolt_d, h = brg_h + 2);
        // shields, for looks
        for (z = [-1, brg_h - 0.3]) translate([0, 0, z]) difference() {
            cylinder(d = brg_od - 2.4, h = 1.3); cylinder(d = brg_inner_d, h = 1.3);
        }
    }
    if (n == "retainer") turned() bearing_retainer();
    if (n == "screws") turned() for (a = ret_screw_ang) rotate([0, 0, a]) translate([ret_screw_r, 0, 0]) {
        cylinder(d = 5.5, h = 2.4); translate([0, 0, -6]) cylinder(d = 2.9, h = 6 + e); }
    if (n == "spacer") mirror([0, 0, 1]) hub_spacer();
    if (n == "hub") platter_hub();
    if (n == "platter") difference() {
        cylinder(d = plat_d, h = plat_th); translate([0, 0, -1]) cylinder(d = plat_hole_d, h = plat_th + 2); }
    if (n == "cap") turned() platter_clamp();
    if (n == "bolt") {
        translate([0, 0, -bolt_shank]) cylinder(d = bolt_d, h = bolt_shank + e);
        hull() {                                                    // pan head
            cylinder(d = bolt_head_d, h = bolt_head_h * 0.4);
            cylinder(d = bolt_head_d * 0.75, h = bolt_head_h);
        }
    }
    if (n == "collar") lower_collar();
    if (n == "nut") difference() { cylinder(d = hex_d, h = nut_h, $fn = 6); translate([0, 0, -1]) cylinder(d = bolt_d, h = nut_h + 2); }
    if (n == "magnet") cylinder(d = mag_d, h = mag_h);
    if (n == "pcb") {
        translate([-enc_pcb / 2, -enc_pcb / 2, 0]) cube([enc_pcb, enc_pcb, enc_pcb_t]);
        translate([-2.5, -2, enc_pcb_t]) cube([5, 4, enc_chip_h]);
    }
    if (n == "wire") turned() rotate([0, 0, cap_wire_ang]) {
        translate([brg_seat_d / 2 - 0.2, -0.4, boss_bot + 1.5]) cube([0.8, 0.8, -boss_bot - 1.6]);
        translate([brg_seat_d / 2 - 0.2, -0.4, boss_bot + 1.5]) cube([enc_boss_d / 2 + 8, 0.8, 0.8]);
    }
}

// front half (y < 0) removed
module halved() intersection() { children(); translate([-100, 0, -100]) cube([200, 200, 200]); }

if (PART != "") halved() part(PART);

module shown(n, z, cut) {
    i = search([n], NAMES)[0];
    color(COLORS[i], n == "platter" ? 0.9 : 1)
        translate([0, 0, z]) if (cut) import(str("tmp/", n, ".stl")); else part(n);
}

// leader from (x0, z) on the cut plane, dog-leg to text at height tz
module label(z, x0, txt, tx, ts, tz = undef) {
    t = is_undef(tz) ? z : tz;
    k = tx - 1.5 - 6;
    color("#222") {
        translate([x0, -0.3, z]) rotate([90, 0, 0]) cylinder(d = ts * 0.35, h = 0.3);
        hull() { translate([x0, -0.3, z - ts * 0.04]) cube([0.01, 0.3, ts * 0.08]);
                 translate([k,  -0.3, z - ts * 0.04]) cube([0.01, 0.3, ts * 0.08]); }
        hull() { translate([k,  -0.3, z - ts * 0.04]) cube([0.01, 0.3, ts * 0.08]);
                 translate([tx - 1.5, -0.3, t - ts * 0.04]) cube([0.01, 0.3, ts * 0.08]); }
        translate([tx, 0, t - ts * 0.45]) rotate([90, 0, 0]) linear_extrude(0.5)
            text(txt, size = ts, font = "Liberation Sans:style=Bold");
    }
}

if (VIEW == "exploded") {
    color("#aaa") translate([-0.15, -0.15, ze("pcb")]) cube([0.3, 0.3, ze("bolt") - ze("pcb") + 6]);
    shown("deck", 0, true);
    shown("wire", 0, false);
    for (n = ["bearing", "retainer", "screws", "spacer", "hub", "platter", "cap",
              "bolt", "collar", "nut", "magnet", "pcb"]) shown(n, ze(n), false);
    T = 62; S = 3.4;
    label(ze("bolt") - 8,        2.5, "8  M6 x 25.5 pan-head bolt, from the top", T, S);
    label(ze("cap") + 3,         15, "7  Top cap (clamps the platter)", T, S);
    label(ze("platter") + 0.5,   47.5, "6  Platter (25 mm hole)", T, S);
    label(ze("hub") + 1.5,       30, "5  Platter hub: spigot up", T, S);
    label(ze("spacer") - 3,      6,  "4  Spacer: narrow tip down", T, S);
    label(ze("screws") + 1,      15, "3b 3x M3 x 6", T, S);
    label(ze("retainer") + 1,    17, "3  Retainer (screws the bearing in)", T, S);
    label(ze("bearing") + 3,     9, "2  626ZZ bearing, press fit, 0.2 proud", T, S);
    label(-1.5,                  30, "1  High deck: wire in the groove", T, S);
    label(ze("collar") + 0.7,    5,  "9  Collar, Ø8 boss up", T, S);
    label(ze("nut") + 2,         4.6,  "10 M6 nut: tighten", T, S);
    label(ze("magnet") + 1,      2,  "11 Ø4 x 2 magnet on the tip", T, S);
    label(ze("pcb") + 1,         11.5, "MT6701 (in the tray)", T, S);
}

if (VIEW == "section") {
    intersection() {
        union() { for (n = NAMES) if (n != "wire") shown(n, zc(n), true); shown("wire", 0, false); }
        translate([-24, -1, -40]) cube([24 + 46, 120, 80]);
    }
    T = 54; S = 1.6; D = 2.6; Z0 = 17;
    label(head_bot + 2,           6,  "M6 x 25.5 pan-head bolt", T, S, Z0);
    label(plat_top_d + 1.5,       15, "Top cap",                  T, S, Z0 - D);
    label(plat_bot_d + 0.5,       40, "Platter 1 mm",             T, S, Z0 - 2*D);
    label(hub_bot + 1.5,          26, "Platter hub",              T, S, Z0 - 3*D);
    label(brg_top + ret_th / 2,   14, "Retainer + 3x M3 x 6",     T, S, Z0 - 4*D);
    label(-1.8,                   28, "High deck",                T, S, Z0 - 5*D);
    label(brg_bot + 2.5,          7,  "626ZZ bearing",            T, S, Z0 - 6*D);
    label(boss_bot + 2.2,         22, "Touch wire",               T, S, Z0 - 7*D);
    label(nut_bot + 2,            4.6, "Collar + M6 nut",         T, S, Z0 - 8*D);
    label(mag_face + 1,           1.5,"Magnet",                   T, S, Z0 - 9*D);
    label(enc_pcb_top_d - 0.8,    10, str("MT6701 (", mag_gap, " mm under the magnet)"), T, S, Z0 - 10*D);
}
