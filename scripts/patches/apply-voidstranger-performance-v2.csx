using UndertaleModLib.Compiler;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Models;
using System.Text.RegularExpressions;

EnsureDataLoaded();
CodeImportGroup imports = new(Data) { AutoCreateAssets = true };

void ReplaceCode(string name, Func<string, string> transform) {
    UndertaleCode code = Data.Code.ByName(name);
    if (code == null) throw new Exception("Missing code: " + name);
    string before = GetDecompiledText(code).Replace("\r\n", "\n");
    string after = transform(before);
    if (after == before) throw new Exception("Patch did not change " + name);
    imports.QueueReplace(code, after);
}

// Do not patch scrScript here. The Vita settings patch performs a direct
// included/extracted array lookup that is both cheaper and compatible with Butterscotch.

// The original creates an extra render target for 212x40 text and redraws it
// for every revealed character. Drawing at the equivalent final coordinates
// removes two surface switches and a surface composite without changing layout.
ReplaceCode("gml_Object_obj_textbox_Draw_0", gml => {
    string pattern = @"if \(!surface_exists\(text_surf\)\)\n\{.*?\n\}\nif \(surface_exists\(text_surf\)\)\n\{.*?\n\}";
    string direct = "draw_set_font(font);\ndraw_set_halign(fa_left);\ndraw_set_valign(fa_top);\ndraw_text_ext_color(text_x, text_y + initial_text_y, substr, text_height, 1000, c, c, c, c, 1);";
    return Regex.Replace(gml, pattern, direct, RegexOptions.Singleline);
});

// scr_input_check_pressed(4) is normally the action input, but the Steam
// controller/keyboard arbitration can miss it after Start opens the menu.
// Accept the Vita's explicit Z/X-confirm event as an additional edge.
ReplaceCode("gml_Object_obj_menu_Step_0", gml => {
    string needle = "input_enter_p = scr_input_check_pressed(4);";
    if (!gml.Contains(needle)) throw new Exception("Menu confirm assignment not found");
    return gml.Replace(needle, "input_enter_p = scr_input_check_pressed(4) || keyboard_check_pressed(ord(\"Z\"));");
});

imports.Import();
ScriptMessage("Void Stranger Vita performance/input v2 patches applied.");
