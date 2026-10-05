using System;
using UndertaleModLib;
using UndertaleModLib.Models;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Compiler;

EnsureDataLoaded();
CodeImportGroup imports = new(Data) { AutoCreateAssets = true };
string Norm(UndertaleCode c) => GetDecompiledText(c).Replace("\r\n", "\n");

var saveCode = Data.Code.ByName("gml_GlobalScript_scr_savesettings");
var loadCode = Data.Code.ByName("gml_GlobalScript_scr_loadsettings");
if (saveCode == null || loadCode == null) throw new Exception("Missing settings scripts");

string save = Norm(saveCode);
string saveMarker = "Vita Graphics Persist v7";
if (!save.Contains(saveMarker)) {
    string oldSave = @"            ini_write_string(""Save1"", ""System Settings"", ds_grid_write(_System_gstring));
            ini_write_string(""Save1"", ""Version"", ""4"");
            ini_close();".Replace("\r\n", "\n");
    string newSave = @"            ini_write_string(""Save1"", ""System Settings"", ds_grid_write(_System_gstring));
            ini_write_string(""Save1"", ""Version"", ""4"");
            // Vita Graphics Persist v7: named values survive menu row/layout changes.
            ini_write_real(""Vita Graphics"", ""Scaling"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 0));
            ini_write_real(""Vita Graphics"", ""Border"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 1));
            ini_write_real(""Vita Graphics"", ""VSync"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 2));
            ini_write_real(""Vita Graphics"", ""Flicker"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 3));
            ini_write_real(""Vita Graphics"", ""Timer"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 4));
            ini_write_real(""Vita Graphics"", ""Counter"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 5));
            ini_write_real(""Vita Graphics"", ""Palette"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 6));
            ini_write_real(""Vita Graphics"", ""Brightness"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 7));
            ini_write_real(""Vita Graphics"", ""Stretch"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 8));
            ini_close();".Replace("\r\n", "\n");
    if (!save.Contains(oldSave)) throw new Exception("scr_savesettings insertion point not found");
    save = save.Replace(oldSave, newSave);
    imports.QueueReplace(saveCode, save);
}

string load = Norm(loadCode);
if (!load.Contains(saveMarker)) {
    string oldTail = @"    if (is_undefined(global.language) || global.language < 0 || global.language >= array_length(global.script_array))
    {
        global.language = 0;
    }
}".Replace("\r\n", "\n");
    string newTail = @"    if (is_undefined(global.language) || global.language < 0 || global.language >= array_length(global.script_array))
    {
        global.language = 0;
    }
    // Vita Graphics Persist v7: named settings are the final authority after
    // the legacy settings.vs + settings.vslocal compatibility loaders.
    if (instance_exists(obj_menu) && file_exists(""settings.vs""))
    {
        ini_open(""settings.vs"");
        if (ini_section_exists(""Vita Graphics""))
        {
            var _vita_scaling = clamp(round(ini_read_real(""Vita Graphics"", ""Scaling"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 0))), 0, 1);
            var _vita_border = clamp(round(ini_read_real(""Vita Graphics"", ""Border"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 1))), 0, 1);
            var _vita_vsync = clamp(round(ini_read_real(""Vita Graphics"", ""VSync"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 2))), 0, 1);
            var _vita_flicker = clamp(round(ini_read_real(""Vita Graphics"", ""Flicker"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 3))), 0, 2);
            var _vita_timer = clamp(round(ini_read_real(""Vita Graphics"", ""Timer"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 4))), 0, 1);
            var _vita_counter = clamp(round(ini_read_real(""Vita Graphics"", ""Counter"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 5))), 0, 1);
            var _vita_palette = clamp(round(ini_read_real(""Vita Graphics"", ""Palette"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 6))), 0, 8);
            var _vita_brightness = clamp(round(ini_read_real(""Vita Graphics"", ""Brightness"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 7))), 0, 9);
            var _vita_stretch = clamp(round(ini_read_real(""Vita Graphics"", ""Stretch"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 8))), 0, 1);
            ds_grid_set(obj_menu.ds_menu_graphics, 3, 0, _vita_scaling);
            ds_grid_set(obj_menu.ds_menu_graphics, 3, 1, _vita_border);
            ds_grid_set(obj_menu.ds_menu_graphics, 3, 2, _vita_vsync);
            ds_grid_set(obj_menu.ds_menu_graphics, 3, 3, _vita_flicker);
            ds_grid_set(obj_menu.ds_menu_graphics, 3, 4, _vita_timer);
            ds_grid_set(obj_menu.ds_menu_graphics, 3, 5, _vita_counter);
            ds_grid_set(obj_menu.ds_menu_graphics, 3, 6, _vita_palette);
            ds_grid_set(obj_menu.ds_menu_graphics, 3, 7, _vita_brightness);
            ds_grid_set(obj_menu.ds_menu_graphics, 3, 8, _vita_stretch);
            change_fullscreen_scaling(_vita_scaling);
            change_border_fill(_vita_border);
            toggle_vsync(_vita_vsync);
            change_flicker(_vita_flicker);
            toggle_timer(_vita_timer);
            toggle_counter(_vita_counter);
            change_palette(_vita_palette);
            change_resolution(_vita_brightness);
            change_window_mode(_vita_stretch);
        }
        ini_close();
    }
}".Replace("\r\n", "\n");
    if (!load.Contains(oldTail)) throw new Exception("scr_loadsettings tail not found");
    load = load.Replace(oldTail, newTail);
    imports.QueueReplace(loadCode, load);
}

imports.Import();
ScriptMessage("Void Stranger settings v7: named Vita graphics persistence added.");
