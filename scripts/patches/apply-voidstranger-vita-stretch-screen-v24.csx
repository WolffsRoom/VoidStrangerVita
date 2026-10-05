using System;
using UndertaleModLib;
using UndertaleModLib.Models;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Compiler;

EnsureDataLoaded();
var code = Data.Code.ByName("gml_Object_obj_game_Draw_77");
if (code == null) throw new Exception("Missing obj_game Draw 77");
var text = GetDecompiledText(code).Replace("\r\n", "\n");

var oldBlock = @"var vita_graphics_menu_fullscreen = instance_exists(obj_menu) && obj_menu.page == 2;
if (vita_graphics_menu_fullscreen || (variable_global_exists(""vita_stretch_screen"") && global.vita_stretch_screen == 1))
{
    xsceel = window_get_width() / sx;
    ysceel = window_get_height() / sy;
}";
var oldBlockWithoutMenuForce = @"if (variable_global_exists(""vita_stretch_screen"") && global.vita_stretch_screen == 1)
{
    xsceel = window_get_width() / sx;
    ysceel = window_get_height() / sy;
}";
var newBlock = @"// VITA_STRETCH_SCREEN_V24: window_get_width/height fall back to the
// logical 224x144 GameMaker window on Vita. Request the physical host size
// explicitly so the renderer receives a non-uniform scale and presents the
// application surface across the full 960x544 display.
if (variable_global_exists(""vita_stretch_screen"") && global.vita_stretch_screen == 1)
{
    xsceel = 960 / sx;
    ysceel = 544 / sy;
}";

if (text.Contains(oldBlock)) text = text.Replace(oldBlock, newBlock);
else if (text.Contains(oldBlockWithoutMenuForce)) text = text.Replace(oldBlockWithoutMenuForce, newBlock);
else if (text.Contains("xsceel = 960 / sx;") && text.Contains("ysceel = 544 / sy;")) {
    Console.WriteLine("Stretch Screen v24 already applied.");
    return;
} else throw new Exception("Stretch presentation block not found");

CodeImportGroup imports = new(Data) { AutoCreateAssets = true };
imports.QueueReplace(code, text);
imports.Import();
Console.WriteLine("Void Stranger Stretch Screen v24 applied: OFF=FIT, ON=960x544.");
