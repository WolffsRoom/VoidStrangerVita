using UndertaleModLib.Compiler;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Models;
EnsureDataLoaded();
CodeImportGroup imports = new(Data) { AutoCreateAssets = true };
UndertaleCode c = Data.Code.ByName("gml_GlobalScript_scr_loadsettings");
string s = GetDecompiledText(c).Replace("\r\n", "\n");
int secondV3 = s.IndexOf("if (_settings_version >= 3)", s.IndexOf("if (_settings_version >= 3)") + 1);
if (secondV3 >= 0)
{
    int a = s.IndexOf("                change_window_mode(ds_grid_get(obj_menu.ds_menu_graphics", secondV3);
    int b = s.IndexOf("                global.ctrl_up", a);
    if (a >= 0 && b > a)
    {
        string rep = @"                change_fullscreen_scaling(ds_grid_get(obj_menu.ds_menu_graphics, 3, 0));
                change_border_fill(ds_grid_get(obj_menu.ds_menu_graphics, 3, 1));
                toggle_vsync(ds_grid_get(obj_menu.ds_menu_graphics, 3, 2));
                change_flicker(ds_grid_get(obj_menu.ds_menu_graphics, 3, 3));
                toggle_timer(ds_grid_get(obj_menu.ds_menu_graphics, 3, 4));
                toggle_counter(ds_grid_get(obj_menu.ds_menu_graphics, 3, 5));
                change_palette(ds_grid_get(obj_menu.ds_menu_graphics, 3, 6));
                change_resolution(ds_grid_get(obj_menu.ds_menu_graphics, 3, 7));
                change_window_mode(ds_grid_get(obj_menu.ds_menu_graphics, 3, 8));
";
        s = s.Substring(0, a) + rep + s.Substring(b);
    }
}
imports.QueueReplace(c, s);
imports.Import();
ScriptMessage("Void Stranger Vita menu v2 secondary load fixed.");
