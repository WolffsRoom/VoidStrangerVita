using System;
using UndertaleModLib;
using UndertaleModLib.Models;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Compiler;
EnsureDataLoaded();
var imports = new CodeImportGroup(Data) { AutoCreateAssets = true };

var gameCreate = Data.Code.ByName("gml_Object_obj_game_Create_0");
var gameText = GetDecompiledText(gameCreate).Replace("\r\n", "\n");
var oldInit = "global.current_controller_sprite = 2696;";
var newInit = "global.current_controller_sprite = spr_menu_controllerlayout_duals_dark;";
if (!gameText.Contains(oldInit)) throw new Exception("Controller sprite init not found");
gameText = gameText.Replace(oldInit, newInit);
imports.QueueReplace(gameCreate, gameText);

var dark = Data.Code.ByName("gml_GlobalScript_scr_get_dark_controller_sprite");
var darkText = GetDecompiledText(dark).Replace("\r\n", "\n");
var marker = "switch (arg0)\n    {";
if (!darkText.Contains(marker)) throw new Exception("Dark controller sprite switch not found");
darkText = darkText.Replace(marker, marker + "\n        case spr_menu_controllerlayout_duals_dark:\n            return spr_menu_controllerlayout_duals_dark;\n            break;");
imports.QueueReplace(dark, darkText);

imports.Import();
Console.WriteLine("PS Vita controller layout forced to spr_menu_controllerlayout_duals_dark.");
