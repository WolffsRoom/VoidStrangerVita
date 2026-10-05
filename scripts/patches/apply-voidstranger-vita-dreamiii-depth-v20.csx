using System;
using UndertaleModLib;
using UndertaleModLib.Models;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Compiler;

EnsureDataLoaded();
var code = Data.Code.ByName("gml_Object_obj_cutscene_dreamIII_bandit_Create_0");
if (code == null) throw new Exception("Missing third dream scene controller");
var before = GetDecompiledText(code).Replace("\r\n", "\n");
if (before.Contains("VITA_DREAMIII_DEPTH_V20")) throw new Exception("Third dream depth patch already applied");

// On the Vita renderer, equal-depth instances are drawn in descending instance
// ID order. In rm_dreamIII_002 that places Gray and one horse behind the wagon.
// Give this scene an explicit background-to-foreground order matching Steam.
var prefix = @"
// VITA_DREAMIII_DEPTH_V20
if (room == rm_dreamIII_002)
{
    with (obj_horse) depth = -1;
    with (obj_gray_dreamIII) depth = -2;
}
";
CodeImportGroup imports = new(Data) { AutoCreateAssets = true };
imports.QueueReplace(code, prefix + before);
imports.Import();
Console.WriteLine("Third dream wagon depth compatibility patch applied.");
