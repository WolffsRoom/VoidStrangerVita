using System;
using UndertaleModLib;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Compiler;
EnsureDataLoaded();
var create=Data.Code.ByName("gml_Object_obj_bg_Create_0");
var step=Data.Code.ByName("gml_Object_obj_bg_Step_0");
if(create==null||step==null) throw new Exception("Missing obj_bg code");
var ct=GetDecompiledText(create).Replace("\r\n","\n");
var st=GetDecompiledText(step).Replace("\r\n","\n");
if(!ct.Contains("vita_bg_cached")) ct += "\n// VITA_ALPHABET_BG_CACHE_V27\nvita_bg_cached = false;\n";
if(!st.Contains("VITA_ALPHABET_BG_CACHE_V27")) {
var fast=@"// VITA_ALPHABET_BG_CACHE_V27: rm_0234 contains 88 static background
// helpers. Their floor/pit classification depends only on static room geometry,
// so calculate it once after room creation instead of doing collision queries
// for every helper on every frame.
if (room == rm_0234)
{
    if (vita_bg_cached == false)
    {
        if (place_meeting(x, y - 16, obj_collision) && !place_meeting(x, y - 16, obj_glassfloor) && !place_meeting(x - 16, y, obj_rest) && !place_meeting(x + 16, y, obj_rest) && y > 8)
        {
            sprite_index = spr_floor;
        }
        else
        {
            sprite_index = spr_pit;
        }
        vita_bg_cached = true;
    }
    exit;
}
";
st=fast+"\n"+st;
}
CodeImportGroup imports=new(Data){AutoCreateAssets=true};
imports.QueueReplace(create,ct);
imports.QueueReplace(step,st);
imports.Import();
Console.WriteLine("Alphabet rm_0234 bg cache v27 applied.");
