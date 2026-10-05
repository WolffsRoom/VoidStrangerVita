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
if (!save.Contains("Vita Graphics Persist v9")) {
    int closing = save.LastIndexOf('}');
    if (closing < 0) throw new Exception("scr_savesettings closing brace not found");
    string block = @"
    // Vita Graphics Persist v9: named settings + schema marker.
    if (instance_exists(obj_menu))
    {
        ini_open(""vita_graphics.ini"");
        ini_write_real(""Graphics"", ""Schema"", 9);
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
    }
".Replace("\r\n", "\n");
    save = save.Insert(closing, block);
    imports.QueueReplace(saveCode, save);
}

string load = Norm(loadCode);
if (!load.Contains("Vita Graphics Persist v9")) {
    int closing = load.LastIndexOf('}');
    if (closing < 0) throw new Exception("scr_loadsettings closing brace not found");
    string block = @"
    // Vita Graphics Persist v9: older v7/v8 files may already contain the
    // incorrect migrated values (10% brightness, Timer/Counter ON). Migrate
    // those files exactly once, then trust every user-selected value normally.
    var _vita9_scaling = clamp(round(ds_grid_get(obj_menu.ds_menu_graphics, 3, 0)), 0, 1);
    var _vita9_border = clamp(round(ds_grid_get(obj_menu.ds_menu_graphics, 3, 1)), 0, 1);
    var _vita9_vsync = clamp(round(ds_grid_get(obj_menu.ds_menu_graphics, 3, 2)), 0, 1);
    var _vita9_flicker = clamp(round(ds_grid_get(obj_menu.ds_menu_graphics, 3, 3)), 0, 2);
    var _vita9_timer = 1;
    var _vita9_counter = 1;
    var _vita9_palette = clamp(round(ds_grid_get(obj_menu.ds_menu_graphics, 3, 6)), 0, 8);
    var _vita9_brightness = 9;
    var _vita9_stretch = clamp(round(ds_grid_get(obj_menu.ds_menu_graphics, 3, 8)), 0, 1);
    var _vita9_schema = 0;
    if (file_exists(""vita_graphics.ini""))
    {
        ini_open(""vita_graphics.ini"");
        _vita9_schema = round(ini_read_real(""Graphics"", ""Schema"", 0));
        if (_vita9_schema >= 9)
        {
            _vita9_scaling = clamp(round(ini_read_real(""Graphics"", ""Scaling"", _vita9_scaling)), 0, 1);
            _vita9_border = clamp(round(ini_read_real(""Graphics"", ""Border"", _vita9_border)), 0, 1);
            _vita9_vsync = clamp(round(ini_read_real(""Graphics"", ""VSync"", _vita9_vsync)), 0, 1);
            _vita9_flicker = clamp(round(ini_read_real(""Graphics"", ""Flicker"", _vita9_flicker)), 0, 2);
            _vita9_timer = clamp(round(ini_read_real(""Graphics"", ""Timer"", 1)), 0, 1);
            _vita9_counter = clamp(round(ini_read_real(""Graphics"", ""Counter"", 1)), 0, 1);
            _vita9_palette = clamp(round(ini_read_real(""Graphics"", ""Palette"", _vita9_palette)), 0, 8);
            _vita9_brightness = clamp(round(ini_read_real(""Graphics"", ""Brightness"", 9)), 0, 9);
            _vita9_stretch = clamp(round(ini_read_real(""Graphics"", ""Stretch"", _vita9_stretch)), 0, 1);
        }
        ini_close();
    }
    ds_grid_set(obj_menu.ds_menu_graphics, 3, 0, _vita9_scaling);
    ds_grid_set(obj_menu.ds_menu_graphics, 3, 1, _vita9_border);
    ds_grid_set(obj_menu.ds_menu_graphics, 3, 2, _vita9_vsync);
    ds_grid_set(obj_menu.ds_menu_graphics, 3, 3, _vita9_flicker);
    ds_grid_set(obj_menu.ds_menu_graphics, 3, 4, _vita9_timer);
    ds_grid_set(obj_menu.ds_menu_graphics, 3, 5, _vita9_counter);
    ds_grid_set(obj_menu.ds_menu_graphics, 3, 6, _vita9_palette);
    ds_grid_set(obj_menu.ds_menu_graphics, 3, 7, _vita9_brightness);
    ds_grid_set(obj_menu.ds_menu_graphics, 3, 8, _vita9_stretch);
    change_fullscreen_scaling(_vita9_scaling);
    change_border_fill(_vita9_border);
    toggle_vsync(_vita9_vsync);
    change_flicker(_vita9_flicker);
    toggle_timer(_vita9_timer);
    toggle_counter(_vita9_counter);
    change_palette(_vita9_palette);
    change_resolution(_vita9_brightness);
    change_window_mode(_vita9_stretch);
    ini_open(""vita_graphics.ini"");
    ini_write_real(""Graphics"", ""Schema"", 9);
    ini_write_real(""Graphics"", ""Scaling"", _vita9_scaling);
    ini_write_real(""Graphics"", ""Border"", _vita9_border);
    ini_write_real(""Graphics"", ""VSync"", _vita9_vsync);
    ini_write_real(""Graphics"", ""Flicker"", _vita9_flicker);
    ini_write_real(""Graphics"", ""Timer"", _vita9_timer);
    ini_write_real(""Graphics"", ""Counter"", _vita9_counter);
    ini_write_real(""Graphics"", ""Palette"", _vita9_palette);
    ini_write_real(""Graphics"", ""Brightness"", _vita9_brightness);
    ini_write_real(""Graphics"", ""Stretch"", _vita9_stretch);
    ini_close();
".Replace("\r\n", "\n");
    load = load.Insert(closing, block);
    imports.QueueReplace(loadCode, load);
}
imports.Import();
ScriptMessage("Void Stranger settings v9: one-time legacy migration + persistent named graphics settings.");
