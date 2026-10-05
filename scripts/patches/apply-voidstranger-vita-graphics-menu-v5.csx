using UndertaleModLib.Compiler;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Models;
EnsureDataLoaded();
CodeImportGroup imports = new(Data) { AutoCreateAssets = true };
string Norm(UndertaleCode c) => GetDecompiledText(c).Replace("\r\n", "\n");

string ApplyGraphicsNow = @"
                if (page == 2)
                {
                    var vita_gfx_row = array_get(menu_option, page);
                    var vita_gfx_value = ds_grid_get(ds_, 3, vita_gfx_row);
                    switch (vita_gfx_row)
                    {
                        case 0: change_fullscreen_scaling(vita_gfx_value); break;
                        case 1: change_border_fill(vita_gfx_value); break;
                        case 2: toggle_vsync(vita_gfx_value); break;
                        case 3: change_flicker(vita_gfx_value); break;
                        case 4: toggle_timer(vita_gfx_value); break;
                        case 5: toggle_counter(vita_gfx_value); break;
                        case 6: change_palette(vita_gfx_value); break;
                        case 7: change_resolution(vita_gfx_value); break;
                        case 8: change_window_mode(vita_gfx_value); break;
                    }
                    scr_savesettings();
                }";

UndertaleCode stepCode = Data.Code.ByName("gml_Object_obj_menu_Step_0");
if (stepCode == null) throw new Exception("Missing obj_menu Step");
string step = Norm(stepCode);
string clampV3 = "                ds_grid_set(ds_, 3, array_get(menu_option, page), clamp(ds_grid_get(ds_, 3, array_get(menu_option, page)), 0, array_length(ds_grid_get(ds_, 4, array_get(menu_option, page))) - 1));";
int first = step.IndexOf(clampV3);
if (first < 0) throw new Exception("Value_3 clamp not found");
if (!step.Substring(first, Math.Min(1600, step.Length-first)).Contains("vita_gfx_row"))
    step = step.Insert(first + clampV3.Length, ApplyGraphicsNow);
string clampV4 = "                ds_grid_set(ds_, 3, array_get(menu_option, page), clamp(ds_grid_get(ds_, 3, array_get(menu_option, page)), 0, 1));";
int value4Case = step.IndexOf("case UnknownEnum.Value_4:", first);
if (value4Case < 0) throw new Exception("Value_4 case not found");
int v4pos = step.IndexOf(clampV4, value4Case);
if (v4pos < 0) throw new Exception("Value_4 clamp not found");
if (!step.Substring(v4pos, Math.Min(1600, step.Length-v4pos)).Contains("vita_gfx_row"))
    step = step.Insert(v4pos + clampV4.Length, ApplyGraphicsNow);

string resumeBlock = @"if (input_enter_p && transition != true && page == 0 && menu_option[page] == 0)
{
    audio_play_sound(snd_menu_2, 1, false);
    resume_game();
    input_enter_p = false;
}".Replace("\r\n", "\n");
string quitBlock = @"
if (input_enter_p && transition != true && page == 0 && menu_option[page] == (ds_height - 1))
{
    audio_play_sound(snd_menu_2, 1, false);
    exit_game();
    game_end();
    input_enter_p = false;
}".Replace("\r\n", "\n");
if (!step.Contains("exit_game();\n    game_end();")) {
    int rp = step.IndexOf(resumeBlock);
    if (rp < 0) throw new Exception("Resume block not found");
    step = step.Insert(rp + resumeBlock.Length, quitBlock);
}
imports.QueueReplace(stepCode, step);

UndertaleCode drawCode = Data.Code.ByName("gml_Object_obj_game_Draw_77");
if (drawCode == null) throw new Exception("Missing obj_game Draw_77");
string draw = Norm(drawCode);
string oldDraw = "draw_surface_ext(application_surface, ax + ((window_get_width() - (sx * xsceel)) / 2), ay + ((window_get_height() - (sy * ysceel)) / 2), xsceel_rotat, ysceel_rotat, 0, c_white, 1);";
string newDraw = @"var vita_brightness_value = 1;
if (variable_global_exists(""vita_brightness""))
    vita_brightness_value = clamp(global.vita_brightness, 0.1, 1);
var vita_brightness_byte = round(vita_brightness_value * 255);
var vita_surface_tint = make_color_rgb(vita_brightness_byte, vita_brightness_byte, vita_brightness_byte);
draw_surface_ext(application_surface, ax + ((window_get_width() - (sx * xsceel)) / 2), ay + ((window_get_height() - (sy * ysceel)) / 2), xsceel_rotat, ysceel_rotat, 0, vita_surface_tint, 1);";
if (draw.Contains(oldDraw)) draw = draw.Replace(oldDraw, newDraw);
else if (!draw.Contains("vita_surface_tint")) throw new Exception("Final draw call not found");
int overlayStart = draw.IndexOf("if (variable_global_exists(\"vita_brightness\") && global.vita_brightness < 1)");
if (overlayStart >= 0) {
    int pos = overlayStart;
    int depth = 0; bool started = false;
    for (; pos < draw.Length; ++pos) {
        if (draw[pos] == '{') { depth++; started = true; }
        else if (draw[pos] == '}') { depth--; if (started && depth == 0) { pos++; break; } }
    }
    draw = draw.Remove(overlayStart, pos-overlayStart).TrimEnd() + "\n";
}
imports.QueueReplace(drawCode, draw);

UndertaleCode endCode = Data.Code.ByName("gml_GlobalScript_end_game");
if (endCode == null) throw new Exception("Missing end_game");
imports.QueueReplace(endCode, @"function end_game()
{
    if (global.cc_state == 0)
    {
        exit_game();
        game_end();
    }
    else
    {
        resume_game();
        with (obj_cc_check)
        {
            check_state = 999;
        }
    }
}");

imports.Import();
ScriptMessage("Void Stranger Vita Graphics Menu v5 applied: live apply/save, brightness tint, direct quit.");
