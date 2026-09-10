using UndertaleModLib.Compiler;
using UndertaleModLib.Decompiler;
using UndertaleModLib.Models;
EnsureDataLoaded();
CodeImportGroup imports = new(Data) { AutoCreateAssets = true };

UndertaleCode lang = Data.Code.ByName("gml_GlobalScript_language_menu");
if (lang == null) throw new Exception("Missing language_menu");
imports.QueueReplace(lang, @"
function language_menu()
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
        array_push(_lang_arr, global.script_array[global.language][3][_i]);
    ds_grid_set(obj_menu.ds_menu_language, 4, 0, _lang_arr);
    obj_menu.controller_font = global.script_array[global.language][5][12];
    ds_grid_set(obj_menu.ds_menu_language, 0, 1, scrScript(7));
    ds_grid_set(obj_menu.ds_menu_system, 0, 0, scrScript(75));
    ds_grid_set(obj_menu.ds_menu_system, 0, 1, scrScript(7));
    ds_grid_set(obj_menu.ds_menu_equipment, 0, 3, scrScript(7));
}
");

UndertaleCode createCode = Data.Code.ByName("gml_Object_obj_menu_Create_0");
string create = GetDecompiledText(createCode).Replace("\r\n", "\n");
string oldInit = @"ds_grid_set(ds_menu_graphics, 3, 7, 9);
ds_grid_set(ds_menu_graphics, 3, 8, 0);
global.vita_brightness = 1;
global.vita_stretch_screen = 0;";
string newInit = @"if (!variable_global_exists(""vita_brightness"")) global.vita_brightness = 1;
if (!variable_global_exists(""vita_stretch_screen"")) global.vita_stretch_screen = 0;
ds_grid_set(ds_menu_graphics, 3, 7, clamp(round(global.vita_brightness * 10) - 1, 0, 9));
ds_grid_set(ds_menu_graphics, 3, 8, global.vita_stretch_screen);";
if (create.Contains(oldInit)) create = create.Replace(oldInit, newInit);
imports.QueueReplace(createCode, create);

imports.Import();
ScriptMessage("Void Stranger Vita language menu v2 aligned.");
