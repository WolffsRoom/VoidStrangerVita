using System;
using System.Text.RegularExpressions;
using UndertaleModLib;
using UndertaleModLib.Models;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Compiler;
EnsureDataLoaded();
CodeImportGroup imports = new(Data) { AutoCreateAssets = true };
void ReplaceCode(string name, Func<string,string> edit) {
    var c = Data.Code.ByName(name);
    if (c == null) throw new Exception("Missing code: " + name);
    var before = GetDecompiledText(c).Replace("\r\n", "\n");
    var after = edit(before);
    if (after == before) { Console.WriteLine("No change: " + name); return; }
    imports.QueueReplace(c, after);
}
string ShiftGraphicsRows(string t) {
    // Row 0 (Scaling) is removed on Vita. It remains fixed internally to FIT.
    t = Regex.Replace(t, @"ds_grid_set\((?:obj_menu\.)?ds_menu_graphics, 3, 0, [^;]+\);\n?", "");
    t = t.Replace("ds_grid_get(obj_menu.ds_menu_graphics, 3, 0)", "1");
    t = t.Replace("ds_grid_get(ds_menu_graphics, 3, 0)", "1");
    // Shift every former semantic row 1..8 down by one. Right-hand legacy
    // _grid_substitute indices are intentionally untouched.
    t = Regex.Replace(t, @"((?:obj_menu\.)?ds_menu_graphics, 3, )([1-8])", m => {
        int n = Int32.Parse(m.Groups[2].Value) - 1;
        return m.Groups[1].Value + n.ToString();
    });
    return t;
}

// Remove Scaling from the actual page and expose FPS directly.
ReplaceCode("gml_Object_obj_menu_Create_0", t => {
    t = ShiftGraphicsRows(t);
    t = Regex.Replace(t,
        @"ds_menu_graphics = create_menu_page\([^\n]+\);",
        "ds_menu_graphics = create_menu_page([\"BORDER COLOR\", UnknownEnum.Value_3, \"change_border_fill\", 0, [\"BLACK\", \"DARK\"]], [\"VSYNC\", UnknownEnum.Value_4, \"toggle_vsync\", 0, [\"ON\", \"OFF\"]], [\"FPS\", UnknownEnum.Value_3, \"change_flicker\", 0, [\"60 FPS\", \"30 FPS\", \"REDUCED\"]], [scrScript(24), UnknownEnum.Value_4, \"toggle_timer\", 1, [\"ON\", \"OFF\"]], [scrScript(25), UnknownEnum.Value_4, \"toggle_counter\", 1, [\"ON\", \"OFF\"]], [scrScript(72), UnknownEnum.Value_3, \"change_palette\", 0, [\"GRAY\", \"R***\", \"O***\", \"Y***\", \"G***\", \"B***\", \"I***\", \"V***\", \"MELLOW\"]], [\"BRIGHTNESS\", UnknownEnum.Value_3, \"change_resolution\", 9, [\"10%\", \"20%\", \"30%\", \"40%\", \"50%\", \"60%\", \"70%\", \"80%\", \"90%\", \"100%\"]], [\"STRETCH SCREEN\", UnknownEnum.Value_3, \"change_window_mode\", 0, [\"OFF\", \"ON\"]], [scrScript(7), UnknownEnum.Value_1, UnknownEnum.Value_1]);",
        RegexOptions.None);
    if (!t.Contains("[\"FPS\"")) throw new Exception("Graphics page replacement failed");
    // Scaling is a PC-only concept in this port. Aspect-correct FIT is the Vita
    // baseline and Stretch Screen is the only aspect override exposed.
    var marker = "ds_menu_controls = create_menu_page(";
    var idx = t.IndexOf(marker);
    if (idx < 0) throw new Exception("controls marker missing");
    t = t.Insert(idx, "change_fullscreen_scaling(1);\n");
    return t;
});

// All scripts that reference the old graphics row numbers must follow the new
// page. Keep the INI key names stable so v1.15 settings migrate by semantics.
foreach (var name in new[]{
    "gml_GlobalScript_scr_savesettings",
    "gml_GlobalScript_scr_loadsettings",
    "gml_GlobalScript_change_flicker",
    "gml_GlobalScript_language_menu",
    "gml_Object_obj_help_display_oneoff_Step_0",
    "gml_Object_obj_game_KeyPress_115",
    "gml_Object_obj_game_KeyPress_113",
    "gml_Object_obj_game_KeyPress_114",
    "gml_Object_obj_menu_Alarm_2",
    "gml_Object_obj_menu_Other_18"
}) {
    ReplaceCode(name, t => {
        t = ShiftGraphicsRows(t);
        t = t.Replace("\"Schema\", 12", "\"Schema\", 13");
        t = t.Replace("schema=12", "schema=13");
        if (name == "gml_GlobalScript_scr_loadsettings") {
            // Ignore all persisted Scaling values; final Vita geometry starts in FIT.
            t = Regex.Replace(t, @"change_fullscreen_scaling\([^;]+\);", "change_fullscreen_scaling(1);");
        }
        return t;
    });
}

// The Graphics page itself must not force fullscreen, otherwise Stretch ON/OFF
// looks identical while the user is changing it.
ReplaceCode("gml_Object_obj_game_Draw_77", t => {
    t = t.Replace("var vita_graphics_menu_fullscreen = instance_exists(obj_menu) && obj_menu.page == 2;\nif (vita_graphics_menu_fullscreen || (variable_global_exists(\"vita_stretch_screen\") && global.vita_stretch_screen == 1))",
                  "if (variable_global_exists(\"vita_stretch_screen\") && global.vita_stretch_screen == 1)");
    return t;
});

// While the dedicated album object is active it owns the whole 224x144 frame.
// Do not leave the Settings/menu UI behind it.
ReplaceCode("gml_Object_obj_menu_Draw_0", t => {
    var marker = "if (!global.menu)\n{\n    exit;\n}\n";
    if (!t.Contains(marker)) throw new Exception("menu draw header missing");
    return t.Replace(marker, marker + "if (draw_memories)\n{\n    exit;\n}\n");
});

// Initial instruction screen: show Vita/DualShock icons on the left and the
// localized action name on the right. Remove the parallel PC-key column.
ReplaceCode("gml_Object_obj_titlescreen_Draw_0", t => {
    t = Regex.Replace(t,
        @"    var string_val;\n    switch \(current_val\)\n    \{.*?\n    \}\n    var button_number;",
        @"    var string_val;
    switch (i)
    {
        case 0: string_val = scrScript(26); break;
        case 1: string_val = scrScript(27); break;
        case 2: string_val = scrScript(29); break;
        case 3: string_val = scrScript(28); break;
        case 4: string_val = scrScript(30); break;
        case 5: string_val = scrScript(38); break;
    }
    var button_number;",
        RegexOptions.Singleline);
    var oldDraw = @"    draw_sprite_ext(btn_index, i, (btn_x - 24) + xi[i], btn_y + (i * 16) + 8 + yi[i], 1, 1, 0, c_white, prompt_a);
    draw_set_halign(fa_left);
    var bb = 0;
    draw_text_color(btn_x - 1, btn_y + (i * 16), string_val, bb, bb, bb, bb, btn_a);
    draw_text_color(btn_x, btn_y + (i * 16) + 1, string_val, bb, bb, bb, bb, btn_a);
    draw_text_color(btn_x, (btn_y + (i * 16)) - 1, string_val, bb, bb, bb, bb, btn_a);
    draw_text_color(btn_x + 1, btn_y + (i * 16), string_val, bb, bb, bb, bb, btn_a);
    draw_text_color(btn_x, btn_y + (i * 16), string_val, bc, bc, bc, bc, btn_a);
    draw_sprite_ext(global.current_controller_sprite, button_number, btn_x, btn_y + (i * 16), 1, 1, 0, c_white, ctrl_a);";
    var newDraw = @"    draw_set_halign(fa_left);
    var bb = 0;
    var vita_prompt_alpha = max(ctrl_a, btn_a);
    var vita_prompt_x = (btn_x - 24) + xi[i];
    var vita_prompt_y = btn_y + (i * 16) + yi[i];
    draw_sprite_ext(global.current_controller_sprite, button_number, vita_prompt_x, vita_prompt_y, 1, 1, 0, c_white, vita_prompt_alpha);
    draw_text_color(btn_x - 1, btn_y + (i * 16), string_val, bb, bb, bb, bb, vita_prompt_alpha);
    draw_text_color(btn_x, btn_y + (i * 16) + 1, string_val, bb, bb, bb, bb, vita_prompt_alpha);
    draw_text_color(btn_x, (btn_y + (i * 16)) - 1, string_val, bb, bb, bb, bb, vita_prompt_alpha);
    draw_text_color(btn_x + 1, btn_y + (i * 16), string_val, bb, bb, bb, bb, vita_prompt_alpha);
    draw_text_color(btn_x, btn_y + (i * 16), string_val, bc, bc, bc, bc, vita_prompt_alpha);";
    if (!t.Contains(oldDraw)) throw new Exception("titlescreen draw block missing");
    t = t.Replace(oldDraw, newDraw);
    return t;
});

imports.Import();
Console.WriteLine("v1.16 UI patch applied: Graphics rows, Memories isolation, initial Vita prompts.");
