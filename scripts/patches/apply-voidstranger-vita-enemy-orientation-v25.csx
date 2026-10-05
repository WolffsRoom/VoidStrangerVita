using System;
using UndertaleModLib;
using UndertaleModLib.Models;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Compiler;

EnsureDataLoaded();
string Norm(UndertaleCode c) => GetDecompiledText(c).Replace("\r\n", "\n");
var imports = new CodeImportGroup(Data) { AutoCreateAssets = true };

// Retire v21. It ran from obj_player Room Start; depending on room instance order
// enemy Room Start could execute afterwards and overwrite it. It also used 2 for
// snake-left even though obj_enemy_cl only accepts 0=right / 1=left.
var player = Data.Code.ByName("gml_Object_obj_player_Other_4");
if (player == null) throw new Exception("Missing obj_player Room Start");
var pt = Norm(player);
var v21Marker = pt.IndexOf("show_debug_message(\"VITA_ENEMY_ORIENTATION_V21 applied\");", StringComparison.Ordinal);
if (v21Marker >= 0) {
    var v21Start = pt.LastIndexOf("with (obj_enemy_cl)\n{", v21Marker, StringComparison.Ordinal);
    var v21End = pt.IndexOf("ds_grid_set(ds_exit_lock, 1, 0, instance_number(obj_floorswitch));", v21Marker, StringComparison.Ordinal);
    if (v21Start < 0 || v21End < 0 || v21End <= v21Start) throw new Exception("Could not isolate v21 player override");
    pt = pt.Substring(0, v21Start) + pt.Substring(v21End);
    imports.QueueReplace(player, pt);
}

var cl = Data.Code.ByName("gml_Object_obj_enemy_cl_Other_4");
var cc = Data.Code.ByName("gml_Object_obj_enemy_cc_Other_4");
if (cl == null || cc == null) throw new Exception("Missing snake/maggot Room Start events");
var clt = Norm(cl);
var cct = Norm(cc);

if (!clt.Contains("VITA_ENEMY_ORIENTATION_V25")) {
clt += @"
// VITA_ENEMY_ORIENTATION_V25
// Apply after the enemy's own collision-derived Room Start state.
var vita_force_left =
    (room == rm_0010 && x == 184 && y == 56) ||
    (room == rm_0019 && x == 136 && y == 72) ||
    (room == rm_0027 && x == 200 && (y == 40 || y == 88)) ||
    (room == rm_0038 && ((x == 72 && y == 40) || (x == 184 && y == 88))) ||
    (room == rm_0095 && x == 56 && y == 40) ||
    (room == rm_0105 && ((x == 136 && y == 40) || (x == 72 && y == 56))) ||
    (room == rm_0111 && x == 152 && (y == 40 || y == 88)) ||
    (room == rm_0129 && x == 152 && y == 72) ||
    (room == rm_0130 && x == 200 && y == 72) ||
    (room == rm_0154 && x == 200 && (y == 40 || y == 88)) ||
    (room == rm_0164 && x == 200 && (y == 40 || y == 104)) ||
    (room == rm_0199 && x == 152 && y == 88) ||
    (room == rm_test2_003 && x == 184 && y == 40) ||
    (room == rm_test2_019 && x == 136 && y == 56) ||
    (room == rm_test2_026 && x == 184 && (y == 72 || y == 88)) ||
    (room == rm_test2_030 && x == 136 && (y == 24 || y == 40 || y == 88)) ||
    (room == rm_e_008 && x == 200 && y == 24);
var vita_force_right = room == rm_0010 && x == 88 && y == 104;
if (vita_force_left)
{
    set_e_direction = 1;
    e_direction = 180;
    e_move_x = lengthdir_x(e_spd, e_direction);
    e_move_y = lengthdir_y(e_spd, e_direction);
    sprite_index = spr_l;
}
else if (vita_force_right)
{
    set_e_direction = 0;
    e_direction = 0;
    e_move_x = lengthdir_x(e_spd, e_direction);
    e_move_y = lengthdir_y(e_spd, e_direction);
    sprite_index = spr_r;
}
var vita_orientation_contract = ""VITA_ENEMY_ORIENTATION_V25"";
";
imports.QueueReplace(cl, clt);
}

if (!cct.Contains("VITA_ENEMY_ORIENTATION_V25")) {
cct += @"
// VITA_ENEMY_ORIENTATION_V25
var vita_force_up =
    (room == rm_0005 && x == 168 && y == 104) ||
    (room == rm_0017 && x == 136 && y == 104) ||
    (room == rm_0020 && x == 184 && y == 104) ||
    (room == rm_0024 && y == 88 && (x == 40 || x == 72)) ||
    (room == rm_0095 && x == 200 && y == 104) ||
    (room == rm_0131 && x == 152 && y == 104) ||
    (room == rm_0143 && x == 184 && y == 104) ||
    (room == rm_0158 && x == 88 && y == 104) ||
    (room == rm_test2_036 && x == 120 && y == 104) ||
    (room == rm_test2_003 && x == 136 && y == 104) ||
    (room == rm_test2_031 && y == 104 && (x == 56 || x == 120)) ||
    (room == rm_test2_022 && y == 104 && (x == 72 || x == 152)) ||
    (room == rm_test2_026 && y == 56 && (x == 72 || x == 136)) ||
    (room == rm_test2_030 && x == 104 && y == 104) ||
    (room == rm_test2_021 && x == 88 && y == 104);
if (vita_force_up)
{
    set_e_direction = 1;
    e_direction = 90;
    e_move_x = lengthdir_x(e_spd, e_direction);
    e_move_y = lengthdir_y(e_spd, e_direction);
    sprite_index = spr_u;
}
var vita_orientation_contract = ""VITA_ENEMY_ORIENTATION_V25"";
";
imports.QueueReplace(cc, cct);
}

imports.Import();
Console.WriteLine("Void Stranger enemy orientation v25 applied.");
