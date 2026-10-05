using System;
using UndertaleModLib;
using UndertaleModLib.Models;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Compiler;

EnsureDataLoaded();
var code = Data.Code.ByName("gml_Object_obj_player_Other_4");
if (code == null) throw new Exception("Missing player Room Start code");
var before = GetDecompiledText(code).Replace("\r\n", "\n");
const string anchor = "ds_grid_set(ds_exit_lock, 1, 0, instance_number(obj_floorswitch));";
if (!before.Contains(anchor)) throw new Exception("Missing player Room Start tail anchor");

var oldClCondition = "if ((room == rm_0010 && x == 88 && y == 104) || (room == rm_0019 && x == 136 && y == 72) || (room == rm_0027 && x == 200 && y == 88) || (room == rm_0038 && ((x == 72 && y == 40) || (x == 184 && y == 88))) || (room == rm_0095 && x == 56 && y == 40) || (room == rm_0111 && x == 152 && (y == 40 || y == 88)))";
var condPos = before.IndexOf(oldClCondition, StringComparison.Ordinal);
if (condPos < 0) throw new Exception("Missing v19 enemy orientation block");
var blockStart = before.LastIndexOf("with (obj_enemy_cl)\n{", condPos, StringComparison.Ordinal);
var blockEnd = before.IndexOf(anchor, condPos, StringComparison.Ordinal);
if (blockStart < 0 || blockEnd < 0 || blockEnd <= blockStart) throw new Exception("Could not isolate v19 enemy orientation block");

var fix = @"
// VITA_ENEMY_ORIENTATION_V21: keep visual direction and movement direction consistent.
with (obj_enemy_cl)
{
    var vita_force_left =
        (room == rm_0010 && x == 184 && y == 56) ||
        (room == rm_0019 && x == 136 && y == 72) ||
        (room == rm_0027 && x == 200 && y == 88) ||
        (room == rm_0038 && ((x == 72 && y == 40) || (x == 184 && y == 88))) ||
        (room == rm_0095 && x == 56 && y == 40) ||
        (room == rm_0111 && x == 152 && (y == 40 || y == 88)) ||
        (room == rm_0129 && x == 152 && y == 72) ||
        (room == rm_0130 && x == 200 && y == 72) ||
        (room == rm_0154 && x == 200 && (y == 40 || y == 88)) ||
        (room == rm_0164 && x == 200 && (y == 40 || y == 104)) ||
        (room == rm_0199 && x == 152 && y == 88);
    var vita_force_right = room == rm_0010 && x == 88 && y == 104;
    if (vita_force_left)
    {
        set_e_direction = 2;
        e_direction = 180;
        sprite_index = spr_l;
    }
    else if (vita_force_right)
    {
        set_e_direction = 0;
        e_direction = 0;
        sprite_index = spr_r;
    }
}
with (obj_enemy_cc)
{
    var vita_force_up =
        (room == rm_0017 && x == 136 && y == 104) ||
        (room == rm_0020 && x == 184 && y == 104) ||
        (room == rm_0024 && y == 88 && (x == 40 || x == 72)) ||
        (room == rm_0095 && x == 200 && y == 104) ||
        (room == rm_0131 && x == 152 && y == 104) ||
        (room == rm_0143 && x == 184 && y == 104);
    if (vita_force_up)
    {
        set_e_direction = 1;
        e_direction = 90;
        sprite_index = spr_u;
    }
}
show_debug_message(""VITA_ENEMY_ORIENTATION_V21 applied"");
";

var after = before.Substring(0, blockStart) + fix + before.Substring(blockEnd);
CodeImportGroup imports = new(Data) { AutoCreateAssets = true };
imports.QueueReplace(code, after);
imports.Import();
Console.WriteLine("Vita enemy orientation v21 applied.");
