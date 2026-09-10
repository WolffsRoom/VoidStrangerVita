using UndertaleModLib.Compiler;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Models;

EnsureDataLoaded();
CodeImportGroup imports = new(Data) { AutoCreateAssets = true };
string Norm(UndertaleCode c) => GetDecompiledText(c).Replace("\r\n", "\n");

UndertaleCode stepCode = Data.Code.ByName("gml_Object_obj_menu_Step_0");
string step = Norm(stepCode);
int ss = step.IndexOf("        if (page == 2)\n        {\n            repeat (2)");
if (ss >= 0)
{
    int se = step.IndexOf("        if (menu_option[page] > (ds_height - 1))", ss);
    if (se < 0) throw new Exception("Vita page-2 navigation cleanup end not found");
    step = step.Substring(0, ss) + step.Substring(se);
}
imports.QueueReplace(stepCode, step);

UndertaleCode drawCode = Data.Code.ByName("gml_Object_obj_menu_Draw_0");
string draw = Norm(drawCode);
int dh = draw.IndexOf("if (page == 2)\n{\n    if (!window_get_fullscreen())");
if (dh >= 0)
{
    int de = draw.IndexOf("var c = 0;", dh);
    if (de < 0) throw new Exception("Graphics draw header cleanup end not found");
    draw = draw.Substring(0, dh) + draw.Substring(de);
}
for (int pass = 0; pass < 2; pass++)
{
    int ds = draw.IndexOf("    if (page == 2)\n    {\n        if ((!window_get_fullscreen()");
    if (ds < 0) break;
    int dl = draw.IndexOf(pass == 0 ? "    var lty =" : "    var rty =", ds);
    if (dl < 0)
    {
        // Determine which loop we are in if prior formatting changed.
        int l = draw.IndexOf("    var lty =", ds);
        int r = draw.IndexOf("    var rty =", ds);
        dl = (l >= 0 && (r < 0 || l < r)) ? l : r;
    }
    if (dl < 0) throw new Exception("Graphics draw skip cleanup end not found");
    draw = draw.Substring(0, ds) + draw.Substring(dl);
}
imports.QueueReplace(drawCode, draw);

UndertaleCode loadCode = Data.Code.ByName("gml_GlobalScript_scr_loadsettings");
string load = Norm(loadCode);
string gfxLoad = @"                ds_grid_read(_grid_substitute, _Graphics_gstring);
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
                else if (_settings_version == 2)
                {
                    // Preserve the settings that existed in v2; new Vita-only values use defaults.
                    ds_grid_set(obj_menu.ds_menu_graphics, 3, 4, ds_grid_get(_grid_substitute, 0, 2));
                    ds_grid_set(obj_menu.ds_menu_graphics, 3, 5, ds_grid_get(_grid_substitute, 0, 3));
                    ds_grid_set(obj_menu.ds_menu_graphics, 3, 6, ds_grid_get(_grid_substitute, 0, 4));
                }
";
// First settings.vs graphics load block.
int g1 = load.IndexOf("                ds_grid_read(_grid_substitute, _Graphics_gstring);");
if (g1 < 0) throw new Exception("Primary Graphics Settings load not found");
int g1e = load.IndexOf("                ds_grid_read(_grid_substitute, _Controls_gstring);", g1);
if (g1e < 0) throw new Exception("Primary Graphics Settings load end not found");
load = load.Substring(0, g1) + gfxLoad + load.Substring(g1e);

// Secondary settings.vslocal graphics load block.
int g2 = load.IndexOf("                ds_grid_read(_grid_substitute, _Graphics_gstring);", g1 + gfxLoad.Length);
if (g2 >= 0)
{
    int g2e = load.IndexOf("                ds_grid_read(_grid_substitute, _Controller_gstring);", g2);
    if (g2e < 0) throw new Exception("Secondary Graphics Settings load end not found");
    load = load.Substring(0, g2) + gfxLoad + load.Substring(g2e);
}

// Any legacy application block in the secondary local-settings path uses PC indices.
string oldApply = @"                change_window_mode(ds_grid_get(obj_menu.ds_menu_graphics, 3, 0));
                change_resolution(ds_grid_get(obj_menu.ds_menu_graphics, 3, 1));
                change_fullscreen_scaling(ds_grid_get(obj_menu.ds_menu_graphics, 3, 2));
                change_border_fill(ds_grid_get(obj_menu.ds_menu_graphics, 3, 3));
                toggle_vsync(ds_grid_get(obj_menu.ds_menu_graphics, 3, 4));
                change_flicker(ds_grid_get(obj_menu.ds_menu_graphics, 3, 5));";
string newApply = @"                change_fullscreen_scaling(ds_grid_get(obj_menu.ds_menu_graphics, 3, 0));
                change_border_fill(ds_grid_get(obj_menu.ds_menu_graphics, 3, 1));
                toggle_vsync(ds_grid_get(obj_menu.ds_menu_graphics, 3, 2));
                change_flicker(ds_grid_get(obj_menu.ds_menu_graphics, 3, 3));
                toggle_timer(ds_grid_get(obj_menu.ds_menu_graphics, 3, 4));
                toggle_counter(ds_grid_get(obj_menu.ds_menu_graphics, 3, 5));
                change_palette(ds_grid_get(obj_menu.ds_menu_graphics, 3, 6));
                change_resolution(ds_grid_get(obj_menu.ds_menu_graphics, 3, 7));
                change_window_mode(ds_grid_get(obj_menu.ds_menu_graphics, 3, 8));";
load = load.Replace(oldApply, newApply);
load = load.Replace("ds_grid_set(obj_menu.ds_menu_graphics, 4, 4, [\"GRAY\", \"R***\", \"O***\", \"Y***\", \"G***\", \"B***\", \"I***\", \"V***\"]);",
                    "ds_grid_set(obj_menu.ds_menu_graphics, 4, 6, [\"GRAY\", \"R***\", \"O***\", \"Y***\", \"G***\", \"B***\", \"I***\", \"V***\", \"MELLOW\"]);");
load = load.Replace("global.palette = ds_grid_get(obj_menu.ds_menu_graphics, 3, 4);",
                    "global.palette = ds_grid_get(obj_menu.ds_menu_graphics, 3, 6);");
imports.QueueReplace(loadCode, load);

imports.Import();
ScriptMessage("Void Stranger Vita menu v2 post-cleanup applied.");
