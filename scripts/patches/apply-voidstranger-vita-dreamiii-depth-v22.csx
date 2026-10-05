using System;
using UndertaleModLib;
using UndertaleModLib.Models;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Compiler;

EnsureDataLoaded();
var bandit = Data.Code.ByName("gml_Object_obj_cutscene_dreamIII_bandit_Create_0");
var rescue = Data.Code.ByName("gml_Object_obj_cutscene_dreamIII_rescue_Create_0");
if (bandit == null || rescue == null) throw new Exception("Missing Dream III scene controllers");
var banditText = GetDecompiledText(bandit).Replace("\r\n", "\n");
var rescueText = GetDecompiledText(rescue).Replace("\r\n", "\n");

var oldBanditPrefix = @"if (room == rm_dreamIII_002)
{
    with (obj_horse)
    {
        depth = -1;
    }
    with (obj_gray_dreamIII)
    {
        depth = -2;
    }
}
";
if (!banditText.StartsWith(oldBanditPrefix)) throw new Exception("Missing v20 Dream III prefix");

var newBanditPrefix = @"// VITA_DREAMIII_DEPTH_V22: preserve back-to-front horse overlap.
if (room == rm_dreamIII_002)
{
    with (obj_horse)
    {
        if (y == 48)
        {
            depth = 401;
        }
        else if (y == 64)
        {
            depth = 399;
        }
    }
    with (obj_gray_dreamIII)
    {
        depth = -2;
    }
}
";
var rescuePrefix = @"// VITA_DREAMIII_DEPTH_V22: preserve back-to-front horse overlap.
if (room == rm_dreamIII_004)
{
    with (obj_horse)
    {
        if (y == 48)
        {
            depth = 401;
        }
        else if (y == 64)
        {
            depth = 399;
        }
    }
}
";

CodeImportGroup imports = new(Data) { AutoCreateAssets = true };
imports.QueueReplace(bandit, newBanditPrefix + banditText.Substring(oldBanditPrefix.Length));
imports.QueueReplace(rescue, rescuePrefix + rescueText);
imports.Import();
Console.WriteLine("Dream III horse depth v22 applied.");
