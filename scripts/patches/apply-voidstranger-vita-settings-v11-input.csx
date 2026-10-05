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

ReplaceCode("gml_GlobalScript_scr_loadsettings", t => {
    var r = t.Replace("if (_vita9_schema >= 10)", "if (_vita9_schema >= 11)");
    r = r.Replace("ini_write_real(\"Graphics\", \"Schema\", 10);", "ini_write_real(\"Graphics\", \"Schema\", 11);");
    var end = r.LastIndexOf("\n}");
    if (end < 0) throw new Exception("scr_loadsettings end not found");
    var log = "\n    show_debug_message(\"VITA_SETTINGS_LOAD schema=11 brightness=\" + string(ds_grid_get(obj_menu.ds_menu_graphics, 3, 7)) + \" timer=\" + string(ds_grid_get(obj_menu.ds_menu_graphics, 3, 4)) + \" counter=\" + string(ds_grid_get(obj_menu.ds_menu_graphics, 3, 5)) + \" palette=\" + string(ds_grid_get(obj_menu.ds_menu_graphics, 3, 6)) + \" stretch=\" + string(ds_grid_get(obj_menu.ds_menu_graphics, 3, 8)));";
    return r.Insert(end, log);
});

ReplaceCode("gml_GlobalScript_scr_savesettings", t => {
    var r = t.Replace("ini_write_real(\"Graphics\", \"Schema\", 10);", "ini_write_real(\"Graphics\", \"Schema\", 11);");
    var end = r.LastIndexOf("\n}");
    if (end < 0) throw new Exception("scr_savesettings end not found");
    var log = "\n    show_debug_message(\"VITA_SETTINGS_SAVE schema=11 brightness=\" + string(ds_grid_get(obj_menu.ds_menu_graphics, 3, 7)) + \" timer=\" + string(ds_grid_get(obj_menu.ds_menu_graphics, 3, 4)) + \" counter=\" + string(ds_grid_get(obj_menu.ds_menu_graphics, 3, 5)) + \" palette=\" + string(ds_grid_get(obj_menu.ds_menu_graphics, 3, 6)) + \" stretch=\" + string(ds_grid_get(obj_menu.ds_menu_graphics, 3, 8)));";
    return r.Insert(end, log);
});

ReplaceCode("gml_Object_obj_game_KeyPress_113", t => t.Replace("ds_menu_graphics, 3, 6", "ds_menu_graphics, 3, 4"));
ReplaceCode("gml_Object_obj_game_KeyPress_114", t => t.Replace("ds_menu_graphics, 3, 7", "ds_menu_graphics, 3, 5"));
ReplaceCode("gml_Object_obj_game_KeyPress_115", t => t.Replace("ds_menu_graphics, 3, 0", "ds_menu_graphics, 3, 8"));

ReplaceCode("gml_Object_obj_help_display_oneoff_Step_0", t => {
    var r = t.Replace("ds_menu_graphics, 3, 6", "ds_menu_graphics, 3, 4");
    r = r.Replace("ds_menu_graphics, 3, 7", "ds_menu_graphics, 3, 5");
    r = r.Replace("ds_menu_graphics, 3, 0", "ds_menu_graphics, 3, 8");
    return r;
});

ReplaceCode("gml_Object_obj_menu_Step_0", t => {
    var needle = "                variable_global_set(ds_grid_get(ds_, 2, array_get(menu_option, page)), cb);\n                cb = -4;";
    var repl = "                variable_global_set(ds_grid_get(ds_, 2, array_get(menu_option, page)), cb);\n                show_debug_message(\"VITA_CTRL_ASSIGN row=\" + string(array_get(menu_option, page)) + \" button=\" + string(cb));\n                cb = -4;";
    if (!t.Contains(needle)) throw new Exception("controller assignment block not found");
    return t.Replace(needle, repl);
});

imports.Import();
Console.WriteLine("Settings schema 11 + stale graphics rows + controller assignment diagnostics applied.");
