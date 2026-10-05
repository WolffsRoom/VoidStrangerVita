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
    if (after == before) throw new Exception("No change in " + name);
    imports.QueueReplace(c, after);
}

// Keep the original Graphics grid and all original row indices intact.
// Only hide Scaling from the Vita UI; this prevents save/load/callback drift.
ReplaceCode("gml_Object_obj_menu_Create_0", t => {
    if (!t.Contains("[\"FLASHING FX\"")) throw new Exception("FLASHING FX row missing");
    t = t.Replace("[\"FLASHING FX\"", "[\"FPS\"");
    var steamDeck = "if (global.is_steam_deck)\n{\n    ds_grid_set(obj_menu.ds_menu_graphics, 3, 0, 0);\n}\n";
    if (!t.Contains(steamDeck)) throw new Exception("Steam Deck scaling block missing");
    t = t.Replace(steamDeck, "ds_grid_set(obj_menu.ds_menu_graphics, 3, 0, 1);\nchange_fullscreen_scaling(1);\n");
    return t;
});

ReplaceCode("gml_GlobalScript_change_fullscreen_scaling", t => {
    var marker = "function change_fullscreen_scaling(arg0)\n{\n";
    if (!t.Contains(marker)) throw new Exception("scaling function header missing");
    return t.Replace(marker, marker + "    arg0 = 1;\n");
});

ReplaceCode("gml_Object_obj_menu_Step_0", t => {
    var h = "var ds_height = ds_grid_height(ds_);\n";
    if (!t.Contains(h)) throw new Exception("menu step ds_height missing");
    t = t.Replace(h, h + "if (page == 2 && menu_option[page] == 0)\n{\n    menu_option[page] = 1;\n}\n");
    var wrap = "if (menu_option[page] < 0)\n        {\n            menu_option[page] = ds_height - 1;\n        }\n";
    if (!t.Contains(wrap)) throw new Exception("menu navigation wrap missing");
    t = t.Replace(wrap, wrap + "        if (page == 2 && menu_option[page] == 0)\n        {\n            menu_option[page] = (ochange > 0) ? 1 : (ds_height - 1);\n        }\n");
    return t;
});

ReplaceCode("gml_Object_obj_menu_Draw_0", t => {
    var menuHeader = "if (!global.menu)\n{\n    exit;\n}\n";
    if (!t.Contains(menuHeader)) throw new Exception("menu draw header missing");
    t = t.Replace(menuHeader, menuHeader + "if (draw_memories)\n{\n    exit;\n}\n");

    var sy = "var start_y = (gheight / 2) - (((ds_height - 1) / 2) * y_buffer);";
    if (!t.Contains(sy)) throw new Exception("start_y missing");
    t = t.Replace(sy, "var visible_height = (page == 2) ? (ds_height - 1) : ds_height;\nvar start_y = (gheight / 2) - (((visible_height - 1) / 2) * y_buffer);");

    var bar = "var _bar_y = start_y + (menu_option[page] * y_buffer);";
    if (!t.Contains(bar)) throw new Exception("secret bar position missing");
    t = t.Replace(bar, "var _bar_row = (page == 2) ? (menu_option[page] - 1) : menu_option[page];\n    var _bar_y = start_y + (_bar_row * y_buffer);");

    var loop = "repeat (ds_height)\n{\n";
    int first = t.IndexOf(loop);
    if (first < 0) throw new Exception("first draw loop missing");
    var skip = loop + "    if (page == 2 && yy == 0)\n    {\n        yy++;\n        continue;\n    }\n";
    t = t.Substring(0, first) + skip + t.Substring(first + loop.Length);
    int second = t.IndexOf(loop, first + skip.Length);
    if (second < 0) throw new Exception("second draw loop missing");
    t = t.Substring(0, second) + skip + t.Substring(second + loop.Length);

    t = t.Replace("var lty = start_y + (yy * y_buffer) + _yoffset;", "var lrow = (page == 2) ? (yy - 1) : yy;\n    var lty = start_y + (lrow * y_buffer) + _yoffset;");
    t = t.Replace("var rty = start_y + (yy * y_buffer) + _yoffset;", "var rrow = (page == 2) ? (yy - 1) : yy;\n    var rty = start_y + (rrow * y_buffer) + _yoffset;");
    return t;
});

// The album owns its own backdrop. Use the game's black-screen sprite instead
// of relying on the primitive rectangle, and keep the pause options suppressed.
ReplaceCode("gml_Object_obj_memories_album_Draw_0", t => {
    var old = "draw_rectangle_color(0, 0, 224, 144, cb, cb, cb, cb, false);";
    if (!t.Contains(old)) throw new Exception("album backdrop rectangle missing");
    return t.Replace(old, "draw_sprite(spr_black_screen, 0, 0, 0);");
});

// Initial instruction screen: Vita/DualShock icon left, localized action right.
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
    if (!t.Contains(oldDraw)) throw new Exception("titlescreen PC/Vita draw block missing");
    return t.Replace(oldDraw, newDraw);
});

imports.Import();
Console.WriteLine("v1.17 UI patch applied: hidden Scaling with stable indices, FPS label, Memories backdrop, Vita prompts.");
