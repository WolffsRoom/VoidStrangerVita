using UndertaleModLib.Compiler;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Models;
using System.Text.RegularExpressions;

EnsureDataLoaded();

CodeImportGroup imports = new(Data) { AutoCreateAssets = true };

void KeepSurface(string codeName, string variable) {
    UndertaleCode code = Data.Code.ByName(codeName);
    if (code == null) throw new Exception("Missing code: " + codeName);
    string gml = GetDecompiledText(code);
    string pattern = @"\s*if\s*\(surface_exists\(" + Regex.Escape(variable) +
                     @"\)\)\s*\{\s*surface_free\(" + Regex.Escape(variable) + @"\);\s*\}";
    Regex matcher = new Regex(pattern);
    MatchCollection matches = matcher.Matches(gml);
    if (matches.Count != 1) throw new Exception("Expected one surface free block in " + codeName + ", got " + matches.Count);
    imports.QueueReplace(code, matcher.Replace(gml, "", 1));
}

KeepSurface("gml_Object_obj_errormessages_Draw_0", "error_surf");
KeepSurface("gml_Object_obj_textbox_Draw_0", "text_surf");
KeepSurface("gml_Object_obj_floor_brane_Draw_0", "b1_surf");
KeepSurface("gml_Object_obj_floor_hpn4_Draw_0", "b2_surf");

void FreeOnDestroy(string objectName, string variable) {
    UndertaleGameObject obj = Data.GameObjects.ByName(objectName);
    if (obj == null) throw new Exception("Missing object: " + objectName);
    imports.QueueAppend(obj.EventHandlerFor(EventType.Destroy, Data),
        "\nif (surface_exists(" + variable + ")) { surface_free(" + variable + "); " + variable + " = -1; }");
}

FreeOnDestroy("obj_errormessages", "error_surf");
FreeOnDestroy("obj_textbox", "text_surf");
FreeOnDestroy("obj_floor_brane", "b1_surf");
FreeOnDestroy("obj_floor_hpn4", "b2_surf");

// Uniform locations and values persist in the linked program. Updating all
// four palette uniforms every game frame is unnecessary; refresh on the first
// draw and while the settings menu is actively previewing a palette.
UndertaleCode finalDraw = Data.Code.ByName("gml_Object_obj_game_Draw_77");
string finalGml = GetDecompiledText(finalDraw);
finalGml = finalGml.Replace("\r\n", "\n");
string uniformStart = "if (global.shaders_work)\n{\n    shader_set(shader_palette);";
string uniformEnd = "    }\n}\ngpu_set_blendenable(false);";
if (!finalGml.Contains(uniformStart) || !finalGml.Contains(uniformEnd))
    throw new Exception("Palette uniform block did not match");
finalGml = finalGml.Replace(uniformStart,
    "if (global.shaders_work)\n{\n    shader_set(shader_palette);\n    if (global.menu || !variable_instance_exists(id, \"vita_palette_uniforms_ready\") || !vita_palette_uniforms_ready)\n    {");
finalGml = finalGml.Replace(uniformEnd,
    "        vita_palette_uniforms_ready = true;\n    }\n}\ngpu_set_blendenable(false);");
imports.QueueReplace(finalDraw, finalGml);

imports.Import();
ScriptMessage("Void Stranger Vita performance patches applied.");
