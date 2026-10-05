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
if (!save.Contains("Vita Graphics Persist v8")) {
    string oldSave = @"            ini_write_string(""Save1"", ""Version"", ""4"");
            ini_close();
            ds_grid_destroy(_Audio_gstring);".Replace("\r\n", "\n");
    string newSave = @"            ini_write_string(""Save1"", ""Version"", ""4"");
            ini_close();
            // Vita Graphics Persist v8: dedicated file is independent from the
            // game's legacy settings.vs/settings.vslocal row serialization.
            ini_open(""vita_graphics.ini"");
            ini_write_real(""Graphics"", ""Scaling"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 0));
            ini_write_real(""Graphics"", ""Border"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 1));
            ini_write_real(""Graphics"", ""VSync"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 2));
            ini_write_real(""Graphics"", ""Flicker"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 3));
            ini_write_real(""Graphics"", ""Timer"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 4));
            ini_write_real(""Graphics"", ""Counter"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 5));
            ini_write_real(""Graphics"", ""Palette"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 6));
            ini_write_real(""Graphics"", ""Brightness"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 7));
            ini_write_real(""Graphics"", ""Stretch"", ds_grid_get(obj_menu.ds_menu_graphics, 3, 8));
            ini_close();
            ds_grid_destroy(_Audio_gstring);".Replace("\r\n", "\n");
    if (!save.Contains(oldSave)) throw new Exception("v8 save insertion point not found");
    save = save.Replace(oldSave, newSave);
    imports.QueueReplace(saveCode, save);
}

string load = Norm(loadCode);
if (!load.Contains("Vita Graphics Persist v8")) {
    int closing = load.LastIndexOf('}');
    if (closing < 0) throw new Exception("scr_loadsettings closing brace not found");
    string block = @"
    // Vita Graphics Persist v8: final authoritative load. On the first v8
    // boot, migrate the existing options but deliberately reset Brightness to
    // its intended default (index 9 = 100%) because older builds could migrate
    // the legacy Resolution row as index 0 (10%). Once this file exists every
    // user-selected value, including 10%, is restored exactly on next boot.
    var _vita8_scaling = clamp(round(ds_grid_get(obj_menu.ds_menu_graphics, 3, 0)), 0, 1);
    var _vita8_border = clamp(round(ds_grid_get(obj_menu.ds_menu_graphics, 3, 1)), 0, 1);
    var _vita8_vsync = clamp(round(ds_grid_get(obj_menu.ds_menu_graphics, 3, 2)), 0, 1);
    var _vita8_flicker = clamp(round(ds_grid_get(obj_menu.ds_menu_graphics, 3, 3)), 0, 2);
    var _vita8_timer = 1;
    var _vita8_counter = 1;
    var _vita8_palette = clamp(round(ds_grid_get(obj_menu.ds_menu_graphics, 3, 6)), 0, 8);
    var _vita8_brightness = 9;
    var _vita8_stretch = clamp(round(ds_grid_get(obj_menu.ds_menu_graphics, 3, 8)), 0, 1);
    if (file_exists(""vita_graphics.ini""))
    {
        ini_open(""vita_graphics.ini"");
        _vita8_scaling = clamp(round(ini_read_real(""Graphics"", ""Scaling"", _vita8_scaling)), 0, 1);
        _vita8_border = clamp(round(ini_read_real(""Graphics"", ""Border"", _vita8_border)), 0, 1);
        _vita8_vsync = clamp(round(ini_read_real(""Graphics"", ""VSync"", _vita8_vsync)), 0, 1);
        _vita8_flicker = clamp(round(ini_read_real(""Graphics"", ""Flicker"", _vita8_flicker)), 0, 2);
        _vita8_timer = clamp(round(ini_read_real(""Graphics"", ""Timer"", _vita8_timer)), 0, 1);
        _vita8_counter = clamp(round(ini_read_real(""Graphics"", ""Counter"", _vita8_counter)), 0, 1);
        _vita8_palette = clamp(round(ini_read_real(""Graphics"", ""Palette"", _vita8_palette)), 0, 8);
        _vita8_brightness = clamp(round(ini_read_real(""Graphics"", ""Brightness"", 9)), 0, 9);
        _vita8_stretch = clamp(round(ini_read_real(""Graphics"", ""Stretch"", _vita8_stretch)), 0, 1);
        ini_close();
    }
    ds_grid_set(obj_menu.ds_menu_graphics, 3, 0, _vita8_scaling);
    ds_grid_set(obj_menu.ds_menu_graphics, 3, 1, _vita8_border);
    ds_grid_set(obj_menu.ds_menu_graphics, 3, 2, _vita8_vsync);
    ds_grid_set(obj_menu.ds_menu_graphics, 3, 3, _vita8_flicker);
    ds_grid_set(obj_menu.ds_menu_graphics, 3, 4, _vita8_timer);
    ds_grid_set(obj_menu.ds_menu_graphics, 3, 5, _vita8_counter);
    ds_grid_set(obj_menu.ds_menu_graphics, 3, 6, _vita8_palette);
    ds_grid_set(obj_menu.ds_menu_graphics, 3, 7, _vita8_brightness);
    ds_grid_set(obj_menu.ds_menu_graphics, 3, 8, _vita8_stretch);
    change_fullscreen_scaling(_vita8_scaling);
    change_border_fill(_vita8_border);
    toggle_vsync(_vita8_vsync);
    change_flicker(_vita8_flicker);
    toggle_timer(_vita8_timer);
    toggle_counter(_vita8_counter);
    change_palette(_vita8_palette);
    change_resolution(_vita8_brightness);
    change_window_mode(_vita8_stretch);
    // Create the dedicated settings file immediately on migration so a normal
    // restart before opening Settings still keeps the corrected 100% default.
    if (!file_exists(""vita_graphics.ini""))
    {
        ini_open(""vita_graphics.ini"");
        ini_write_real(""Graphics"", ""Scaling"", _vita8_scaling);
        ini_write_real(""Graphics"", ""Border"", _vita8_border);
        ini_write_real(""Graphics"", ""VSync"", _vita8_vsync);
        ini_write_real(""Graphics"", ""Flicker"", _vita8_flicker);
        ini_write_real(""Graphics"", ""Timer"", _vita8_timer);
        ini_write_real(""Graphics"", ""Counter"", _vita8_counter);
        ini_write_real(""Graphics"", ""Palette"", _vita8_palette);
        ini_write_real(""Graphics"", ""Brightness"", _vita8_brightness);
        ini_write_real(""Graphics"", ""Stretch"", _vita8_stretch);
        ini_close();
    }
".Replace("\r\n", "\n");
    load = load.Insert(closing, block);
    imports.QueueReplace(loadCode, load);
}
imports.Import();
ScriptMessage("Void Stranger settings v8: dedicated Vita graphics persistence + 100% first-boot brightness.");
