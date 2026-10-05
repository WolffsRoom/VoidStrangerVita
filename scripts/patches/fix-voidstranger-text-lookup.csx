using UndertaleModLib.Compiler;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Models;

EnsureDataLoaded();
CodeImportGroup imports = new(Data) { AutoCreateAssets = true };

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

imports.Import();
ScriptMessage("Void Stranger text lookup fixed for Butterscotch/Vita.");
