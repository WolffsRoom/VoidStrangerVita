using System;
using System.Text.RegularExpressions;
using UndertaleModLib;
using UndertaleModLib.Models;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Compiler;

EnsureDataLoaded();
CodeImportGroup imports = new(Data) { AutoCreateAssets = true };
string Norm(UndertaleCode c) => GetDecompiledText(c).Replace("\r\n", "\n");

var stepCode = Data.Code.ByName("gml_Object_obj_menu_Step_0");
if (stepCode == null) throw new Exception("Missing obj_menu Step_0");
string step = Norm(stepCode);

// Graphics Menu v5 dispatched by numeric row. This was fragile because adding/reordering
// a row changes the meaning of the index. Dispatch by the callback stored in column 2,
// which is the menu's original source of truth.
string callbackBlock = @"if (page == 2)
                {
                    var vita_gfx_row = array_get(menu_option, page);
                    var vita_gfx_value = ds_grid_get(ds_, 3, vita_gfx_row);
                    var vita_gfx_callback = ds_grid_get(ds_, 2, vita_gfx_row);
                    if (vita_gfx_callback == ""change_fullscreen_scaling"")
                        change_fullscreen_scaling(vita_gfx_value);
                    else if (vita_gfx_callback == ""change_border_fill"")
                        change_border_fill(vita_gfx_value);
                    else if (vita_gfx_callback == ""toggle_vsync"")
                        toggle_vsync(vita_gfx_value);
                    else if (vita_gfx_callback == ""change_flicker"")
                        change_flicker(vita_gfx_value);
                    else if (vita_gfx_callback == ""toggle_timer"")
                        toggle_timer(vita_gfx_value);
                    else if (vita_gfx_callback == ""toggle_counter"")
                        toggle_counter(vita_gfx_value);
                    else if (vita_gfx_callback == ""change_palette"")
                        change_palette(vita_gfx_value);
                    else if (vita_gfx_callback == ""change_resolution"")
                        change_resolution(vita_gfx_value);
                    else if (vita_gfx_callback == ""change_window_mode"")
                        change_window_mode(vita_gfx_value);
                    scr_savesettings();
                }";

var liveRegex = new Regex(@"if \(page == 2\)\n\s*\{\n\s*var vita_gfx_row = array_get\(menu_option, page\);\n\s*var vita_gfx_value = ds_grid_get\(ds_, 3, vita_gfx_row\);\n\s*switch \(vita_gfx_row\)\n\s*\{.*?\n\s*\}\n\s*scr_savesettings\(\);\n\s*\}", RegexOptions.Singleline);
int liveCount = liveRegex.Matches(step).Count;
if (liveCount != 2) throw new Exception("Expected 2 v5 live graphics blocks, found " + liveCount);
step = liveRegex.Replace(step, callbackBlock);

// Graphics rows are now committed as the user moves left/right. Confirm only exits
// edit mode; do not execute the same callback a second time through asset_get_index.
string oldConfirm = "if (inputting)\n            {\n                show_debug_message(\"Executing toggle script \" + ds_grid_get(ds_, 2, array_get(menu_option, page))";
string newConfirm = "if (inputting && page != 2)\n            {\n                show_debug_message(\"Executing toggle script \" + ds_grid_get(ds_, 2, array_get(menu_option, page))";
if (!step.Contains(oldConfirm)) throw new Exception("Graphics confirmation callback block not found");
step = step.Replace(oldConfirm, newConfirm);
imports.QueueReplace(stepCode, step);

// Palette was moved to row 6 when Brightness and Stretch Screen were added. The old
// Alarm[2] still read row 8 (now Stretch), then called change_palette(), making Palette
// snap to Stretch's 0/1 value. Reapply the palette array directly and do not re-arm
// Alarm[2] recursively.
var alarmCode = Data.Code.ByName("gml_Object_obj_menu_Alarm_2");
if (alarmCode == null) throw new Exception("Missing obj_menu Alarm_2");
imports.QueueReplace(alarmCode, "set_palette(ds_grid_get(ds_menu_graphics, 3, 6));");

imports.Import();
ScriptMessage("Void Stranger Graphics Menu v6: callback-based live apply + Palette Alarm row fixed.");
