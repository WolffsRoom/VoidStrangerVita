using UndertaleModLib.Compiler;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Models;

EnsureDataLoaded();
CodeImportGroup imports = new(Data) { AutoCreateAssets = true };

UndertaleCode loadSettings = Data.Code.ByName("gml_GlobalScript_scr_loadsettings");
if (loadSettings == null) throw new Exception("Missing scr_loadsettings");
string gml = GetDecompiledText(loadSettings).Replace("\r\n", "\n");
int closing = gml.LastIndexOf('}');
if (closing < 0) throw new Exception("scr_loadsettings closing brace not found");

// Make the patch idempotent. Earlier iterative builds accumulated this block
// several times because their data.win was used again as input.
int previousExitButton = gml.IndexOf("global.exit_button = 27;");
if (previousExitButton >= 0)
{
    int previousBlock = gml.LastIndexOf("if (instance_exists(obj_menu))", previousExitButton);
    if (previousBlock >= 0)
    {
        gml = gml.Substring(0, previousBlock) + gml.Substring(closing);
        closing = gml.LastIndexOf('}');
    }
}

string vitaDefaults = @"
    // Vita port: Steam settings files from older builds may contain muted
    // audio or remapped PC keys. Preserve progress/options, but normalize the
    // fixed console audio and control contract after every settings load.
    if (instance_exists(obj_menu))
    {
        ds_grid_set(obj_menu.ds_menu_audio, 3, 0, 1);
        ds_grid_set(obj_menu.ds_menu_audio, 3, 1, 1);
        ds_grid_set(obj_menu.ds_menu_audio, 3, 2, 1);
        ds_grid_set(obj_menu.ds_menu_controls, 3, 0, 38);
        ds_grid_set(obj_menu.ds_menu_controls, 3, 1, 37);
        ds_grid_set(obj_menu.ds_menu_controls, 3, 2, 39);
        ds_grid_set(obj_menu.ds_menu_controls, 3, 3, 40);
        ds_grid_set(obj_menu.ds_menu_controls, 3, 4, 90);
        ds_grid_set(obj_menu.ds_menu_controls, 3, 5, 13);
    }
    audio_master_gain(1);
    scr_audio_group_set_gain_vs(1, 1, 0);
    scr_audio_group_set_gain_vs(2, 1, 0);
    global.key_up = 38;
    global.key_left = 37;
    global.key_right = 39;
    global.key_down = 40;
    global.key_action = 90;
    global.key_enter = 13;
    global.exit_button = 27;
    if (is_undefined(global.language) || global.language < 0 || global.language >= array_length(global.script_array))
        global.language = 0;
";

gml = gml.Insert(closing, vitaDefaults);
imports.QueueReplace(loadSettings, gml);

UndertaleCode scrScript = Data.Code.ByName("gml_GlobalScript_scrScript");
if (scrScript == null) scrScript = Data.Code.ByName("gml_Script_scrScript");
if (scrScript == null) throw new Exception("Missing scrScript");
imports.QueueReplace(scrScript, @"
function scrScript()
{
    if (!variable_global_exists(""script_array"") || !is_array(global.script_array) || array_length(global.script_array) <= 0)
        return """";

    if (!variable_global_exists(""language"") || is_undefined(global.language) || global.language < 0 || global.language >= array_length(global.script_array))
        global.language = 0;

    var vita_lang = global.language;
    var vita_code = global.script_array[vita_lang][0];

    if (is_undefined(global.script_array[vita_lang][1]) || !is_array(global.script_array[vita_lang][1]))
        global.script_array[vita_lang][1] = txt_to_array(vita_code, ""included"");
    if (is_undefined(global.script_array[vita_lang][2]) || !is_array(global.script_array[vita_lang][2]))
        global.script_array[vita_lang][2] = txt_to_array(vita_code, ""extracted"");

    // EN/FI are also present in the original packed CSV. Keep it as a fallback
    // only when the external included.txt is unavailable.
    if (array_length(global.script_array[vita_lang][1]) == 0 && vita_lang <= 1)
    {
        var vita_grid = csv_to_grid(""voidstranger_data.csv"");
        global.script_array[0][1] = vita_grid[0];
        global.script_array[1][1] = vita_grid[1];
    }

    var vita_id = argument0;
    var vita_value = undefined;
    var vita_index = 0;
    var vita_source = undefined;

    if (vita_id > 0)
    {
        vita_index = vita_id - 1;
        vita_source = global.script_array[vita_lang][1];
    }
    else if (vita_id < 0)
    {
        vita_index = (-vita_id) - 1;
        vita_source = global.script_array[vita_lang][2];
    }
    else
    {
        return """";
    }

    if (is_array(vita_source) && vita_index >= 0 && vita_index < array_length(vita_source))
        vita_value = vita_source[vita_index];

    var vita_missing = (!is_string(vita_value) || vita_value == """" || vita_value == ""undefined"");

    // EN and FI have a packed original column. Sparse external language files
    // intentionally leave some entries empty, so an empty string means
    // "fall back", not "display an empty dialogue page".
    if (vita_missing && vita_id > 0 && vita_lang <= 1)
    {
        if (!variable_global_exists(""vita_original_script_grid"") ||
            !is_array(global.vita_original_script_grid) ||
            array_length(global.vita_original_script_grid) < 2)
            global.vita_original_script_grid = csv_to_grid(""voidstranger_data.csv"");

        var vita_packed_source = global.vita_original_script_grid[vita_lang];
        if (is_array(vita_packed_source) && vita_index >= 0 && vita_index < array_length(vita_packed_source))
            vita_value = vita_packed_source[vita_index];
        vita_missing = (!is_string(vita_value) || vita_value == """" || vita_value == ""undefined"");
    }

    // External translations are sparse. Fall back to English external text
    // first, then to the packed original English CSV below.
    if (vita_missing && vita_lang != 0)
    {
        if (is_undefined(global.script_array[0][1]) || !is_array(global.script_array[0][1]))
            global.script_array[0][1] = txt_to_array(global.script_array[0][0], ""included"");
        if (is_undefined(global.script_array[0][2]) || !is_array(global.script_array[0][2]))
            global.script_array[0][2] = txt_to_array(global.script_array[0][0], ""extracted"");

        if (vita_id > 0)
            vita_source = global.script_array[0][1];
        else
            vita_source = global.script_array[0][2];

        if (is_array(vita_source) && vita_index >= 0 && vita_index < array_length(vita_source))
            vita_value = vita_source[vita_index];
        vita_missing = (!is_string(vita_value) || vita_value == """" || vita_value == ""undefined"");
    }

    if (vita_missing && vita_id > 0)
    {
        if (!variable_global_exists(""vita_original_script_grid"") ||
            !is_array(global.vita_original_script_grid) ||
            array_length(global.vita_original_script_grid) < 2)
            global.vita_original_script_grid = csv_to_grid(""voidstranger_data.csv"");

        var vita_english_source = global.vita_original_script_grid[0];
        if (is_array(vita_english_source) && vita_index >= 0 && vita_index < array_length(vita_english_source))
            vita_value = vita_english_source[vita_index];
        vita_missing = (!is_string(vita_value) || vita_value == """" || vita_value == ""undefined"");
    }

    if (vita_missing)
        return """";
    return vita_value;
}
");


// Vita bundle-relative language paths. Avoid mixing the absolute working_directory
// returned by the overlay filesystem with Windows-style separators.
UndertaleCode txtToArray = Data.Code.ByName("gml_GlobalScript_txt_to_array");
if (txtToArray == null) throw new Exception("Missing txt_to_array");
string txtArrayGml = GetDecompiledText(txtToArray).Replace("\r\n", "\n");
txtArrayGml = txtArrayGml.Replace("working_directory + \"Languages\\\\\" + arg0 + \"\\\\\" + arg1 + \".txt\"", "\"Languages/\" + arg0 + \"/\" + arg1 + \".txt\"");
imports.QueueReplace(txtToArray, txtArrayGml);

UndertaleCode changeLanguage = Data.Code.ByName("gml_GlobalScript_change_language");
if (changeLanguage == null) throw new Exception("Missing change_language");
string changeLanguageGml = GetDecompiledText(changeLanguage).Replace("\r\n", "\n");
changeLanguageGml = changeLanguageGml.Replace("working_directory + \"Languages\\\\\" + global.script_array[arg0][0] + \"\\\\config.ini\"", "\"Languages/\" + global.script_array[arg0][0] + \"/config.ini\"");
imports.QueueReplace(changeLanguage, changeLanguageGml);

UndertaleCode loadFonts = Data.Code.ByName("gml_GlobalScript_load_fonts");
if (loadFonts == null) throw new Exception("Missing load_fonts");
string loadFontsGml = GetDecompiledText(loadFonts).Replace("\r\n", "\n");
loadFontsGml = loadFontsGml.Replace("working_directory + \"Languages\\\\\" + global.script_array[arg0][0] + \"\\\\*.ttf\"", "\"Languages/\" + global.script_array[arg0][0] + \"/*.ttf\"");
loadFontsGml = loadFontsGml.Replace("working_directory + \"Languages\\\\\" + global.script_array[arg0][0] + \"\\\\\" + _font + \".ttf\"", "\"Languages/\" + global.script_array[arg0][0] + \"/\" + _font + \".ttf\"");
imports.QueueReplace(loadFonts, loadFontsGml);

// Clean obsolete palette invalidation blocks left by older Vita patch revisions.
UndertaleCode changePalette = Data.Code.ByName("gml_GlobalScript_change_palette");
if (changePalette == null) throw new Exception("Missing change_palette");
string paletteGml = GetDecompiledText(changePalette).Replace("\r\n", "\n");
string obsoletePaletteBlock = "\n    if (instance_exists(obj_game))\n    {\n        obj_game.vita_palette_uniforms_ready = false;\n    }";
while (paletteGml.Contains(obsoletePaletteBlock))
    paletteGml = paletteGml.Replace(obsoletePaletteBlock, "");
imports.QueueReplace(changePalette, paletteGml);
UndertaleCode finalDraw = Data.Code.ByName("gml_Object_obj_game_Draw_77");
if (finalDraw == null) throw new Exception("Missing obj_game Draw 77");
imports.QueueReplace(finalDraw, @"
var sx = surface_get_width(application_surface);
var sy = surface_get_height(application_surface);
var xsceel = min(window_get_width() / sx, window_get_height() / sy);
if (global.fullscreen_scaling == 0)
{
    xsceel = floor(xsceel);
}
var ysceel = xsceel;
var xsceel_rotat = ysceel;
var ysceel_rotat = xsceel;
var ax = 0;
var ay = 0;
if (global.shaders_work)
{
    shader_set(shader_palette);
    var i = 0;
    repeat (4)
    {
        var unistr;
        switch (i)
        {
            case 0:
                unistr = ""cBl"";
                break;
            case 1:
                unistr = ""cG0"";
                break;
            case 2:
                unistr = ""cG1"";
                break;
            default:
                unistr = ""cWh"";
        }
        var uni = shader_get_uniform(shader_palette, unistr);
        shader_set_uniform_f(uni, global.palette_array[(i * 3) + 0], global.palette_array[(i * 3) + 1], global.palette_array[(i * 3) + 2]);
        i++;
    }
    if (global.fullscreen_border == 1)
    {
        draw_clear(make_color_rgb(global.palette_array[0] * 255, global.palette_array[1] * 255, global.palette_array[2] * 255));
    }
}
gpu_set_blendenable(false);
draw_surface_ext(application_surface, ax + ((window_get_width() - (sx * xsceel)) / 2), ay + ((window_get_height() - (sy * ysceel)) / 2), xsceel_rotat, ysceel_rotat, 0, c_white, 1);
gpu_set_blendenable(true);
if (global.shaders_work)
{
    shader_reset();
}
");
UndertaleCode objBegin = Data.Code.ByName("gml_Object_obj_begin_Other_2");
if (objBegin != null)
{
    string beginGml = GetDecompiledText(objBegin).Replace("\r\n", "\n");
    int languageStart = beginGml.IndexOf("var language_codes =");
    int languageEnd = beginGml.IndexOf("var language_names =", languageStart);
    if (languageStart < 0 || languageEnd < 0) throw new Exception("Language discovery block not found");
    beginGml = beginGml.Substring(0, languageStart) +
        "var language_codes = [\"EN\", \"FI\", \"ES\", \"FR\", \"IT\", \"PTBR\"];\n" +
        beginGml.Substring(languageEnd);
    beginGml = beginGml.Replace("working_directory + \"Languages\\\\names.csv\"", "\"Languages/names.csv\"");
    imports.QueueReplace(objBegin, beginGml);
}


UndertaleCode menuDraw = Data.Code.ByName("gml_Object_obj_menu_Draw_0");
if (menuDraw != null)
{
    string menuGml = GetDecompiledText(menuDraw).Replace("\r\n", "\n");
    string menuNeedle = "            var current_val = ds_grid_get(ds_grid, 3, yy);\n            var current_array = ds_grid_get(ds_grid, 4, yy);\n            var left_shift = \"<<\";";
    string menuReplacement = "            var current_val = ds_grid_get(ds_grid, 3, yy);\n            var current_array = ds_grid_get(ds_grid, 4, yy);\n            if (!is_real(current_val) || !is_array(current_array) || array_length(current_array) <= 0 || current_val < 0 || current_val >= array_length(current_array))\n            {\n                current_val = 0;\n                ds_grid_set(ds_grid, 3, yy, 0);\n            }\n            var left_shift = \"<<\";";
    if (menuGml.Contains(menuNeedle))
        menuGml = menuGml.Replace(menuNeedle, menuReplacement);
    else if (!menuGml.Contains("!is_real(current_val) || !is_array(current_array)"))
        throw new Exception("Menu option draw block not found");
    imports.QueueReplace(menuDraw, menuGml);
}

imports.Import();
ScriptMessage("Void Stranger Vita settings and safe language fallback applied.");

