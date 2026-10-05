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
    var r = t.Replace("ini_write_real(\"Graphics\", \"Schema\", 9);", "ini_write_real(\"Graphics\", \"Schema\", 10);");
    return r;
});

ReplaceCode("gml_GlobalScript_scr_loadsettings", t => {
    var r = t.Replace("if (_vita9_schema >= 9)", "if (_vita9_schema >= 10)");
    r = r.Replace("ini_write_real(\"Graphics\", \"Schema\", 9);", "ini_write_real(\"Graphics\", \"Schema\", 10);");
    return r;
});

ReplaceCode("gml_GlobalScript_any_settings_file_exists", t => {
    var needle = "    if (!_file_is_usable)\n    {\n        return false;\n    }\n    else\n    {\n        return true;\n    }";
    var repl = "    if (!_file_is_usable && file_exists(\"vita_graphics.ini\"))\n    {\n        return true;\n    }\n    if (!_file_is_usable)\n    {\n        return false;\n    }\n    else\n    {\n        return true;\n    }";
    if (!t.Contains(needle)) throw new Exception("any_settings tail not found");
    return t.Replace(needle, repl);
});

ReplaceCode("gml_Object_obj_game_Create_0", t => {
    var r = t.Replace("global.control_type = 0;", "global.control_type = 1;");
    r = r.Replace("global.current_controller = -1;", "global.current_controller = 0;");
    return r;
});

imports.Import();
Console.WriteLine("Settings schema v10 + persistent Vita INI + controller-first boot applied.");
