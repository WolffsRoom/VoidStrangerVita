using UndertaleModLib.Compiler;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Models;
using System.Text.RegularExpressions;

EnsureDataLoaded();
CodeImportGroup imports = new(Data) { AutoCreateAssets = true };

// Translation lookup is intentionally left untouched here.
// apply-voidstranger-vita-settings.csx owns scrScript and uses direct included/extracted arrays;
// overriding it with TranslationGet reintroduces CALLV/undefined failures in Butterscotch.

// Preserve only the safe dialogue rendering optimization: draw directly at
// the final coordinates instead of switching through a 212x40 surface.
UndertaleCode textbox = Data.Code.ByName("gml_Object_obj_textbox_Draw_0");
string textboxGml = GetDecompiledText(textbox).Replace("\r\n", "\n");
string pattern = @"if \(!surface_exists\(text_surf\)\)\n\{.*?\n\}\nif \(surface_exists\(text_surf\)\)\n\{.*?\n\}";
string direct = "draw_set_font(font);\ndraw_set_halign(fa_left);\ndraw_set_valign(fa_top);\ndraw_text_ext_color(text_x, text_y + initial_text_y, substr, text_height, 1000, c, c, c, c, 1);";
string patchedTextbox = Regex.Replace(textboxGml, pattern, direct, RegexOptions.Singleline);
if (patchedTextbox == textboxGml) throw new Exception("Textbox surface block not found");
imports.QueueReplace(textbox, patchedTextbox);

imports.Import();
ScriptMessage("Void Stranger Vita safe 60 FPS data patch applied.");
