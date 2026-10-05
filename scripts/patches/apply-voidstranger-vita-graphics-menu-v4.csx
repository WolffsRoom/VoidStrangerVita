using UndertaleModLib.Compiler;
using UndertaleModLib.Models;
EnsureDataLoaded();
CodeImportGroup imports = new(Data) { AutoCreateAssets = true };

UndertaleCode c0 = Data.Code.ByName("gml_Object_obj_menu_Create_0");
if (c0 == null) throw new Exception("Missing gml_Object_obj_menu_Create_0");
imports.QueueReplace(c0, @"font = global.text_font;
counter = 0;
transition = false;
image_speed = 0;
menu_art_x = 160;
menu_art_y = 4;
menu_h_decel = menu_art_x * 0.1;
alarm[4] = 4;
found_controller = 0;
draw_controller_info = 0;
control_info_state = 1;
controller_counter = 0;
controller_font = 12;
controller_count = 0;
controller_count2 = -1;
ctrl_x = 112;
ctrl_y = 72;
ctrl_r_x1 = 0;
ctrl_r_x2 = 224;
ctrl_r_y1 = 1;
ctrl_r_y2 = 0;
ctrl_scroll_x = 0;
ctrl_string = scrScript(45);
ctrl_scroll_add = string_width(ctrl_string);
ctrl_scroll_half = string_width(ctrl_string) * 0.5;
cr_c = 12632256;
ct_c = 16777215;
ct_c2 = 8421504;
ct_a = 0;
pageicon_count = 1;
pi_x = 1;
pi_y = 136;
ds_menu_main = create_menu_page([scrScript(12), UnknownEnum.Value_0, ""resume_game"", 821], [scrScript(84), UnknownEnum.Value_1, UnknownEnum.Value_6, 2834], [scrScript(82), UnknownEnum.Value_7, UnknownEnum.Value_9, 417], [scrScript(13), UnknownEnum.Value_1, UnknownEnum.Value_1, 823], [scrScript(14), UnknownEnum.Value_0, ""end_game"", 1402]);
ds_settings = create_menu_page([scrScript(15), UnknownEnum.Value_1, UnknownEnum.Value_2, 826], [scrScript(16), UnknownEnum.Value_1, UnknownEnum.Value_3, 827], [scrScript(17), UnknownEnum.Value_1, UnknownEnum.Value_7, 828], [scrScript(31), UnknownEnum.Value_1, UnknownEnum.Value_5, 831], [scrScript(7), UnknownEnum.Value_1, UnknownEnum.Value_0, 830]);
ds_menu_audio = create_menu_page([scrScript(19), UnknownEnum.Value_2, ""change_volume"", 1, [0, 1]], [scrScript(20), UnknownEnum.Value_2, ""change_volume"", 1, [0, 1]], [scrScript(21), UnknownEnum.Value_2, ""change_volume"", 1, [0, 1]], [scrScript(7), UnknownEnum.Value_1, UnknownEnum.Value_1]);
ds_menu_graphics = create_menu_page([""SCALING"", UnknownEnum.Value_3, ""change_fullscreen_scaling"", 0, [""INTEGER"", ""FIT""]], [""BORDER COLOR"", UnknownEnum.Value_3, ""change_border_fill"", 0, [""BLACK"", ""DARK""]], [""VSYNC"", UnknownEnum.Value_4, ""toggle_vsync"", 0, [""ON"", ""OFF""]], [""FLASHING FX"", UnknownEnum.Value_3, ""change_flicker"", 0, [""60 FPS"", ""30 FPS"", ""REDUCED""]], [scrScript(24), UnknownEnum.Value_4, ""toggle_timer"", 1, [""ON"", ""OFF""]], [scrScript(25), UnknownEnum.Value_4, ""toggle_counter"", 1, [""ON"", ""OFF""]], [scrScript(72), UnknownEnum.Value_3, ""change_palette"", 0, [""GRAY"", ""R***"", ""O***"", ""Y***"", ""G***"", ""B***"", ""I***"", ""V***"", ""MELLOW""]], [""BRIGHTNESS"", UnknownEnum.Value_3, ""change_resolution"", 9, [""10%"", ""20%"", ""30%"", ""40%"", ""50%"", ""60%"", ""70%"", ""80%"", ""90%"", ""100%""]], [""STRETCH SCREEN"", UnknownEnum.Value_3, ""change_window_mode"", 0, [""OFF"", ""ON""]], [scrScript(7), UnknownEnum.Value_1, UnknownEnum.Value_1]);
if (global.is_steam_deck)
{
    ds_grid_set(obj_menu.ds_menu_graphics, 3, 0, 0);
}
ds_menu_controls = create_menu_page([scrScript(26), UnknownEnum.Value_5, ""key_up"", 38], [scrScript(27), UnknownEnum.Value_5, ""key_left"", 37], [scrScript(28), UnknownEnum.Value_5, ""key_right"", 39], [scrScript(29), UnknownEnum.Value_5, ""key_down"", 40], [scrScript(30), UnknownEnum.Value_5, ""key_action"", 90], [scrScript(38), UnknownEnum.Value_5, ""key_enter"", 13], [scrScript(89), UnknownEnum.Value_3, ""change_movement"", 0, [""TAP"", ""HOLD *"", ""HOLD **"", ""HOLD ***"", ""SecretHOLD ****!!""]], [scrScript(7), UnknownEnum.Value_1, UnknownEnum.Value_7]);
var _lang_arr = [];
for (var _i = 0; _i < array_length(global.script_array); _i++)
{
    array_push(_lang_arr, global.script_array[_i][3][_i]);
}
ds_menu_language = create_menu_page([scrScript(31), UnknownEnum.Value_3, ""change_language"", 0, _lang_arr], [scrScript(7), UnknownEnum.Value_1, UnknownEnum.Value_1]);
ds_menu_equipment = create_menu_page([""?????"", UnknownEnum.Value_6, ""toggle_memory"", 0, [""ON"", ""OFF""]], [""?????"", UnknownEnum.Value_6, ""toggle_wings"", 0, [""ON"", ""OFF""]], [""?????"", UnknownEnum.Value_6, ""toggle_sword"", 0, [""ON"", ""OFF""]], [scrScript(7), UnknownEnum.Value_1, UnknownEnum.Value_0]);
ds_menu_control_type = create_menu_page([scrScript(7485), UnknownEnum.Value_1, UnknownEnum.Value_4, 2429], [scrScript(7486), UnknownEnum.Value_1, UnknownEnum.Value_8, 2604], [scrScript(7), UnknownEnum.Value_1, UnknownEnum.Value_1, 830]);
ds_menu_controller = create_menu_page([scrScript(26), UnknownEnum.Value_8, ""ctrl_up"", 32781], [scrScript(27), UnknownEnum.Value_8, ""ctrl_left"", 32783], [scrScript(28), UnknownEnum.Value_8, ""ctrl_right"", 32784], [scrScript(29), UnknownEnum.Value_8, ""ctrl_down"", 32782], [scrScript(30), UnknownEnum.Value_8, ""ctrl_action"", 32769], [scrScript(38), UnknownEnum.Value_8, ""ctrl_enter"", 32778], [scrScript(89), UnknownEnum.Value_3, ""change_movement"", 0, [""TAP"", ""HOLD *"", ""HOLD **"", ""HOLD ***"", ""SecretHOLD ****!!""]], [scrScript(7), UnknownEnum.Value_1, UnknownEnum.Value_7]);
ds_menu_system = create_menu_page([""REST SHUTDOWN"", UnknownEnum.Value_4, ""change_restshut_behavior"", 0, [""DEFAULT"", ""FORCE STAY ON""]], [scrScript(7), UnknownEnum.Value_1, UnknownEnum.Value_7]);
page = 0;
menu_pages = [ds_menu_main, ds_settings, ds_menu_graphics, ds_menu_audio, ds_menu_controls, ds_menu_language, ds_menu_equipment, ds_menu_control_type, ds_menu_controller];
var i = 0;
var array_len = array_length(menu_pages);
repeat (array_len)
{
    menu_option[i] = 0;
    i++;
}
input_up_p = scr_input_check_pressed(2);
input_down_p = scr_input_check_pressed(3);
input_enter_p = scr_input_check_pressed(4);
cb = -4;
inputting = false;
secret_timer = 0;
ds_grid_set(ds_menu_graphics, 3, 0, clamp(round(global.fullscreen_scaling), 0, 1));
ds_grid_set(ds_menu_graphics, 3, 1, clamp(round(global.fullscreen_border), 0, 1));
ds_grid_set(ds_menu_graphics, 3, 2, global.vsync ? 0 : 1);
if (!variable_global_exists(""s_g_fli"")) global.s_g_fli = 0;
if (!variable_global_exists(""s_g_tim"")) global.s_g_tim = 1;
if (!variable_global_exists(""s_g_cou"")) global.s_g_cou = 1;
if (!variable_global_exists(""vita_brightness"")) global.vita_brightness = 1;
if (!variable_global_exists(""vita_stretch_screen"")) global.vita_stretch_screen = 0;
ds_grid_set(ds_menu_graphics, 3, 3, clamp(round(global.s_g_fli), 0, 2));
ds_grid_set(ds_menu_graphics, 3, 4, clamp(round(global.s_g_tim), 0, 1));
ds_grid_set(ds_menu_graphics, 3, 5, clamp(round(global.s_g_cou), 0, 1));
ds_grid_set(ds_menu_graphics, 3, 6, clamp(round(global.palette), 0, 8));
ds_grid_set(ds_menu_graphics, 3, 7, clamp(round(global.vita_brightness * 10) - 1, 0, 9));
ds_grid_set(ds_menu_graphics, 3, 8, clamp(round(global.vita_stretch_screen), 0, 1));
alarm[2] = 1;
puumerkki_index = 1897;
pmframe_index = 1849;
pmframe_speed = 0;
pm_x = 216;
pm_y = 136;
puumerkki_grid = ds_grid_create(6, 6);
puumerkki = 0;
draw_puumerkki = false;
draw_memories = false;
draw_gor_eyecatch = false;
gor_appears = false;
gor_ec_index = 2670;
gor_ec_speed = 0;
gor_x = 0;
gor_y = 180;
gor_counter = 0;
gor_string = scrScript(2661);
gor_char_count = 0;
gor_char_counter = 0;
gor_string_x[0] = 112;
gor_string_y[0] = 0;
var istring_length = string_length(gor_string);
for (var i2 = 0; i2 < istring_length; i2++)
{
    var iran_y = irandom_range(-8, 8);
    gor_string_y[i2] = 96 + iran_y;
    gor_string_x[i2] = ((112 + (12 * i2)) - (istring_length * 6)) + 6;
}
slider_icon_speed = 0;
draw_add_info = 0;
add_info_state = 1;
add_info_page = 0;
add_x = 112;
add_y = 104;
add_r_x1 = 0;
add_r_x2 = 224;
add_r_y1 = 1;
add_r_y2 = 0;
add_scroll_x = 0;
add_string = scrScript(39);
add_scroll_add = string_width(add_string);
add_scroll_half = string_width(add_string) * 0.5;
a_c = 12632256;
a_c2 = 8421504;
a_a = 0;
debug_glow = 0;

function got_any_burdens()
{
    with (obj_inventory)
    {
        if (ds_grid_get(ds_equipment, 0, 2) == 3 || ds_grid_get(ds_equipment, 0, 1) == 2 || ds_grid_get(ds_equipment, 0, 0) == 1)
        {
            return true;
        }
    }
    return false;
}

version_number = global.vs_version;
vn_font = 4;
vn_x = 168;
vn_y = 128;
vn_c = 8421504;

enum UnknownEnum
{
    Value_0,
    Value_1,
    Value_2,
    Value_3,
    Value_4,
    Value_5,
    Value_6,
    Value_7,
    Value_8,
    Value_9
}
");

UndertaleCode c1 = Data.Code.ByName("gml_Object_obj_menu_Draw_0");
if (c1 == null) throw new Exception("Missing gml_Object_obj_menu_Draw_0");
imports.QueueReplace(c1, @"if (!global.menu)
{
    exit;
}
draw_sprite(spr_black_screen, 0, 0, 0);
draw_set_font(global.text_font);
var gwidth = global.game_width;
var gheight = global.game_height;
var ds_grid = menu_pages[page];
var ds_height = ds_grid_height(ds_grid);
var y_buffer = (page == 2) ? 13 : 16;
var x_buffer = (page == 2) ? 10 : 8;
var start_y = (gheight / 2) - (((ds_height - 1) / 2) * y_buffer);
var start_x = gwidth / 2;
if (page == 0)
{
    if (!got_any_burdens())
    {
        start_y += (y_buffer / 2);
    }
}
var c = 0;
draw_rectangle_color(0, 0, gwidth, gheight, c, c, c, c, false);
if (secret_timer > 0)
{
    var _delayframes = 7;
    var _bar_y = start_y + (menu_option[page] * y_buffer);
    var _bar_length = (secret_timer - _delayframes) * (224 / (60 - _delayframes));
    if (_bar_length > 0)
    {
        draw_set_color(c_gray);
        draw_rectangle(0, _bar_y - 8, _bar_length, _bar_y + 7, 0);
    }
}
draw_set_valign(fa_middle);
draw_set_halign(fa_right);
var ltx = start_x - x_buffer;
var _yoffset = 0;
var yy = 0;
repeat (ds_height)
{
    if (page == 0 && yy == 1)
    {
        if (!got_any_burdens())
        {
            _yoffset -= 16;
            yy++;
            continue;
        }
    }
    var lty = start_y + (yy * y_buffer) + _yoffset;
    c = 8421504;
    var xoffset = 0;
    if (yy == menu_option[page])
    {
        c = 16777215;
        if (page == 0 || page == 1 || page == 7)
        {
            draw_sprite(ds_grid_get(ds_grid, 3, yy), image_speed, 112 + menu_art_x, menu_art_y);
        }
    }
    if (page == 2)
    {
        draw_text_transformed_color(ltx + xoffset, lty, ds_grid_get(ds_grid, 0, yy), 0.9, 0.9, 0, c, c, c, c, 1);
    }
    else
    {
        draw_text_color(ltx + xoffset, lty, ds_grid_get(ds_grid, 0, yy), c, c, c, c, 1);
    }
    yy++;
}
draw_set_halign(fa_left);
var rtx = start_x + x_buffer;
_yoffset = 0;
yy = 0;
repeat (ds_height)
{
    if (page == 0 && yy == 1)
    {
        if (!got_any_burdens())
        {
            _yoffset -= 16;
            yy++;
            continue;
        }
    }
    var rty = start_y + (yy * y_buffer) + _yoffset;
    switch (ds_grid_get(ds_grid, 1, yy))
    {
        case UnknownEnum.Value_3:
            var current_val = ds_grid_get(ds_grid, 3, yy);
            var current_array = ds_grid_get(ds_grid, 4, yy);
            if (!is_real(current_val) || !is_array(current_array) || array_length(current_array) <= 0 || current_val < 0 || current_val >= array_length(current_array))
            {
                current_val = 0;
                ds_grid_set(ds_grid, 3, yy, 0);
            }
            var left_shift = ""<<"";
            var right_shift = "">>"";
            c = 8421504;
            if (current_val == 0)
            {
                left_shift = """";
            }
            if (current_val == (array_length(ds_grid_get(ds_grid, 4, yy)) - 1))
            {
                right_shift = """";
            }
            if (inputting && yy == menu_option[page])
            {
                counter++;
                if (counter <= 8)
                {
                    c = 8421504;
                }
                else
                {
                    c = 16777215;
                }
                if (counter > 16)
                {
                    counter = 0;
                }
                if (menu_pages[page] == ds_menu_language)
                {
                    var ilanguage = global.script_array[current_val][4];
                    load_fonts(current_val);
                    draw_set_font(global.script_array[current_val][5][9]);
                    draw_set_halign(fa_center);
                    draw_text_color(112, 48, ilanguage, c, c, c, c, 1);
                    draw_set_halign(fa_left);
                    draw_set_font(global.text_font);
                }
            }
            var _content = current_array[current_val];
            if (array_length(ds_grid_get(ds_grid, 4, yy)) >= 2)
            {
                if (ds_grid_get(ds_grid, 3, yy) == (array_length(ds_grid_get(ds_grid, 4, yy)) - 2))
                {
                    var _nextcontent = array_get(ds_grid_get(ds_grid, 4, yy), ds_grid_get(ds_grid, 3, yy) + 1);
                    if (is_string(_nextcontent))
                    {
                        if (string_copy(_nextcontent, 1, 6) == ""Secret"")
                        {
                            if (secret_timer < 12)
                            {
                                right_shift = """";
                            }
                            else if (secret_timer < 16)
                            {
                                right_shift = ""-"";
                            }
                            else if (secret_timer < 22)
                            {
                                right_shift = "">"";
                            }
                            else if (secret_timer < 26)
                            {
                                right_shift = "">-"";
                            }
                            else
                            {
                                right_shift = "">>"";
                            }
                        }
                    }
                }
            }
            if (is_string(_content))
            {
                if (string_copy(_content, 1, 6) == ""Secret"")
                {
                    _content = string_delete(_content, 1, 6);
                }
            }
            if (page == 2)
            {
                draw_text_transformed_color(rtx, rty, left_shift + _content + right_shift, 0.9, 0.9, 0, c, c, c, c, 1);
            }
            else
            {
                draw_text_color(rtx, rty, left_shift + _content + right_shift, c, c, c, c, 1);
            }
            break;
        case UnknownEnum.Value_2:
            var len = 64;
            var current_val = ds_grid_get(ds_grid, 3, yy);
            var current_array = ds_grid_get(ds_grid, 4, yy);
            var circle_pos = ((current_val - current_array[0]) / current_array[1]) - current_array[0];
            c = 8421504;
            for (var i = 0; i < 10; i += 1)
            {
                draw_sprite(spr_menu_page_icon_b, 0, rtx + (i * 8), rty - 4);
            }
            if (inputting && yy == menu_option[page])
            {
                slider_icon_speed += 0.25;
            }
            var ispeed = 0;
            var icon_count = floor(circle_pos * 10);
            for (var i = 0; i < icon_count; i += 1)
            {
                if (i == (icon_count - 1) && inputting && yy == menu_option[page])
                {
                    ispeed = slider_icon_speed;
                }
                draw_sprite(spr_menu_page_icon_a, ispeed, rtx + (i * 8), rty - 4);
            }
            if (inputting && yy == menu_option[page])
            {
                counter++;
                if (counter <= 8)
                {
                    c = 8421504;
                }
                else
                {
                    c = 16777215;
                }
                if (counter > 16)
                {
                    counter = 0;
                }
            }
            draw_set_halign(fa_center);
            draw_text_color(rtx + 90, rty, string(floor(circle_pos * 10)), c, c, c, c, 1);
            draw_set_halign(fa_left);
            break;
        case UnknownEnum.Value_4:
            var current_val = ds_grid_get(ds_grid, 3, yy);
            c = 16777215;
            if (inputting && yy == menu_option[page])
            {
                counter++;
                if (counter <= 8)
                {
                    c = 8421504;
                }
                else
                {
                    c = 16777215;
                }
                if (counter > 16)
                {
                    counter = 0;
                }
            }
            var c1, c2;
            if (current_val == 0)
            {
                c1 = c;
                c2 = 8421504;
            }
            else
            {
                c1 = 8421504;
                c2 = c;
            }
            if (page == 2)
            {
                draw_text_transformed_color(rtx, rty, scrScript(8), 0.9, 0.9, 0, c1, c1, c1, c1, 1);
            }
            else
            {
                draw_text_color(rtx, rty, scrScript(8), c1, c1, c1, c1, 1);
            }
            if (page == 2)
            {
                draw_text_transformed_color(rtx + 24, rty, scrScript(9), 0.9, 0.9, 0, c2, c2, c2, c2, 1);
            }
            else
            {
                draw_text_color(rtx + 32, rty, scrScript(9), c2, c2, c2, c2, 1);
            }
            break;
        case UnknownEnum.Value_5:
            var current_val = ds_grid_get(ds_grid, 3, yy);
            var string_val;
            switch (current_val)
            {
                case 38:
                    string_val = ""UP KEY"";
                    break;
                case 37:
                    string_val = ""LEFT KEY"";
                    break;
                case 39:
                    string_val = ""RIGHT KEY"";
                    break;
                case 40:
                    string_val = ""DOWN KEY"";
                    break;
                case 90:
                    string_val = ""Z"";
                    break;
                case 13:
                    string_val = ""ENTER"";
                    break;
                case 16:
                    string_val = ""SHIFT"";
                    break;
                case 17:
                    string_val = ""CTRL"";
                    break;
                case 162:
                    string_val = ""LEFT CTRL"";
                    break;
                case 163:
                    string_val = ""RIGHT CTRL"";
                    break;
                case 8:
                    string_val = ""BSPACE"";
                    break;
                case 18:
                    string_val = ""ALT"";
                    break;
                case 32:
                    string_val = ""SPACE"";
                    break;
                case 9:
                    string_val = ""TAB"";
                    break;
                case 46:
                    string_val = ""DEL"";
                    break;
                default:
                    string_val = chr(current_val);
                    break;
            }
            c = 8421504;
            if (inputting && yy == menu_option[page])
            {
                counter++;
                if (counter <= 8)
                {
                    c = 8421504;
                }
                else
                {
                    c = 16777215;
                }
                if (counter > 16)
                {
                    counter = 0;
                }
            }
            draw_text_color(rtx, rty, string_val, c, c, c, c, 1);
            break;
        case UnknownEnum.Value_8:
            var current_val = ds_grid_get(ds_grid, 3, yy);
            var button_number;
            switch (current_val)
            {
                case 32769:
                    button_number = 0;
                    break;
                case 32770:
                    button_number = 1;
                    break;
                case 32771:
                    button_number = 2;
                    break;
                case 32772:
                    button_number = 3;
                    break;
                case 32781:
                    button_number = 8;
                    break;
                case 32782:
                    button_number = 9;
                    break;
                case 32783:
                    button_number = 10;
                    break;
                case 32784:
                    button_number = 11;
                    break;
                case 32774:
                    button_number = 4;
                    break;
                case 32773:
                    button_number = 5;
                    break;
                case 32776:
                    button_number = 6;
                    break;
                case 32775:
                    button_number = 7;
                    break;
                case 32778:
                    button_number = 12;
                    break;
                case 32779:
                    button_number = 13;
                    break;
                case 32780:
                    button_number = 14;
                    break;
                default:
                    button_number = 15;
                    break;
            }
            c = 8421504;
            var icon_index = global.current_controller_sprite;
            if (inputting && yy == menu_option[page])
            {
                counter++;
                if (counter <= 8)
                {
                    icon_index = scr_get_dark_controller_sprite(global.current_controller_sprite);
                }
                else
                {
                    icon_index = global.current_controller_sprite;
                }
                if (counter > 16)
                {
                    counter = 0;
                }
            }
            draw_sprite(icon_index, button_number, rtx, rty - 8);
            break;
        case UnknownEnum.Value_6:
            break;
        case UnknownEnum.Value_7:
            break;
    }
    yy++;
}
draw_set_valign(fa_top);
if (draw_puumerkki == true)
{
    for (var i = 0; i < 36; i += 1)
    {
        var idim = 1;
        var ix = idim * i;
        var iy;
        if (i > 29)
        {
            iy = idim * 5;
            ix -= (idim * 30);
        }
        else if (i > 23)
        {
            iy = idim * 4;
            ix -= (idim * 24);
        }
        else if (i > 17)
        {
            iy = idim * 3;
            ix -= (idim * 18);
        }
        else if (i > 11)
        {
            iy = idim * 2;
            ix -= (idim * 12);
        }
        else if (i > 5)
        {
            iy = idim * 1;
            ix -= (idim * 6);
        }
        else
        {
            iy = 0;
        }
        var ipx = ix;
        var ipy = iy;
        var igrid = ds_grid_get(puumerkki_grid, ipx, ipy);
        if (igrid != 0)
        {
            draw_sprite(puumerkki_index, igrid, pm_x + ix, pm_y + iy);
        }
    }
}
if (gor_appears == true)
{
    for (var ig = 0; ig < 3; ig++)
    {
        var ic = choose(12632256, 8421504, 0);
        var ix = irandom_range(-2, 2);
        var iy = irandom_range(-2, 2);
        gpu_set_fog(true, ic, 0, 0);
        draw_sprite(gor_ec_index, 0, gor_x + ix, gor_y + iy);
        gpu_set_fog(false, c_white, 0, 0);
    }
    var igx = irandom_range(-1, 1);
    var igy = irandom_range(-1, 1);
    draw_sprite(gor_ec_index, gor_ec_speed, gor_x + igx, gor_y + igy);
    draw_set_halign(fa_center);
    if (gor_char_count != 0)
    {
        for (var is = 0; is < gor_char_count; is++)
        {
            var icc = 16777215;
            var ichar = string_char_at(gor_string, is + 1);
            var ichar_x = gor_string_x[is] + irandom_range(-2, 2);
            var ichar_y = gor_string_y[is] + irandom_range(-2, 2);
            draw_text_color(ichar_x, ichar_y, ichar, icc, icc, icc, icc, 1);
        }
    }
    draw_set_halign(fa_left);
}
if (menu_pages[page] == 9)
{
    if (draw_controller_info == 1)
    {
        draw_set_halign(fa_center);
        draw_set_font(controller_font);
        switch (found_controller)
        {
            case 0:
                var irx1 = ctrl_r_x1;
                var irx2 = ctrl_r_x2;
                var iry1 = ctrl_y + ctrl_r_y1 + 8;
                var iry2 = ctrl_y + ctrl_r_y2 + 8;
                draw_rectangle_color(irx1, iry1 - 16, irx2, iry2 - 16, cr_c, cr_c, cr_c, cr_c, false);
                draw_text_color(ctrl_x, ctrl_y - 16, scrScript(47), ct_c, ct_c, ct_c, ct_c, ct_a);
                draw_rectangle_color(irx1, iry1, irx2, iry2, cr_c, cr_c, cr_c, cr_c, false);
                draw_text_color(ctrl_x + ctrl_scroll_x, ctrl_y, ctrl_string, ct_c2, ct_c2, ct_c2, ct_c2, ct_a);
                draw_text_color(ctrl_x + ctrl_scroll_x + ctrl_scroll_add, ctrl_y, ctrl_string, ct_c2, ct_c2, ct_c2, ct_c2, ct_a);
                break;
            case 1:
                var gp_num = gamepad_get_device_count();
                var gp_count = global.controller_count;
                var iyh = gp_count * 8;
                var irx1 = ctrl_r_x1;
                var irx2 = ctrl_r_x2;
                var iry1 = ((ctrl_y + ctrl_r_y1) - iyh) + 8;
                var iry2 = ((ctrl_y + ctrl_r_y2) - iyh) + 8;
                draw_rectangle_color(irx1, iry1 - 16, irx2, iry2 - 16, cr_c, cr_c, cr_c, cr_c, false);
                draw_text_color(ctrl_x, ctrl_y - 16 - iyh, scrScript(50), ct_c, ct_c, ct_c, ct_c, ct_a);
                var irow_add = 0;
                for (var i = 0; i < gp_num; i++)
                {
                    if (global.gp[i] == true)
                    {
                        var isslot = ""SLOT["" + string(i) + ""] - "";
                        var irow_y = irow_add * 16;
                        irow_add++;
                        var idesc = gamepad_get_description(i);
                        var ilength = string_length(idesc);
                        if (ilength > 24)
                        {
                            idesc = string_copy(idesc, 1, 21) + ""..."";
                        }
                        draw_rectangle_color(irx1, iry1 + irow_y, irx2, iry2 + irow_y, cr_c, cr_c, cr_c, cr_c, false);
                        draw_text_color(ctrl_x, (ctrl_y + irow_y) - iyh, isslot + idesc, ct_c2, ct_c2, ct_c2, ct_c2, ct_a);
                    }
                }
                draw_rectangle_color(irx1, iry1 + (gp_count * 16), irx2, iry2 + (gp_count * 16), cr_c, cr_c, cr_c, cr_c, false);
                draw_text_color(ctrl_x + ctrl_scroll_x, (ctrl_y + (gp_count * 16)) - iyh, ctrl_string, ct_c2, ct_c2, ct_c2, ct_c2, ct_a);
                draw_text_color(ctrl_x + ctrl_scroll_x + ctrl_scroll_add, (ctrl_y + (gp_count * 16)) - iyh, ctrl_string, ct_c2, ct_c2, ct_c2, ct_c2, ct_a);
                break;
        }
        draw_set_halign(fa_left);
    }
}
if (draw_add_info == 1)
{
    draw_set_halign(fa_center);
    draw_set_font(controller_font);
    var irx1 = add_r_x1;
    var irx2 = add_r_x2;
    var iry1 = add_y + add_r_y1 + 8;
    var iry2 = add_y + add_r_y2 + 8;
    draw_rectangle_color(irx1, iry1, irx2, iry2, a_c, a_c, a_c, a_c, false);
    draw_text_color(add_x + add_scroll_x, add_y, add_string, a_c2, a_c2, a_c2, a_c2, a_a);
    draw_text_color(add_x + add_scroll_x + add_scroll_add, add_y, add_string, a_c2, a_c2, a_c2, a_c2, a_a);
    draw_set_halign(fa_left);
}
for (var ip = 0; ip < pageicon_count; ip++)
{
    var ip_index = 22;
    if (ip == (pageicon_count - 1))
    {
        ip_index = 207;
    }
    draw_sprite(ip_index, 0, pi_x + (8 * ip), pi_y);
}
draw_set_halign(fa_left);
draw_set_valign(fa_top);
draw_set_font(vn_font);
var ivn_string_length = string_length(version_number);
for (var i = 0; i < ivn_string_length; i++)
{
    var ivn_char = string_char_at(version_number, i + 1);
    var isep = 0;
    if (ivn_char == ""."")
    {
        isep = 2;
    }
    if (ivn_string_length < 7)
    {
        draw_text_color(vn_x + (8 * i) + isep, vn_y, ivn_char, vn_c, vn_c, vn_c, vn_c, 1);
    }
    else
    {
        draw_text_color((vn_x - 16) + (8 * i) + isep, vn_y, ivn_char, vn_c, vn_c, vn_c, vn_c, 1);
    }
}
var vita_brand = ""BY WOLFFS ROOM"";
for (var i = 0; i < string_length(vita_brand); i++)
{
    var ivn_char = string_char_at(vita_brand, i + 1);
    draw_text_color(2 + (8 * i), -5, ivn_char, vn_c, vn_c, vn_c, vn_c, 1);
}
draw_set_valign(fa_top);

enum UnknownEnum
{
    Value_2 = 2,
    Value_3,
    Value_4,
    Value_5,
    Value_6,
    Value_7,
    Value_8
}
");

UndertaleCode c2 = Data.Code.ByName("gml_Object_obj_game_Draw_77");
if (c2 == null) throw new Exception("Missing gml_Object_obj_game_Draw_77");
imports.QueueReplace(c2, @"var sx = surface_get_width(application_surface);
var sy = surface_get_height(application_surface);
var xsceel = min(window_get_width() / sx, window_get_height() / sy);
if (global.fullscreen_scaling == 0)
{
    xsceel = floor(xsceel);
}
var ysceel = xsceel;
var vita_graphics_menu_fullscreen = instance_exists(obj_menu) && obj_menu.page == 2;
if (vita_graphics_menu_fullscreen || (variable_global_exists(""vita_stretch_screen"") && global.vita_stretch_screen == 1))
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
if (variable_global_exists(""vita_brightness"") && global.vita_brightness < 1)
{
    draw_set_color(c_black);
    draw_set_alpha(1 - global.vita_brightness);
    draw_rectangle(0, 0, window_get_width(), window_get_height(), false);
    draw_set_alpha(1);
    draw_set_color(c_white);
}
");

UndertaleCode c3 = Data.Code.ByName("gml_GlobalScript_change_flicker");
if (c3 == null) throw new Exception("Missing gml_GlobalScript_change_flicker");
imports.QueueReplace(c3, @"function change_flicker(arg0)
{
    switch (arg0)
    {
        case 0:
            global.flicker_alpha = 1;
            break;
        case 1:
            global.flicker_alpha = 1;
            break;
        case 2:
            global.flicker_alpha = 0.6;
            break;
    }
    global.s_g_fli = arg0;
    ds_grid_set(obj_menu.ds_menu_graphics, 3, 3, arg0);
}
");

UndertaleCode c4 = Data.Code.ByName("gml_GlobalScript_language_menu");
if (c4 == null) throw new Exception("Missing gml_GlobalScript_language_menu");
imports.QueueReplace(c4, @"function language_menu()
{
    ds_grid_set(obj_menu.ds_menu_main, 0, 0, scrScript(12));
    ds_grid_set(obj_menu.ds_menu_main, 0, 1, scrScript(84));
    ds_grid_set(obj_menu.ds_menu_main, 0, 2, scrScript(82));
    ds_grid_set(obj_menu.ds_menu_main, 0, 3, scrScript(13));
    ds_grid_set(obj_menu.ds_menu_main, 0, 4, scrScript(14));
    ds_grid_set(obj_menu.ds_settings, 0, 0, scrScript(15));
    ds_grid_set(obj_menu.ds_settings, 0, 1, scrScript(16));
    ds_grid_set(obj_menu.ds_settings, 0, 2, scrScript(17));
    ds_grid_set(obj_menu.ds_settings, 0, 3, scrScript(31));
    ds_grid_set(obj_menu.ds_settings, 0, 4, scrScript(7));
    ds_grid_set(obj_menu.ds_menu_audio, 0, 0, scrScript(19));
    ds_grid_set(obj_menu.ds_menu_audio, 0, 1, scrScript(20));
    ds_grid_set(obj_menu.ds_menu_audio, 0, 2, scrScript(21));
    ds_grid_set(obj_menu.ds_menu_audio, 0, 3, scrScript(7));
    switch (global.language)
    {
        case 1:
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 0, ""SKAALAUS"");
            ds_grid_set(obj_menu.ds_menu_graphics, 4, 0, [""TARKKA"", ""SOVITETTU""]);
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 1, ""REUNOJEN VÄRI"");
            ds_grid_set(obj_menu.ds_menu_graphics, 4, 1, [""MUSTA"", ""TUMMA""]);
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 2, ""VSYNC"");
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 3, ""VÄLKKYMINEN"");
            ds_grid_set(obj_menu.ds_menu_graphics, 4, 3, [""60 FPS"", ""30 FPS"", ""VÄHENNETTY""]);
            break;
        case 0:
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 0, ""SCALING"");
            ds_grid_set(obj_menu.ds_menu_graphics, 4, 0, [""INTEGER"", ""FIT""]);
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 1, ""BORDER COLOR"");
            ds_grid_set(obj_menu.ds_menu_graphics, 4, 1, [""BLACK"", ""DARK""]);
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 2, ""VSYNC"");
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 3, ""FLASHING FX"");
            ds_grid_set(obj_menu.ds_menu_graphics, 4, 3, [""60 FPS"", ""30 FPS"", ""REDUCED""]);
            break;
        default:
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 0, scrScript(-3));
            ds_grid_set(obj_menu.ds_menu_graphics, 4, 0, [scrScript(-4), scrScript(-5)]);
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 1, scrScript(-6));
            ds_grid_set(obj_menu.ds_menu_graphics, 4, 1, [scrScript(-7), scrScript(-8)]);
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 2, scrScript(-9));
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 3, scrScript(-10));
            ds_grid_set(obj_menu.ds_menu_graphics, 4, 3, [scrScript(-11), scrScript(-12), scrScript(-13)]);
    }
    ds_grid_set(obj_menu.ds_menu_graphics, 0, 4, scrScript(24));
    ds_grid_set(obj_menu.ds_menu_graphics, 0, 5, scrScript(25));
    ds_grid_set(obj_menu.ds_menu_graphics, 0, 6, scrScript(72));
    switch (global.language)
    {
        case 1:
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 7, ""KIRKKAUS"");
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 8, ""VENYTÄ NÄYTTÖ"");
            break;
        case 2:
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 7, ""BRILLO"");
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 8, ""ESTIRAR PANTALLA"");
            break;
        case 3:
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 7, ""LUMINOSITÉ"");
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 8, ""ÉTIRER L'ÉCRAN"");
            break;
        case 4:
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 7, ""LUMINOSITÀ"");
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 8, ""ESTENDI SCHERMO"");
            break;
        case 5:
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 7, ""BRILHO"");
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 8, ""ESTICAR TELA"");
            break;
        default:
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 7, ""BRIGHTNESS"");
            ds_grid_set(obj_menu.ds_menu_graphics, 0, 8, ""STRETCH SCREEN"");
    }
    ds_grid_set(obj_menu.ds_menu_graphics, 4, 8, [scrScript(9), scrScript(8)]);
    ds_grid_set(obj_menu.ds_menu_graphics, 0, 9, scrScript(7));
    ds_grid_set(obj_menu.ds_menu_controls, 0, 0, scrScript(26));
    ds_grid_set(obj_menu.ds_menu_controls, 0, 1, scrScript(27));
    ds_grid_set(obj_menu.ds_menu_controls, 0, 2, scrScript(28));
    ds_grid_set(obj_menu.ds_menu_controls, 0, 3, scrScript(29));
    ds_grid_set(obj_menu.ds_menu_controls, 0, 4, scrScript(30));
    ds_grid_set(obj_menu.ds_menu_controls, 0, 5, scrScript(38));
    ds_grid_set(obj_menu.ds_menu_controls, 0, 6, scrScript(89));
    ds_grid_set(obj_menu.ds_menu_controls, 0, 7, scrScript(7));
    ds_grid_set(obj_menu.ds_menu_controller, 0, 0, scrScript(26));
    ds_grid_set(obj_menu.ds_menu_controller, 0, 1, scrScript(27));
    ds_grid_set(obj_menu.ds_menu_controller, 0, 2, scrScript(28));
    ds_grid_set(obj_menu.ds_menu_controller, 0, 3, scrScript(29));
    ds_grid_set(obj_menu.ds_menu_controller, 0, 4, scrScript(30));
    ds_grid_set(obj_menu.ds_menu_controller, 0, 5, scrScript(38));
    ds_grid_set(obj_menu.ds_menu_controller, 0, 6, scrScript(89));
    ds_grid_set(obj_menu.ds_menu_controller, 0, 7, scrScript(7));
    ds_grid_set(obj_menu.ds_menu_control_type, 0, 0, scrScript(7485));
    ds_grid_set(obj_menu.ds_menu_control_type, 0, 1, scrScript(7486));
    ds_grid_set(obj_menu.ds_menu_control_type, 0, 2, scrScript(7));
    ds_grid_set(obj_menu.ds_menu_language, 0, 0, scrScript(31));
    var _lang_arr = [];
    for (var _i = 0; _i < array_length(global.script_array); _i++)
    {
        array_push(_lang_arr, global.script_array[global.language][3][_i]);
    }
    ds_grid_set(obj_menu.ds_menu_language, 4, 0, _lang_arr);
    obj_menu.controller_font = global.script_array[global.language][5][12];
    ds_grid_set(obj_menu.ds_menu_language, 0, 1, scrScript(7));
    ds_grid_set(obj_menu.ds_menu_system, 0, 0, scrScript(75));
    ds_grid_set(obj_menu.ds_menu_system, 0, 1, scrScript(7));
    ds_grid_set(obj_menu.ds_menu_equipment, 0, 3, scrScript(7));
}
");

UndertaleCode c5 = Data.Code.ByName("gml_GlobalScript_scr_savesettings");
if (c5 == null) throw new Exception("Missing gml_GlobalScript_scr_savesettings");
imports.QueueReplace(c5, @"function scr_savesettings()
{
    switch (global.settings_ver)
    {
        case 0:
            var _Audio_gstring = ds_grid_create(1, ds_grid_height(obj_menu.ds_menu_audio) - 1);
            var yy = 0;
            repeat (ds_grid_height(_Audio_gstring))
            {
                ds_grid_add(_Audio_gstring, 0, yy, ds_grid_get(obj_menu.ds_menu_audio, 3, yy));
                yy++;
            }
            var _Graphics_gstring = ds_grid_create(1, ds_grid_height(obj_menu.ds_menu_graphics) - 1);
            yy = 0;
            repeat (ds_grid_height(_Graphics_gstring))
            {
                ds_grid_set(_Graphics_gstring, 0, yy, ds_grid_get(obj_menu.ds_menu_graphics, 3, yy));
                yy++;
            }
            var _Controls_gstring = ds_grid_create(1, ds_grid_height(obj_menu.ds_menu_controls) - 1);
            yy = 0;
            repeat (ds_grid_height(_Controls_gstring))
            {
                ds_grid_add(_Controls_gstring, 0, yy, ds_grid_get(obj_menu.ds_menu_controls, 3, yy));
                yy++;
            }
            var _Controller_gstring = ds_grid_create(1, ds_grid_height(obj_menu.ds_menu_controller) - 1);
            yy = 0;
            repeat (ds_grid_height(_Controller_gstring))
            {
                ds_grid_add(_Controller_gstring, 0, yy, ds_grid_get(obj_menu.ds_menu_controller, 3, yy));
                yy++;
            }
            var _Language_gstring_old = ds_grid_create(1, 1);
            ds_grid_add(_Language_gstring_old, 0, 0, 0);
            ds_grid_add(_Language_gstring_old, 0, 1, 1);
            var _Language_gstring = ds_grid_create(1, ds_grid_height(obj_menu.ds_menu_language) - 1);
            yy = 0;
            repeat (ds_grid_height(_Language_gstring))
            {
                ds_grid_add(_Language_gstring, 0, yy, ds_grid_get(obj_menu.ds_menu_language, 3, yy));
                yy++;
            }
            var _Equip_gstring = ds_grid_create(1, ds_grid_height(obj_menu.ds_menu_equipment) - 1);
            yy = 0;
            repeat (ds_grid_height(_Equip_gstring))
            {
                ds_grid_add(_Equip_gstring, 0, yy, ds_grid_get(obj_menu.ds_menu_equipment, 3, yy));
                yy++;
            }
            var _System_gstring = ds_grid_create(1, ds_grid_height(obj_menu.ds_menu_system) - 1);
            yy = 0;
            repeat (ds_grid_height(_System_gstring))
            {
                ds_grid_add(_System_gstring, 0, yy, ds_grid_get(obj_menu.ds_menu_system, 3, yy));
                yy++;
            }
            if (file_exists(""settings.vs""))
            {
                file_delete(""settings.vs"");
            }
            ini_open(""settings.vs"");
            ini_write_real(""Save1"", ""language_selection"", 0);
            ini_write_string(""Save1"", ""language_code"", global.script_array[global.language][0]);
            ini_write_string(""Save1"", ""Audio Settings"", ds_grid_write(_Audio_gstring));
            ini_write_string(""Save1"", ""Graphics Settings"", ds_grid_write(_Graphics_gstring));
            ini_write_string(""Save1"", ""Control Settings"", ds_grid_write(_Controls_gstring));
            ini_write_string(""Save1"", ""Controller Settings"", ds_grid_write(_Controller_gstring));
            ini_write_string(""Save1"", ""Language Settings"", ds_grid_write(_Language_gstring_old));
            ini_write_string(""Save1"", ""Language Settings New"", ds_grid_write(_Language_gstring));
            ini_write_string(""Save1"", ""Equip Settings"", ds_grid_write(_Equip_gstring));
            ini_write_string(""Save1"", ""System Settings"", ds_grid_write(_System_gstring));
            ini_write_string(""Save1"", ""Version"", ""4"");
            ini_close();
            if (file_exists(""settings.vslocal""))
            {
                file_delete(""settings.vslocal"");
            }
            ini_open(""settings.vslocal"");
            ini_write_string(""Save1"", ""Audio Settings"", ds_grid_write(_Audio_gstring));
            ini_write_string(""Save1"", ""Graphics Settings"", ds_grid_write(_Graphics_gstring));
            ini_write_string(""Save1"", ""Controller Settings"", ds_grid_write(_Controller_gstring));
            ini_write_string(""Save1"", ""Version"", ""4"");
            ini_close();
            ds_grid_destroy(_Audio_gstring);
            ds_grid_destroy(_Graphics_gstring);
            ds_grid_destroy(_Controls_gstring);
            ds_grid_destroy(_Controller_gstring);
            ds_grid_destroy(_Language_gstring);
            ds_grid_destroy(_Equip_gstring);
            ds_grid_destroy(_System_gstring);
            break;
    }
}
");

UndertaleCode c6 = Data.Code.ByName("gml_GlobalScript_scr_loadsettings");
if (c6 == null) throw new Exception("Missing gml_GlobalScript_scr_loadsettings");
imports.QueueReplace(c6, @"function scr_loadsettings()
{
    switch (global.settings_ver)
    {
        case 0:
            var _filename = ""settings.vs"";
            var _file_is_usable = false;
            if (file_exists(_filename))
            {
                if (scr_settings_check_corruption(_filename) == false)
                {
                    _file_is_usable = true;
                }
                else
                {
                }
            }
            if (!_file_is_usable)
            {
                _filename = ""settings_backup_1.vs"";
                if (file_exists(_filename))
                {
                    if (scr_settings_check_corruption(_filename) == false)
                    {
                        _file_is_usable = true;
                    }
                }
            }
            if (!_file_is_usable)
            {
                _filename = ""probably_no_file_named_like_this"";
            }
            if (file_exists(_filename))
            {
                ini_open(_filename);
                var _settings_version = ini_read_real(""Save1"", ""Version"", 2);
                var _language_selection = 0;
                if (ini_key_exists(""Save1"", ""language_code""))
                {
                    var _language_code = ini_read_string(""Save1"", ""language_code"", """");
                    for (var _i = 0; _i < array_length(global.script_array); _i++)
                    {
                        if (global.script_array[_i][0] == _language_code)
                        {
                            _language_selection = _i;
                            break;
                        }
                    }
                }
                else
                {
                    _language_selection = ini_read_real(""Save1"", ""language_selection"", 0);
                }
                var _Audio_gstring = ini_read_string(""Save1"", ""Audio Settings"", """");
                var _Graphics_gstring = ini_read_string(""Save1"", ""Graphics Settings"", """");
                var _Controls_gstring = ini_read_string(""Save1"", ""Control Settings"", """");
                var _Controller_gstring = ini_read_string(""Save1"", ""Controller Settings"", """");
                var _Language_gstring;
                if (ini_key_exists(""Save1"", ""Language Setting New""))
                {
                    _Language_gstring = ini_read_string(""Save1"", ""Language Settings New"", """");
                }
                else
                {
                    _Language_gstring = ini_read_string(""Save1"", ""Language Settings"", """");
                }
                var _Equip_gstring = ini_read_string(""Save1"", ""Equip Settings"", """");
                var _System_gstring = ini_read_string(""Save1"", ""Equip Settings"", """");
                ini_close();
                change_language(_language_selection);
                var _grid_substitute = ds_grid_create(1, 50);
                if (_settings_version <= 1)
                {
                    ds_grid_read(_grid_substitute, _Audio_gstring);
                    yy = 0;
                    repeat (ds_grid_height(obj_menu.ds_menu_audio) - 1)
                    {
                        ds_grid_set(obj_menu.ds_menu_audio, 3, yy, ds_grid_get(_grid_substitute, 0, yy));
                        yy++;
                    }
                }
                ds_grid_read(_grid_substitute, _Graphics_gstring);
                if (_settings_version >= 4)
                {
                    yy = 0;
                    repeat (ds_grid_height(obj_menu.ds_menu_graphics) - 1)
                    {
                        ds_grid_set(obj_menu.ds_menu_graphics, 3, yy, ds_grid_get(_grid_substitute, 0, yy));
                        yy++;
                    }
                }
                else if (_settings_version >= 3)
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
                else if (_settings_version == 2)
                {
                    ds_grid_set(obj_menu.ds_menu_graphics, 3, 4, ds_grid_get(_grid_substitute, 0, 2));
                    ds_grid_set(obj_menu.ds_menu_graphics, 3, 5, ds_grid_get(_grid_substitute, 0, 3));
                    ds_grid_set(obj_menu.ds_menu_graphics, 3, 6, ds_grid_get(_grid_substitute, 0, 4));
                }
                ds_grid_read(_grid_substitute, _Controls_gstring);
                var yy = 0;
                repeat (ds_grid_height(obj_menu.ds_menu_controls) - 1)
                {
                    ds_grid_set(obj_menu.ds_menu_controls, 3, yy, ds_grid_get(_grid_substitute, 0, yy));
                    yy++;
                }
                if (_settings_version <= 1)
                {
                    ds_grid_read(_grid_substitute, _Controller_gstring);
                    yy = 0;
                    repeat (ds_grid_height(obj_menu.ds_menu_controller) - 1)
                    {
                        ds_grid_set(obj_menu.ds_menu_controller, 3, yy, ds_grid_get(_grid_substitute, 0, yy));
                        yy++;
                    }
                }
                ds_grid_read(_grid_substitute, _Language_gstring);
                yy = 0;
                repeat (ds_grid_height(obj_menu.ds_menu_language) - 1)
                {
                    ds_grid_set(obj_menu.ds_menu_language, 3, yy, ds_grid_get(_grid_substitute, 0, yy));
                    yy++;
                }
                ds_grid_read(_grid_substitute, _Equip_gstring);
                yy = 0;
                repeat (ds_grid_height(obj_menu.ds_menu_equipment) - 1)
                {
                    ds_grid_set(obj_menu.ds_menu_equipment, 3, yy, ds_grid_get(_grid_substitute, 0, yy));
                    yy++;
                }
                if (string_length(_System_gstring) > 0)
                {
                    ds_grid_read(_grid_substitute, _System_gstring);
                    yy = 0;
                    repeat (ds_grid_height(obj_menu.ds_menu_system) - 1)
                    {
                        ds_grid_set(obj_menu.ds_menu_system, 3, yy, ds_grid_get(_grid_substitute, 0, yy));
                        yy++;
                    }
                }
                ds_grid_destroy(_grid_substitute);
                switch (_settings_version)
                {
                    case 1:
                        audio_master_gain(ds_grid_get(obj_menu.ds_menu_audio, 3, 0));
                        scr_audio_group_set_gain_vs(1, ds_grid_get(obj_menu.ds_menu_audio, 3, 1), 0);
                        scr_audio_group_set_gain_vs(2, ds_grid_get(obj_menu.ds_menu_audio, 3, 2), 0);
                        break;
                }
                change_fullscreen_scaling(ds_grid_get(obj_menu.ds_menu_graphics, 3, 0));
                change_border_fill(ds_grid_get(obj_menu.ds_menu_graphics, 3, 1));
                toggle_vsync(ds_grid_get(obj_menu.ds_menu_graphics, 3, 2));
                change_flicker(ds_grid_get(obj_menu.ds_menu_graphics, 3, 3));
                toggle_timer(ds_grid_get(obj_menu.ds_menu_graphics, 3, 4));
                toggle_counter(ds_grid_get(obj_menu.ds_menu_graphics, 3, 5));
                change_palette(ds_grid_get(obj_menu.ds_menu_graphics, 3, 6));
                change_resolution(ds_grid_get(obj_menu.ds_menu_graphics, 3, 7));
                change_window_mode(ds_grid_get(obj_menu.ds_menu_graphics, 3, 8));
                toggle_memory(ds_grid_get(obj_menu.ds_menu_equipment, 3, 0));
                toggle_wings(ds_grid_get(obj_menu.ds_menu_equipment, 3, 1));
                toggle_sword(ds_grid_get(obj_menu.ds_menu_equipment, 3, 2));
                global.key_up = ds_grid_get(obj_menu.ds_menu_controls, 3, 0);
                global.key_left = ds_grid_get(obj_menu.ds_menu_controls, 3, 1);
                global.key_right = ds_grid_get(obj_menu.ds_menu_controls, 3, 2);
                global.key_down = ds_grid_get(obj_menu.ds_menu_controls, 3, 3);
                global.key_action = ds_grid_get(obj_menu.ds_menu_controls, 3, 4);
                global.key_enter = ds_grid_get(obj_menu.ds_menu_controls, 3, 5);
                switch (_settings_version)
                {
                    case 1:
                        global.ctrl_up = ds_grid_get(obj_menu.ds_menu_controller, 3, 0);
                        global.ctrl_left = ds_grid_get(obj_menu.ds_menu_controller, 3, 1);
                        global.ctrl_right = ds_grid_get(obj_menu.ds_menu_controller, 3, 2);
                        global.ctrl_down = ds_grid_get(obj_menu.ds_menu_controller, 3, 3);
                        global.ctrl_action = ds_grid_get(obj_menu.ds_menu_controller, 3, 4);
                        global.ctrl_enter = ds_grid_get(obj_menu.ds_menu_controller, 3, 5);
                        break;
                }
                ds_grid_set(obj_menu.ds_menu_controller, 3, 6, ds_grid_get(obj_menu.ds_menu_controls, 3, 6));
                change_movement(ds_grid_get(obj_menu.ds_menu_controls, 3, 6));
                change_restshut_behavior(ds_grid_get(obj_menu.ds_menu_system, 3, 0));
                var _lang_arr = [];
                for (var _i = 0; _i < array_length(global.script_array); _i++)
                {
                    array_push(_lang_arr, global.script_array[_i][3][_i]);
                }
                ds_grid_set(obj_menu.ds_menu_language, 4, 0, _lang_arr);
                ds_grid_set(obj_menu.ds_menu_graphics, 4, 6, [""GRAY"", ""R***"", ""O***"", ""Y***"", ""G***"", ""B***"", ""I***"", ""V***"", ""MELLOW""]);
                ds_grid_set(obj_menu.ds_menu_language, 3, 0, global.language);
                global.palette = ds_grid_get(obj_menu.ds_menu_graphics, 3, 6);
                language_menu();
            }
            else
            {
            }
            _filename = ""settings.vslocal"";
            _file_is_usable = false;
            if (file_exists(_filename))
            {
                if (scr_settings_check_corruption(_filename) == false)
                {
                    _file_is_usable = true;
                }
                else
                {
                }
            }
            if (!_file_is_usable)
            {
                _filename = ""settings_backup_1.vslocal"";
                if (file_exists(_filename))
                {
                    if (scr_settings_check_corruption(_filename) == false)
                    {
                        _file_is_usable = true;
                    }
                }
            }
            if (!_file_is_usable)
            {
                _filename = ""probably_no_file_named_like_this"";
            }
            if (file_exists(_filename))
            {
                ini_open(_filename);
                var _settings_version = ini_read_real(""Save1"", ""Version"", 2);
                var _Audio_gstring = ini_read_string(""Save1"", ""Audio Settings"", """");
                var _Graphics_gstring = ini_read_string(""Save1"", ""Graphics Settings"", """");
                var _Controller_gstring = ini_read_string(""Save1"", ""Controller Settings"", """");
                ini_close();
                var _grid_substitute = ds_grid_create(1, 50);
                ds_grid_read(_grid_substitute, _Audio_gstring);
                var yy = 0;
                repeat (ds_grid_height(obj_menu.ds_menu_audio) - 1)
                {
                    ds_grid_set(obj_menu.ds_menu_audio, 3, yy, ds_grid_get(_grid_substitute, 0, yy));
                    yy++;
                }
                ds_grid_read(_grid_substitute, _Graphics_gstring);
                if (_settings_version >= 4)
                {
                    yy = 0;
                    repeat (ds_grid_height(obj_menu.ds_menu_graphics) - 1)
                    {
                        ds_grid_set(obj_menu.ds_menu_graphics, 3, yy, ds_grid_get(_grid_substitute, 0, yy));
                        yy++;
                    }
                }
                else if (_settings_version >= 3)
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
                else if (_settings_version == 2)
                {
                    ds_grid_set(obj_menu.ds_menu_graphics, 3, 4, ds_grid_get(_grid_substitute, 0, 2));
                    ds_grid_set(obj_menu.ds_menu_graphics, 3, 5, ds_grid_get(_grid_substitute, 0, 3));
                    ds_grid_set(obj_menu.ds_menu_graphics, 3, 6, ds_grid_get(_grid_substitute, 0, 4));
                }
                ds_grid_read(_grid_substitute, _Controller_gstring);
                yy = 0;
                repeat (ds_grid_height(obj_menu.ds_menu_controller) - 1)
                {
                    ds_grid_set(obj_menu.ds_menu_controller, 3, yy, ds_grid_get(_grid_substitute, 0, yy));
                    yy++;
                }
                ds_grid_destroy(_grid_substitute);
                audio_master_gain(ds_grid_get(obj_menu.ds_menu_audio, 3, 0));
                scr_audio_group_set_gain_vs(1, ds_grid_get(obj_menu.ds_menu_audio, 3, 1), 0);
                scr_audio_group_set_gain_vs(2, ds_grid_get(obj_menu.ds_menu_audio, 3, 2), 0);
                change_fullscreen_scaling(ds_grid_get(obj_menu.ds_menu_graphics, 3, 0));
                change_border_fill(ds_grid_get(obj_menu.ds_menu_graphics, 3, 1));
                toggle_vsync(ds_grid_get(obj_menu.ds_menu_graphics, 3, 2));
                change_flicker(ds_grid_get(obj_menu.ds_menu_graphics, 3, 3));
                toggle_timer(ds_grid_get(obj_menu.ds_menu_graphics, 3, 4));
                toggle_counter(ds_grid_get(obj_menu.ds_menu_graphics, 3, 5));
                change_palette(ds_grid_get(obj_menu.ds_menu_graphics, 3, 6));
                change_resolution(ds_grid_get(obj_menu.ds_menu_graphics, 3, 7));
                change_window_mode(ds_grid_get(obj_menu.ds_menu_graphics, 3, 8));
                global.ctrl_up = ds_grid_get(obj_menu.ds_menu_controller, 3, 0);
                global.ctrl_left = ds_grid_get(obj_menu.ds_menu_controller, 3, 1);
                global.ctrl_right = ds_grid_get(obj_menu.ds_menu_controller, 3, 2);
                global.ctrl_down = ds_grid_get(obj_menu.ds_menu_controller, 3, 3);
                global.ctrl_action = ds_grid_get(obj_menu.ds_menu_controller, 3, 4);
                global.ctrl_enter = ds_grid_get(obj_menu.ds_menu_controller, 3, 5);
                language_menu();
            }
            else
            {
            }
            break;
    }
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
    {
        global.language = 0;
    }
}
");

imports.Import();
ScriptMessage("Void Stranger Vita Graphics Menu v4 applied.");