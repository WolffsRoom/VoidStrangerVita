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
    if (t.Contains("\"Controls\", \"Schema\", 1")) return t;
    var needle = "        ini_write_real(\"Graphics\", \"Stretch\", ds_grid_get(obj_menu.ds_menu_graphics, 3, 8));\n        ini_close();\n    }\n    show_debug_message(\"VITA_SETTINGS_SAVE schema=12";
    if (!t.Contains(needle)) throw new Exception("save tail marker missing");
    var repl = "        ini_write_real(\"Graphics\", \"Stretch\", ds_grid_get(obj_menu.ds_menu_graphics, 3, 8));\n" +
               "        ini_write_real(\"Controls\", \"Schema\", 1);\n" +
               "        ini_write_real(\"Controls\", \"Up\", ds_grid_get(obj_menu.ds_menu_controller, 3, 0));\n" +
               "        ini_write_real(\"Controls\", \"Left\", ds_grid_get(obj_menu.ds_menu_controller, 3, 1));\n" +
               "        ini_write_real(\"Controls\", \"Right\", ds_grid_get(obj_menu.ds_menu_controller, 3, 2));\n" +
               "        ini_write_real(\"Controls\", \"Down\", ds_grid_get(obj_menu.ds_menu_controller, 3, 3));\n" +
               "        ini_write_real(\"Controls\", \"Action\", ds_grid_get(obj_menu.ds_menu_controller, 3, 4));\n" +
               "        ini_write_real(\"Controls\", \"Enter\", ds_grid_get(obj_menu.ds_menu_controller, 3, 5));\n" +
               "        ini_write_real(\"Controls\", \"Exit\", 32777);\n" +
               "        ini_write_real(\"Controls\", \"Movement\", ds_grid_get(obj_menu.ds_menu_controller, 3, 6));\n" +
               "        ini_close();\n    }\n" +
               "    show_debug_message(\"VITA_CONTROLS_SAVE up=\" + string(ds_grid_get(obj_menu.ds_menu_controller, 3, 0)) + \" left=\" + string(ds_grid_get(obj_menu.ds_menu_controller, 3, 1)) + \" right=\" + string(ds_grid_get(obj_menu.ds_menu_controller, 3, 2)) + \" down=\" + string(ds_grid_get(obj_menu.ds_menu_controller, 3, 3)) + \" action=\" + string(ds_grid_get(obj_menu.ds_menu_controller, 3, 4)) + \" enter=\" + string(ds_grid_get(obj_menu.ds_menu_controller, 3, 5)));\n" +
               "    show_debug_message(\"VITA_SETTINGS_SAVE schema=12";
    return t.Replace(needle, repl);
});

ReplaceCode("gml_GlobalScript_scr_loadsettings", t => {
    if (t.Contains("VITA_CONTROLS_LOAD schema=1")) return t;
    var marker = "    show_debug_message(\"VITA_SETTINGS_LOAD schema=12 brightness=\"";
    if (!t.Contains(marker)) throw new Exception("load tail marker missing");
    var block =
"    // Vita-native controller settings are authoritative over the legacy serialized blob.\n" +
"    var _vita_ctrl_schema = 0;\n" +
"    var _vita_ctrl_up = 32781;\n" +
"    var _vita_ctrl_left = 32783;\n" +
"    var _vita_ctrl_right = 32784;\n" +
"    var _vita_ctrl_down = 32782;\n" +
"    var _vita_ctrl_action = 32769;\n" +
"    var _vita_ctrl_enter = 32778;\n" +
"    var _vita_ctrl_exit = 32777;\n" +
"    var _vita_ctrl_movement = 0;\n" +
"    if (file_exists(\"vita_graphics.ini\"))\n" +
"    {\n" +
"        ini_open(\"vita_graphics.ini\");\n" +
"        _vita_ctrl_schema = round(ini_read_real(\"Controls\", \"Schema\", 0));\n" +
"        if (_vita_ctrl_schema >= 1)\n" +
"        {\n" +
"            _vita_ctrl_up = round(ini_read_real(\"Controls\", \"Up\", 32781));\n" +
"            _vita_ctrl_left = round(ini_read_real(\"Controls\", \"Left\", 32783));\n" +
"            _vita_ctrl_right = round(ini_read_real(\"Controls\", \"Right\", 32784));\n" +
"            _vita_ctrl_down = round(ini_read_real(\"Controls\", \"Down\", 32782));\n" +
"            _vita_ctrl_action = round(ini_read_real(\"Controls\", \"Action\", 32769));\n" +
"            _vita_ctrl_enter = round(ini_read_real(\"Controls\", \"Enter\", 32778));\n" +
"            _vita_ctrl_exit = round(ini_read_real(\"Controls\", \"Exit\", 32777));\n" +
"            _vita_ctrl_movement = clamp(round(ini_read_real(\"Controls\", \"Movement\", 0)), 0, 4);\n" +
"        }\n" +
"        ini_close();\n" +
"    }\n" +
"    if (_vita_ctrl_up < 32769 || _vita_ctrl_up > 32784) _vita_ctrl_up = 32781;\n" +
"    if (_vita_ctrl_left < 32769 || _vita_ctrl_left > 32784) _vita_ctrl_left = 32783;\n" +
"    if (_vita_ctrl_right < 32769 || _vita_ctrl_right > 32784) _vita_ctrl_right = 32784;\n" +
"    if (_vita_ctrl_down < 32769 || _vita_ctrl_down > 32784) _vita_ctrl_down = 32782;\n" +
"    if (_vita_ctrl_action < 32769 || _vita_ctrl_action > 32784) _vita_ctrl_action = 32769;\n" +
"    if (_vita_ctrl_enter < 32769 || _vita_ctrl_enter > 32784) _vita_ctrl_enter = 32778;\n" +
"    if (_vita_ctrl_exit < 32769 || _vita_ctrl_exit > 32784) _vita_ctrl_exit = 32777;\n" +
"    ds_grid_set(obj_menu.ds_menu_controller, 3, 0, _vita_ctrl_up);\n" +
"    ds_grid_set(obj_menu.ds_menu_controller, 3, 1, _vita_ctrl_left);\n" +
"    ds_grid_set(obj_menu.ds_menu_controller, 3, 2, _vita_ctrl_right);\n" +
"    ds_grid_set(obj_menu.ds_menu_controller, 3, 3, _vita_ctrl_down);\n" +
"    ds_grid_set(obj_menu.ds_menu_controller, 3, 4, _vita_ctrl_action);\n" +
"    ds_grid_set(obj_menu.ds_menu_controller, 3, 5, _vita_ctrl_enter);\n" +
"    ds_grid_set(obj_menu.ds_menu_controller, 3, 6, _vita_ctrl_movement);\n" +
"    global.ctrl_up = _vita_ctrl_up;\n" +
"    global.ctrl_left = _vita_ctrl_left;\n" +
"    global.ctrl_right = _vita_ctrl_right;\n" +
"    global.ctrl_down = _vita_ctrl_down;\n" +
"    global.ctrl_action = _vita_ctrl_action;\n" +
"    global.ctrl_enter = _vita_ctrl_enter;\n" +
"    global.ctrl_exit = _vita_ctrl_exit;\n" +
"    global.control_type = 1;\n" +
"    global.current_controller = 0;\n" +
"    global.current_controller_sprite = 2709;\n" +
"    change_movement(_vita_ctrl_movement);\n" +
"    ini_open(\"vita_graphics.ini\");\n" +
"    ini_write_real(\"Controls\", \"Schema\", 1);\n" +
"    ini_write_real(\"Controls\", \"Up\", _vita_ctrl_up);\n" +
"    ini_write_real(\"Controls\", \"Left\", _vita_ctrl_left);\n" +
"    ini_write_real(\"Controls\", \"Right\", _vita_ctrl_right);\n" +
"    ini_write_real(\"Controls\", \"Down\", _vita_ctrl_down);\n" +
"    ini_write_real(\"Controls\", \"Action\", _vita_ctrl_action);\n" +
"    ini_write_real(\"Controls\", \"Enter\", _vita_ctrl_enter);\n" +
"    ini_write_real(\"Controls\", \"Exit\", _vita_ctrl_exit);\n" +
"    ini_write_real(\"Controls\", \"Movement\", _vita_ctrl_movement);\n" +
"    ini_close();\n" +
"    show_debug_message(\"VITA_CONTROLS_LOAD schema=1 up=\" + string(_vita_ctrl_up) + \" left=\" + string(_vita_ctrl_left) + \" right=\" + string(_vita_ctrl_right) + \" down=\" + string(_vita_ctrl_down) + \" action=\" + string(_vita_ctrl_action) + \" enter=\" + string(_vita_ctrl_enter) + \" exit=\" + string(_vita_ctrl_exit));\n";
    return t.Replace(marker, block + marker);
});

imports.Import();
Console.WriteLine("Vita Controls schema 1 applied: dedicated authoritative bindings + legacy migration.");
