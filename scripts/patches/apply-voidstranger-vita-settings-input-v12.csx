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
ReplaceCode("gml_GlobalScript_scr_savesettings", t => {
    var marker = "function scr_savesettings()\n{";
    if (!t.Contains(marker)) throw new Exception("scr_savesettings function marker missing");
    var guard = marker + "\n    if (!instance_exists(obj_menu) || !ds_exists(obj_menu.ds_menu_graphics, ds_type_grid))\n    {\n        show_debug_message(\"VITA_SETTINGS_SAVE_SKIPPED invalid_menu_grid\");\n        exit;\n    }";
    var r = t.Replace(marker, guard);
    r = r.Replace("ini_write_real(\"Graphics\", \"Schema\", 11);", "ini_write_real(\"Graphics\", \"Schema\", 12);");
    r = r.Replace("VITA_SETTINGS_SAVE schema=11", "VITA_SETTINGS_SAVE schema=12");
    return r;
});
ReplaceCode("gml_GlobalScript_scr_loadsettings", t => {
    var r = t.Replace("if (_vita9_schema >= 11)", "if (_vita9_schema >= 12)");
    r = r.Replace("ini_write_real(\"Graphics\", \"Schema\", 11);", "ini_write_real(\"Graphics\", \"Schema\", 12);");
    r = r.Replace("VITA_SETTINGS_LOAD schema=11", "VITA_SETTINGS_LOAD schema=12");
    return r;
});
ReplaceCode("gml_Object_obj_menu_Other_3", t => {
    if (t.Contains("VITA_SETTINGS_FINAL_SAVE")) return t;
    return "scr_savesettings();\nshow_debug_message(\"VITA_SETTINGS_FINAL_SAVE before_grid_destroy\");\n" + t;
});
ReplaceCode("gml_Object_obj_game_Step_1", t => {
    var oldBlock = "    else if (keyboard_check(_key))\n    {\n        if (global.control_type != 0)\n        {\n            global.control_type = 0;\n            show_debug_message(\"Keyboard in use\");\n        }\n        _was_held = true;\n    }";
    var newBlock = "    else if (keyboard_check(_key))\n    {\n        // Vita keyboard compatibility must not switch the UI away from controller mode.\n        _was_held = true;\n    }";
    if (!t.Contains(oldBlock)) throw new Exception("keyboard fallback block missing");
    return t.Replace(oldBlock, newBlock);
});
imports.Import();
Console.WriteLine("Settings schema 12 + safe final save + controller-mode keyboard fallback applied.");
