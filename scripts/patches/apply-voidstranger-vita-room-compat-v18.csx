using System;
using UndertaleModLib;
using UndertaleModLib.Models;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Compiler;
EnsureDataLoaded();
CodeImportGroup imports = new(Data) { AutoCreateAssets = true };

void ReplaceCode(string name, Func<string,string> edit) {
    var c = Data.Code.ByName(name);
    if (c == null) throw new Exception("Missing code: " + name);
    var before = GetDecompiledText(c).Replace("\r\n", "\n");
    var after = edit(before);
    if (after == before) throw new Exception("No change in " + name);
    imports.QueueReplace(c, after);
}

ReplaceCode("gml_Object_obj_player_Other_4", t => {
    var anchor = "ds_grid_set(ds_exit_lock, 1, 0, instance_number(obj_floorswitch));";
    if (!t.Contains(anchor)) throw new Exception("obj_player Room Start tail anchor missing");
    if (t.Contains("VITA_COMPAT_FB006") || t.Contains("VITA_COMPAT_E022"))
        throw new Exception("room compatibility patch already present");

    var compat = @"
// Vita/Butterscotch compatibility: the player builds the generated collision
// grid later in Room Start than some static room objects expect. Refresh the
// two known dependencies after the grid is complete instead of changing the
// runner's global Room Start ordering.
if (room == rm_fb_006)
{
    with (obj_elevator_activate)
    {
        var _vita_icoll = instance_place(x, y, obj_collision);
        if (_vita_icoll != noone)
        {
            with (_vita_icoll)
            {
                instance_destroy();
            }
        }
    }
    show_debug_message(""VITA_COMPAT_FB006 collision cleared"");
}
if (room == rm_e_022)
{
    with (obj_enemy_cc)
    {
        if (!place_meeting(x + e_move_x, y + e_move_y, obj_obstacle_parent) && !place_meeting(x + e_move_x, y + e_move_y, obj_pit) && !place_meeting(x + e_move_x, y + e_move_y, obj_enemy_parent))
        {
            set_e_direction = 0;
        }
        else
        {
            set_e_direction = 1;
        }
        if (set_e_direction == 0)
        {
            e_direction = 270;
            sprite_index = spr_d;
        }
        else
        {
            e_direction = 90;
            sprite_index = spr_u;
        }
    }
    show_debug_message(""VITA_COMPAT_E022 enemy orientation refreshed"");
}
";
    return t.Replace(anchor, compat + anchor);
});

imports.Import();
Console.WriteLine("v1.22 room compatibility patch applied: rm_fb_006 collision cleanup + rm_e_022 maggot orientation refresh.");
