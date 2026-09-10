using UndertaleModLib.Compiler;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Models;

EnsureDataLoaded();
CodeImportGroup imports = new(Data) { AutoCreateAssets = true };

string Norm(UndertaleCode c) => GetDecompiledText(c).Replace("\r\n", "\n");

// --- Vita graphics page ---
UndertaleCode menuCreate = Data.Code.ByName("gml_Object_obj_menu_Create_0");
if (menuCreate == null) throw new Exception("Missing obj_menu Create");
string create = Norm(menuCreate);
int gfxStart = create.IndexOf("ds_menu_graphics = create_menu_page(");
int gfxEnd = create.IndexOf(";\nif (global.is_steam_deck)", gfxStart);
if (gfxStart < 0 || gfxEnd < 0) throw new Exception("Graphics page declaration not found");
string gfxPage = @"ds_menu_graphics = create_menu_page(
    [""SCALING"", UnknownEnum.Value_3, ""change_fullscreen_scaling"", 0, [""INTEGER"", ""FIT""]],
    [""BORDER COLOR"", UnknownEnum.Value_3, ""change_border_fill"", 0, [""BLACK"", ""DARK""]],
    [""VSYNC"", UnknownEnum.Value_4, ""toggle_vsync"", 0, [""ON"", ""OFF""]],
    [""FLASHING FX"", UnknownEnum.Value_3, ""change_flicker"", 0, [""60 FPS"", ""30 FPS"", ""REDUCED""]],
    [scrScript(24), UnknownEnum.Value_4, ""toggle_timer"", 1, [""ON"", ""OFF""]],
    [scrScript(25), UnknownEnum.Value_4, ""toggle_counter"", 1, [""ON"", ""OFF""]],
    [scrScript(72), UnknownEnum.Value_3, ""change_palette"", 0, [""GRAY"", ""R***"", ""O***"", ""Y***"", ""G***"", ""B***"", ""I***"", ""V***"", ""MELLOW""]],
    [""BRIGHTNESS"", UnknownEnum.Value_3, ""change_resolution"", 9, [""10%"", ""20%"", ""30%"", ""40%"", ""50%"", ""60%"", ""70%"", ""80%"", ""90%"", ""100%""]],
    [""STRETCH SCREEN"", UnknownEnum.Value_4, ""change_window_mode"", 0, [""OFF"", ""ON""]],
    [scrScript(7), UnknownEnum.Value_1, UnknownEnum.Value_1])";
create = create.Substring(0, gfxStart) + gfxPage + create.Substring(gfxEnd);

// Remove PC-only fullscreen/resolution initialization and establish Vita defaults.
int initStart = create.IndexOf("if (global.fullscreenmode == 1)");
int initEnd = create.IndexOf("alarm[2] = 1;", initStart);
if (initStart < 0 || initEnd < 0) throw new Exception("Graphics init block not found");
string vitaInit = @"ds_grid_set(ds_menu_graphics, 3, 0, global.fullscreen_scaling);
ds_grid_set(ds_menu_graphics, 3, 1, global.fullscreen_border);
ds_grid_set(ds_menu_graphics, 3, 2, global.vsync ? 0 : 1);
ds_grid_set(ds_menu_graphics, 3, 6, global.palette);
ds_grid_set(ds_menu_graphics, 3, 7, 9);
ds_grid_set(ds_menu_graphics, 3, 8, 0);
global.vita_brightness = 1;
global.vita_stretch_screen = 0;
";
create = create.Substring(0, initStart) + vitaInit + create.Substring(initEnd);
imports.QueueReplace(menuCreate, create);

// Repurpose the two PC-only callbacks for Vita-only settings.
UndertaleCode changeResolution = Data.Code.ByName("gml_GlobalScript_change_resolution");
UndertaleCode changeWindowMode = Data.Code.ByName("gml_GlobalScript_change_window_mode");
if (changeResolution == null || changeWindowMode == null) throw new Exception("Missing PC graphics callbacks");
imports.QueueReplace(changeResolution, @"
function change_resolution(arg0)
{
    var idx = clamp(round(arg0), 0, 9);
    global.vita_brightness = (idx + 1) / 10;
}
");
imports.QueueReplace(changeWindowMode, @"
function change_window_mode(arg0)
{
    global.vita_stretch_screen = clamp(round(arg0), 0, 1);
}
");

// --- Menu input fixes ---
UndertaleCode menuStep = Data.Code.ByName("gml_Object_obj_menu_Step_0");
if (menuStep == null) throw new Exception("Missing obj_menu Step");
string step = Norm(menuStep);
string pcSkip = @"        if (page == 2)
        {
            repeat (2)
            {
                if ((!window_get_fullscreen() && (menu_option[page] == 2 || menu_option[page] == 3)) || (window_get_fullscreen() && menu_option[page] == 1))
                {
                    menu_option[page] += ochange;
                }
            }
        }
";
step = step.Replace(pcSkip, "");
string enterNeedle = "if (input_enter_p && transition != true)\n{";
if (!step.Contains(enterNeedle)) throw new Exception("Menu enter block not found");
string directActions = @"// Vita: resolve the two navigation actions directly. asset_get_index() is not
// reliable for these dynamically referenced script names on the console runner.
if (input_enter_p && transition != true && page == 0 && menu_option[page] == 0)
{
    audio_play_sound(snd_menu_2, 1, false);
    resume_game();
    input_enter_p = false;
}
if (input_enter_p && transition != true && page == 1 && menu_option[page] == (ds_height - 1))
{
    audio_play_sound(snd_menu_2, 1, false);
    page = 0;
    menu_art_x = 160;
    image_speed = 0;
    inputting = false;
    input_enter_p = false;
}
";
step = step.Replace(enterNeedle, directActions + enterNeedle);
imports.QueueReplace(menuStep, step);

// --- Graphics menu draw: remove PC hiding rules, 70% text, tighter spacing. ---
UndertaleCode menuDraw = Data.Code.ByName("gml_Object_obj_menu_Draw_0");
if (menuDraw == null) throw new Exception("Missing obj_menu Draw");
string draw = Norm(menuDraw);
draw = draw.Replace("var y_buffer = 16;", "var y_buffer = (page == 2) ? 12 : 16;");
string pcDrawHead = @"if (page == 2)
{
    if (!window_get_fullscreen())
    {
        start_y += ((y_buffer / 2) * 2);
    }
    else
    {
        start_y += (y_buffer / 2);
    }
}
";
draw = draw.Replace(pcDrawHead, "");
string pcDrawSkip = @"    if (page == 2)
    {
        if ((!window_get_fullscreen() && (yy == 2 || yy == 3)) || (window_get_fullscreen() && yy == 1))
        {
            _yoffset -= 16;
            yy++;
            continue;
        }
    }
";
draw = draw.Replace(pcDrawSkip, "");

string labelLine = "    draw_text_color(ltx + xoffset, lty, ds_grid_get(ds_grid, 0, yy), c, c, c, c, 1);";
string labelScaled = @"    if (page == 2)
        draw_text_transformed_color(ltx + xoffset, lty, ds_grid_get(ds_grid, 0, yy), 0.7, 0.7, 0, c, c, c, c, 1);
    else
        draw_text_color(ltx + xoffset, lty, ds_grid_get(ds_grid, 0, yy), c, c, c, c, 1);";
if (!draw.Contains(labelLine)) throw new Exception("Menu label draw not found");
draw = draw.Replace(labelLine, labelScaled);

string value3 = "            draw_text_color(rtx, rty, left_shift + _content + right_shift, c, c, c, c, 1);";
string value3Scaled = @"            if (page == 2)
                draw_text_transformed_color(rtx, rty, left_shift + _content + right_shift, 0.7, 0.7, 0, c, c, c, c, 1);
            else
                draw_text_color(rtx, rty, left_shift + _content + right_shift, c, c, c, c, 1);";
draw = draw.Replace(value3, value3Scaled);
string toggleA = "            draw_text_color(rtx, rty, scrScript(8), c1, c1, c1, c1, 1);";
string toggleB = "            draw_text_color(rtx + 32, rty, scrScript(9), c2, c2, c2, c2, 1);";
draw = draw.Replace(toggleA, @"            if (page == 2)
                draw_text_transformed_color(rtx, rty, scrScript(8), 0.7, 0.7, 0, c1, c1, c1, c1, 1);
            else
                draw_text_color(rtx, rty, scrScript(8), c1, c1, c1, c1, 1);");
draw = draw.Replace(toggleB, @"            if (page == 2)
                draw_text_transformed_color(rtx + 24, rty, scrScript(9), 0.7, 0.7, 0, c2, c2, c2, c2, 1);
            else
                draw_text_color(rtx + 32, rty, scrScript(9), c2, c2, c2, c2, 1);");

// Branding in settings screen.
int brandStart = draw.IndexOf("for (var i = 0; i < 9; i++)\n{\n    var ivn_char = string_char_at(\"vsI 3.1.0\"");
if (brandStart < 0) throw new Exception("vsI branding block not found");
int brandEnd = draw.IndexOf("}\ndraw_set_valign(fa_top);", brandStart);
if (brandEnd < 0) throw new Exception("vsI branding block end not found");
brandEnd += 2;
string branding = @"var vita_brand = ""BY WOLFFS ROOM"";
for (var i = 0; i < string_length(vita_brand); i++)
{
    var ivn_char = string_char_at(vita_brand, i + 1);
    draw_text_color(2 + (8 * i), -5, ivn_char, vn_c, vn_c, vn_c, vn_c, 1);
}";
draw = draw.Substring(0, brandStart) + branding + draw.Substring(brandEnd);
imports.QueueReplace(menuDraw, draw);

// --- Presentation: preserve aspect mode, add stretch and brightness overlay. ---
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
if (variable_global_exists(""vita_stretch_screen"") && global.vita_stretch_screen == 1)
{
    xsceel = window_get_width() / sx;
    ysceel = window_get_height() / sy;
}
var xsceel_rotat = xsceel;
var ysceel_rotat = ysceel;
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
            case 0: unistr = ""cBl""; break;
            case 1: unistr = ""cG0""; break;
            case 2: unistr = ""cG1""; break;
            default: unistr = ""cWh"";
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
if (variable_global_exists(""vita_brightness"") && global.vita_brightness < 1)
{
    draw_set_color(c_black);
    draw_set_alpha(1 - global.vita_brightness);
    draw_rectangle(0, 0, window_get_width(), window_get_height(), false);
    draw_set_alpha(1);
    draw_set_color(c_white);
}
");

// --- Settings persistence version 3 ---
UndertaleCode saveSettings = Data.Code.ByName("gml_GlobalScript_scr_savesettings");
if (saveSettings != null)
{
    string save = Norm(saveSettings).Replace("ini_write_string(\"Save1\", \"Version\", \"2\");", "ini_write_string(\"Save1\", \"Version\", \"3\");");
    imports.QueueReplace(saveSettings, save);
}

UndertaleCode loadSettings = Data.Code.ByName("gml_GlobalScript_scr_loadsettings");
if (loadSettings != null)
{
    string load = Norm(loadSettings);
    // Version 2 carried only timer/counter/palette. Map those to their new rows.
    string oldV2 = @"                else
                {
                    yy = 0;
                    repeat (ds_grid_height(obj_menu.ds_menu_graphics) - 1)
                    {
                        switch (yy)
                        {
                            case 2:
                                ds_grid_set(obj_menu.ds_menu_graphics, 3, 6, ds_grid_get(_grid_substitute, 0, yy));
                                break;
                            case 3:
                                ds_grid_set(obj_menu.ds_menu_graphics, 3, 7, ds_grid_get(_grid_substitute, 0, yy));
                                break;
                            case 4:
                                ds_grid_set(obj_menu.ds_menu_graphics, 3, 8, ds_grid_get(_grid_substitute, 0, yy));
                                break;
                        }
                        yy++;
                    }
                }";
    string newLoad = @"                else
                {
                    if (_settings_version >= 3)
                    {
                        ds_grid_set(obj_menu.ds_menu_graphics, 3, 1, ds_grid_get(_grid_substitute, 0, 0));
                        ds_grid_set(obj_menu.ds_menu_graphics, 3, 0, ds_grid_get(_grid_substitute, 0, 1));
                        ds_grid_set(obj_menu.ds_menu_graphics, 3, 6, ds_grid_get(_grid_substitute, 0, 2));
                        ds_grid_set(obj_menu.ds_menu_graphics, 3, 7, ds_grid_get(_grid_substitute, 0, 3));
                        ds_grid_set(obj_menu.ds_menu_graphics, 3, 8, ds_grid_get(_grid_substitute, 0, 4));
                        ds_grid_set(obj_menu.ds_menu_graphics, 3, 2, ds_grid_get(_grid_substitute, 0, 5));
                        ds_grid_set(obj_menu.ds_menu_graphics, 3, 3, ds_grid_get(_grid_substitute, 0, 6));
                        ds_grid_set(obj_menu.ds_menu_graphics, 3, 4, ds_grid_get(_grid_substitute, 0, 7));
                        ds_grid_set(obj_menu.ds_menu_graphics, 3, 5, ds_grid_get(_grid_substitute, 0, 8));
                    }
                    else
                    {
                        ds_grid_set(obj_menu.ds_menu_graphics, 3, 4, ds_grid_get(_grid_substitute, 0, 2));
                        ds_grid_set(obj_menu.ds_menu_graphics, 3, 5, ds_grid_get(_grid_substitute, 0, 3));
                        ds_grid_set(obj_menu.ds_menu_graphics, 3, 6, ds_grid_get(_grid_substitute, 0, 4));
                    }
                }";
    if (load.Contains(oldV2)) load = load.Replace(oldV2, newLoad);

    // Replace the first graphics apply switch with the Vita row layout.
    string applyStartNeedle = "                switch (_settings_version)\n                {\n                    case 1:\n                        change_window_mode";
    int a0 = load.IndexOf(applyStartNeedle);
    if (a0 >= 0)
    {
        int a1 = load.IndexOf("                toggle_memory", a0);
        if (a1 < 0) throw new Exception("Graphics apply end not found");
        string apply = @"                change_fullscreen_scaling(ds_grid_get(obj_menu.ds_menu_graphics, 3, 0));
                change_border_fill(ds_grid_get(obj_menu.ds_menu_graphics, 3, 1));
                toggle_vsync(ds_grid_get(obj_menu.ds_menu_graphics, 3, 2));
                change_flicker(ds_grid_get(obj_menu.ds_menu_graphics, 3, 3));
                toggle_timer(ds_grid_get(obj_menu.ds_menu_graphics, 3, 4));
                toggle_counter(ds_grid_get(obj_menu.ds_menu_graphics, 3, 5));
                change_palette(ds_grid_get(obj_menu.ds_menu_graphics, 3, 6));
                change_resolution(ds_grid_get(obj_menu.ds_menu_graphics, 3, 7));
                change_window_mode(ds_grid_get(obj_menu.ds_menu_graphics, 3, 8));
";
        load = load.Substring(0, a0) + apply + load.Substring(a1);
    }
    imports.QueueReplace(loadSettings, load);
}

imports.Import();
ScriptMessage("Void Stranger Vita menu v2 applied.");
