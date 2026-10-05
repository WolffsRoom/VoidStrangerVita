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
if (before.Contains("VITA_ENEMY_ORIENTATION_V19")) throw new Exception("Enemy orientation patch already applied");

// These are the internal room coordinates corresponding to the PC reference
// captures in logs/b012, b017, b019, b020, b024, b027, b038, b092 and b108.
// The Vita spawn direction differs in these instances; do not refresh every
// enemy, because other instances in the same rooms already face correctly.
var fix = @"
// VITA_ENEMY_ORIENTATION_V19: match the original PC spawn directions.
with (obj_enemy_cl)
{
    if ((room == rm_0010 && x == 88 && y == 104) ||
        (room == rm_0019 && x == 136 && y == 72) ||
        (room == rm_0027 && x == 200 && y == 88) ||
        (room == rm_0038 && ((x == 72 && y == 40) || (x == 184 && y == 88))) ||
        (room == rm_0095 && x == 56 && y == 40) ||
        (room == rm_0111 && x == 152 && (y == 40 || y == 88)))
    {
        set_e_direction = 1;
        e_direction = 180;
        sprite_index = spr_l;
    }
}
with (obj_enemy_cc)
{
    if ((room == rm_0017 && x == 136 && y == 104) ||
        (room == rm_0020 && x == 184 && y == 104) ||
        (room == rm_0024 && y == 88 && (x == 40 || x == 72)) ||
        (room == rm_0095 && x == 200 && y == 104))
    {
        set_e_direction = 1;
        e_direction = 90;
        sprite_index = spr_u;
    }
}
";
CodeImportGroup imports = new(Data) { AutoCreateAssets = true };
imports.QueueReplace(code, before.Replace(anchor, fix + anchor));
imports.Import();
Console.WriteLine("Vita enemy orientation v19 applied to the reported PC/Vita mismatches.");
