using System;
using System.Linq;
using UndertaleModLib.Compiler;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Models;

EnsureDataLoaded();
var imports = new CodeImportGroup(Data) { AutoCreateAssets = true };
// The Lev minifight spaces its command list using font_get_size(), which is
// not implemented by Butterscotch. Measure the font that is actually active
// at draw time instead. string_height_ext() is also stubbed, so choose the
// one-line battle box using the implemented string_width() metric.
var battleCreate = Data.Code.ByName("gml_Object_obj_battle_lev_Create_0");
if (battleCreate == null) throw new Exception("Missing obj_battle_lev Create");
var battleCreateGml = GetDecompiledText(battleCreate).Replace("\r\n", "\n");
if (battleCreateGml.Contains("fontSize = font_get_size(fnt_text_12);"))
    battleCreateGml = battleCreateGml.Replace("fontSize = font_get_size(fnt_text_12);", "fontSize = 12;");
else if (!battleCreateGml.Contains("fontSize = 12;"))
    throw new Exception("Lev battle fontSize assignment not found");
imports.QueueReplace(battleCreate, battleCreateGml);

var battleDraw = Data.Code.ByName("gml_Object_obj_battle_lev_Draw_0");
if (battleDraw == null) throw new Exception("Missing obj_battle_lev Draw");
var battleDrawGml = GetDecompiledText(battleDraw).Replace("\r\n", "\n");
var scaledSpacing = "var command_scale = 0.8;\nvar option_line_height = (string_height(\"Ag\") + buffer) * command_scale;";
var unscaledSpacing = "var option_line_height = string_height(\"Ag\") + buffer;";
if (battleDrawGml.Contains(unscaledSpacing))
{
    battleDrawGml = battleDrawGml.Replace(unscaledSpacing, scaledSpacing);
}
else if (!battleDrawGml.Contains(scaledSpacing))
{
    var bufferNeedle = "var buffer = 4;";
    if (!battleDrawGml.Contains(bufferNeedle)) throw new Exception("Lev battle buffer assignment not found");
    battleDrawGml = battleDrawGml.Replace(bufferNeedle, bufferNeedle + "\n" + scaledSpacing);
}
battleDrawGml = battleDrawGml.Replace("((fontSize + buffer) * i)", "(option_line_height * i)");
battleDrawGml = battleDrawGml.Replace(
    "draw_text_color(optionX, optionY + (option_line_height * i), o_text, c_white, c_white, c_white, c_white, 1);",
    "draw_text_transformed_color(optionX, optionY + (option_line_height * i), o_text, command_scale, command_scale, 0, c_white, c_white, c_white, c_white, 1);");
battleDrawGml = battleDrawGml.Replace(
    "draw_text_color(optionX, optionY + (option_line_height * i), o_text, c_gray, c_gray, c_gray, c_gray, 1);",
    "draw_text_transformed_color(optionX, optionY + (option_line_height * i), o_text, command_scale, command_scale, 0, c_gray, c_gray, c_gray, c_gray, 1);");
var heightExt = "if (string_height_ext(str, string_height(global.text_font), 220) == string_height(global.text_font))";
if (battleDrawGml.Contains(heightExt))
{
    battleDrawGml = battleDrawGml.Replace(heightExt,
        "var battle_line_height = string_height(\"Ag\");\n    if (string_width(str) <= 220)");
}
else if (!battleDrawGml.Contains("if (string_width(str) <= 220)"))
    throw new Exception("Lev battle textbox height test not found");
battleDrawGml = battleDrawGml.Replace(
    "draw_text_ext_color(room_width * 0.5, 8, str, string_height(global.text_font), 220, c_white, c_white, c_white, c_white, 1);",
    "draw_text_ext_color(room_width * 0.5, 8, str, battle_line_height, 220, c_white, c_white, c_white, c_white, 1);");
imports.QueueReplace(battleDraw, battleDrawGml);

// The same unsupported GameMaker font metric pattern is duplicated across the
// dream battle variants. Apply the already-validated Lev layout consistently:
// 80% command labels, line spacing based on the active rendered font, and the
// supported string_width() test for choosing the compact battle textbox.
string[,] relatedBattles = {
    { "obj_dream_battle", "gml_Object_obj_dream_battle_Create_0", "gml_Object_obj_dream_battle_Draw_0", "1" },
    { "obj_dreamIII_battle", "gml_Object_obj_dreamIII_battle_Create_0", "gml_Object_obj_dreamIII_battle_Draw_0", "1" },
    { "obj_battle_dreamXI", "gml_Object_obj_battle_dreamXI_Create_0", "gml_Object_obj_battle_dreamXI_Draw_0", "1" },
    { "obj_battle_dummy", "gml_Object_obj_battle_dummy_Create_0", "gml_Object_obj_battle_dummy_Draw_64", "0" }
};
for (int battleIndex = 0; battleIndex < relatedBattles.GetLength(0); battleIndex++)
{
    var label = relatedBattles[battleIndex, 0];
    var createName = relatedBattles[battleIndex, 1];
    var drawName = relatedBattles[battleIndex, 2];
    var patchBattleText = relatedBattles[battleIndex, 3] == "1";

    var relatedCreate = Data.Code.ByName(createName);
    if (relatedCreate == null) throw new Exception("Missing " + createName);
    var relatedCreateGml = GetDecompiledText(relatedCreate).Replace("\r\n", "\n");
    if (relatedCreateGml.Contains("fontSize = font_get_size(fnt_text_12);"))
        relatedCreateGml = relatedCreateGml.Replace("fontSize = font_get_size(fnt_text_12);", "fontSize = 12;");
    else if (!relatedCreateGml.Contains("fontSize = 12;"))
        throw new Exception(label + ": fontSize assignment not found");
    imports.QueueReplace(relatedCreate, relatedCreateGml);

    var relatedDraw = Data.Code.ByName(drawName);
    if (relatedDraw == null) throw new Exception("Missing " + drawName);
    var relatedDrawGml = GetDecompiledText(relatedDraw).Replace("\r\n", "\n");
    var relatedScaledSpacing = "var command_scale = 0.8;\nvar option_line_height = (string_height(\"Ag\") + buffer) * command_scale;";
    var relatedUnscaledSpacing = "var option_line_height = string_height(\"Ag\") + buffer;";
    if (relatedDrawGml.Contains(relatedUnscaledSpacing))
    {
        relatedDrawGml = relatedDrawGml.Replace(relatedUnscaledSpacing, relatedScaledSpacing);
    }
    else if (!relatedDrawGml.Contains(relatedScaledSpacing))
    {
        var relatedBufferNeedle = "var buffer = 4;";
        if (!relatedDrawGml.Contains(relatedBufferNeedle)) throw new Exception(label + ": buffer assignment not found");
        relatedDrawGml = relatedDrawGml.Replace(relatedBufferNeedle, relatedBufferNeedle + "\n" + relatedScaledSpacing);
    }
    relatedDrawGml = relatedDrawGml.Replace("((fontSize + buffer) * i)", "(option_line_height * i)");
    relatedDrawGml = relatedDrawGml.Replace(
        "draw_text_color(optionX, optionY + (option_line_height * i), o_text, c_white, c_white, c_white, c_white, 1);",
        "draw_text_transformed_color(optionX, optionY + (option_line_height * i), o_text, command_scale, command_scale, 0, c_white, c_white, c_white, c_white, 1);");
    relatedDrawGml = relatedDrawGml.Replace(
        "draw_text_color(optionX, optionY + (option_line_height * i), o_text, c_gray, c_gray, c_gray, c_gray, 1);",
        "draw_text_transformed_color(optionX, optionY + (option_line_height * i), o_text, command_scale, command_scale, 0, c_gray, c_gray, c_gray, c_gray, 1);");

    if (patchBattleText)
    {
        var relatedHeightExt = "if (string_height_ext(str, string_height(global.text_font), 220) == string_height(global.text_font))";
        if (relatedDrawGml.Contains(relatedHeightExt))
            relatedDrawGml = relatedDrawGml.Replace(relatedHeightExt,
                "var battle_line_height = string_height(\"Ag\");\n    if (string_width(str) <= 220)");
        else if (!relatedDrawGml.Contains("if (string_width(str) <= 220)"))
            throw new Exception(label + ": battle textbox height test not found");
        relatedDrawGml = relatedDrawGml.Replace(
            "draw_text_ext_color(room_width * 0.5, 8, str, string_height(global.text_font), 220, c_white, c_white, c_white, c_white, 1);",
            "draw_text_ext_color(room_width * 0.5, 8, str, battle_line_height, 220, c_white, c_white, c_white, c_white, 1);");
    }
    imports.QueueReplace(relatedDraw, relatedDrawGml);
}

// Restore the game's 212x40 textbox render target as a clip region, but keep
// it alive for the lifetime of the textbox. The original frees/recreates this
// surface every Draw; the previous Vita direct-draw optimization removed the
// clip entirely and let long translated text escape the box.
var textboxDraw = Data.Code.ByName("gml_Object_obj_textbox_Draw_0");
if (textboxDraw == null) throw new Exception("Missing obj_textbox Draw");
var drawGml = GetDecompiledText(textboxDraw).Replace("\r\n", "\n");
var direct = "draw_set_font(font);\ndraw_set_halign(fa_left);\ndraw_set_valign(fa_top);\ndraw_text_ext_color(text_x, text_y + initial_text_y, substr, text_height, 1000, c, c, c, c, 1);";
var clipped = @"if (!surface_exists(text_surf))
{
    text_surf = surface_create(212, 40);
}
if (surface_exists(text_surf))
{
    surface_set_target(text_surf);
    draw_clear_alpha(c_black, 0);
    draw_set_font(font);
    draw_set_halign(fa_left);
    draw_set_valign(fa_top);
    draw_text_ext_color(2, initial_text_y + 4, substr, text_height, 1000, c, c, c, c, 1);
    surface_reset_target();
    draw_surface(text_surf, text_x - 2, text_y - 4);
}";
if (drawGml.Contains(direct))
{
    drawGml = drawGml.Replace(direct, clipped);
}
else if (drawGml.Contains("surface_create(212, 40)"))
{
    var freeBlock = "    if (surface_exists(text_surf))\n    {\n        surface_free(text_surf);\n    }\n";
    drawGml = drawGml.Replace(freeBlock, "");
}
else throw new Exception("Textbox direct/surface block not found");
imports.QueueReplace(textboxDraw, drawGml);

var textboxDestroy = Data.Code.ByName("gml_Object_obj_textbox_Destroy_0");
if (textboxDestroy == null) throw new Exception("Missing obj_textbox Destroy");
var destroyGml = GetDecompiledText(textboxDestroy).Replace("\r\n", "\n");
if (!destroyGml.Contains("surface_free(text_surf)"))
{
    destroyGml += "\nif (surface_exists(text_surf)) { surface_free(text_surf); text_surf = -1; }\n";
    imports.QueueReplace(textboxDestroy, destroyGml);
}

// Every final-fight enemy projectile asks for a precise blade collision every
// frame. When Add is not actively swinging there is no blade instance, so the
// expensive spatial-grid/precise collision query is provably unnecessary.
// Preserve the exact collision path whenever a blade exists.
string[] bulletObjects = {
    "001","002","003","004","005","006","007","007_ex","008","009","010","011","012"
};
foreach (var suffix in bulletObjects)
{
    var codeName = "gml_Object_obj_ex_enemybullet_" + suffix + "_Step_0";
    var code = Data.Code.ByName(codeName);
    if (code == null) throw new Exception("Missing " + codeName);
    var gml = GetDecompiledText(code).Replace("\r\n", "\n");
    var before = "if (place_meeting(x, y, obj_ex_addblade))";
    var after = "if (instance_exists(obj_ex_addblade) && place_meeting(x, y, obj_ex_addblade))";
    if (gml.Contains(before)) gml = gml.Replace(before, after);
    else if (!gml.Contains(after)) throw new Exception("Blade collision not found in " + codeName);
    imports.QueueReplace(code, gml);
}

imports.Import();
ScriptMessage("Void Stranger Vita v1.3 textbox clipping + final fight collision optimization applied.");
