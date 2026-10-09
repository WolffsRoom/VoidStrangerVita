using System;
using UndertaleModLib;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Compiler;
EnsureDataLoaded();
var draw=Data.Code.ByName("gml_Object_obj_universe_Draw_0");
if(draw==null) throw new Exception("Missing obj_universe Draw");
var t=GetDecompiledText(draw).Replace("\r\n","\n");
var old=@"    shader_reset();
    if (surface_exists(u_surf))
    {
        surface_free(u_surf);
    }
";
if(t.Contains(old)) {
    t=t.Replace(old,"    shader_reset();\n");
} else if(!t.Contains("surface_free(u_surf)")) {
    Console.WriteLine("Universe surface cache v28 already applied.");
    return;
} else throw new Exception("Unexpected obj_universe Draw surface-free block");
CodeImportGroup imports=new(Data){AutoCreateAssets=true};
imports.QueueReplace(draw,t);
imports.Import();
Console.WriteLine("Universe surface cache v28 applied: 256x176 surface persists until Destroy.");
