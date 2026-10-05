using System;
using UndertaleModLib;
using UndertaleModLib.Models;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Compiler;
EnsureDataLoaded();
var imports = new CodeImportGroup(Data) { AutoCreateAssets = true };
var code = Data.Code.ByName("gml_Object_obj_game_Step_1");
if (code == null) throw new Exception("Missing gml_Object_obj_game_Step_1");
var text = GetDecompiledText(code).Replace("\r\n", "\n");
var oldBlock = @"if (keyboard_check(_key))
    {
        if (global.control_type != 0)
        {
            global.control_type = 0;
            show_debug_message(""Keyboard in use"");
        }
        _was_held = true;
    }
    else if (global.current_controller != -1 && gamepad_button_check(global.current_controller, _btn))
    {
        if (global.control_type != 1)
        {
            global.control_type = 1;
            show_debug_message(""Controller in use"");
        }
        _was_held = true;
    }";
var newBlock = @"if (global.current_controller != -1 && gamepad_button_check(global.current_controller, _btn))
    {
        if (global.control_type != 1)
        {
            global.control_type = 1;
            show_debug_message(""Controller in use"");
        }
        _was_held = true;
    }
    else if (keyboard_check(_key))
    {
        if (global.control_type != 0)
        {
            global.control_type = 0;
            show_debug_message(""Keyboard in use"");
        }
        _was_held = true;
    }";
if (!text.Contains(oldBlock)) throw new Exception("Input priority block not found");
text = text.Replace(oldBlock, newBlock);
imports.QueueReplace(code, text);
imports.Import();
Console.WriteLine("PS Vita controller input now wins over keyboard compatibility input.");
