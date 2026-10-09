#include <psp2/ctrl.h>
int _newlib_heap_size_user = 256 * 1024 * 1024;
#include <psp2/appmgr.h>
#include <psp2/apputil.h>
#include <psp2/common_dialog.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/io/dirent.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/gxm.h>
#include <psp2/message_dialog.h>
#include <psp2/power.h>
#include <psp2/rtc.h>
#include <psp2/touch.h>
#include <psp2/sysmodule.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vitaGL.h>

// Present in the bundled vitaGL fork but absent from the installed public header.
void vglPhycontMemLazyInit(size_t size);

#include "data_win.h"
#include "collision.h"
#include "gl_legacy_renderer.h"
#include "gl_renderer.h"
#include "audio/openal/al_audio_system.h"
#include "overlay_file_system.h"
#include "debug_overlay.h"
#include "runner.h"
#include "runner_gamepad.h"
#include "runner_keyboard.h"
#include "runner_mouse.h"
#include "spatial_grid.h"
#include "vita_settings.h"
#include "vita_borders.h"
#include "vita_video.h"
#include "vita_trophies.h"
#include "vm.h"
#include "vm_builtins.h"
#include "profiler.h"
#include "stb_image.h"
#include "loading_animation.h"
#include "fruit_touch.h"
#include "dev_room_nav.h"
#include "voidstranger_audio_ids.h"
#include "missing_data_scene.h"

#define DATA_ROOT "ux0:data/voidstranger/"
#define SAVE_PATH "ux0:data/voidstranger/saves/"
#define LOG_PATH DATA_ROOT "butterscotch-probe.log"
#define COMPAT_LOG_PATH DATA_ROOT "compat-diagnostics.log"
#define NEXT_CHAPTER_PATH DATA_ROOT "next-chapter.txt"
#define DEV_LOG_ROOT DATA_ROOT "devlogs"
#define PORT_BUILD_VERSION "v2.1"
#define VITA_CDIALOG_MEMORY_SIZE 0x8C6000

// Read by the Vita renderer to apply chapter-specific memory safety policies.
int g_vitaActiveChapter = 0;
int g_vitaSpanishModActive = 0;
// Shared by every Vita backend that can append to butterscotch-probe.log.
// Keep retail play free from repeated memory-card writes unless Dev Mode was
// explicitly enabled before startup.
int g_vitaProbeLoggingEnabled = 0;
// Read by room-transition code before accessing backend-specific structures.
bool g_vitaModernGlActive = false;
extern int g_vitaGameVsyncEnabled;

static SceUID probe_log_fd = -1;
static SceUID dev_log_fd = -1;
static char* dev_log_buffer = NULL;
static size_t dev_log_buffer_size = 0;
static size_t dev_log_buffer_capacity = 0;
enum {
    LOADING_LANG_ENGLISH = 0,
    LOADING_LANG_PTBR,
    LOADING_LANG_SPANISH,
    LOADING_LANG_ITALIAN,
    LOADING_LANG_TURKISH,
    LOADING_LANG_COUNT
};

#define LOADING_FLOOR_COUNT 11
#define LOADING_EDGE_SLIDE_FRAMES 47
#define LOADING_STATE23_TOTAL_FRAMES 60
#define LOADING_FALL_DELAY_FRAMES 4
#define LOADING_FALL_GAME_FRAMES 40
#define LOADING_COMPLETE_HOLD_US 840000ULL
#define LOADING_FADEOUT_US 700000ULL

static GLuint loading_sprite_atlas = 0;
static GLuint loading_digits_texture = 0;
static GLuint loading_prefix_textures[LOADING_LANG_COUNT] = {0};
static GLuint loading_complete_textures[LOADING_LANG_COUNT] = {0};
static int loading_language = LOADING_LANG_ENGLISH;

typedef struct {
    int currentCell;
    int targetCell;
    bool stepActive;
    uint64_t stepStartedUs;
    float ratio;
    int labelType;
} LoadingAnimationRuntime;

static LoadingAnimationRuntime loading_animation = {0};
static AudioSystem* loading_audio_system = NULL;

static GLuint startup_skip_texture = 0;
static GLuint startup_black_texture = 0;
static bool startup_long_skipped = false;
static bool startup_long_watched = false;
static void log_line(const char *text);
static void dev_log_write(const char* text);
static void loading_screen_init(void);
static void loading_screen_shutdown(void);
static void loading_screen_set_language(const char* language);
static void draw_loading_screen(bool showProgress, float ratio, int labelType);
static void play_loading_complete_sequence(void);
static bool play_startup_video_sequence(void);

// Lightweight CRT presentation filter.  It draws sparse one-pixel dark lines
// into the host framebuffer after the game and Vita overlays are composed.
// No additional surface, shader program, texture or framebuffer is allocated.
// Sharp Bilinear is handled by the texture upload filtering policy instead and
// therefore also remains a single-pass option.
static void draw_screen_filter(const VitaSettings* settings, Renderer* renderer) {
    if (settings == NULL || renderer == NULL || settings->screenFilterMode == 0 ||
        settings->screenFilterMode == 2) return;
    int previousOverlayMode = g_vitaPortOverlayFullScreen;
    g_vitaPortOverlayFullScreen = 1;
    renderer->vtable->beginGUI(renderer, 960, 544, 0, 0, 960, 544,
                               RENDER_TARGET_HOST_FRAMEBUFFER);
    if (settings->screenFilterMode == 1) {
        // A six-line pitch costs only 91 batched translucent quads per frame.
        for (int y = 3; y < 544; y += 6)
            renderer->vtable->drawRectangle(renderer, 0.0f, (float)y, 960.0f,
                                             (float)(y + 1), 0x000000, 0.18f, false);
    } else if (settings->screenFilterMode == 3) {
        // Visible low-cost CRT glass approximation. Progressive corner masks
        // reproduce the tube silhouette without a second framebuffer.
        for (int i = 0; i < 12; ++i) {
            float edge = 3.0f + (float)i * 2.0f;
            float cut = (float)((11 - i) * (11 - i)) * 0.34f;
            float alpha = 0.10f + (float)(11 - i) * 0.018f;
            renderer->vtable->drawRectangle(renderer, 0, (float)i * 4.0f, cut,
                                             (float)i * 4.0f + edge, 0x000000, alpha, false);
            renderer->vtable->drawRectangle(renderer, 960.0f - cut, (float)i * 4.0f, 960,
                                             (float)i * 4.0f + edge, 0x000000, alpha, false);
            renderer->vtable->drawRectangle(renderer, 0, 544.0f - (float)i * 4.0f - edge, cut,
                                             544.0f - (float)i * 4.0f, 0x000000, alpha, false);
            renderer->vtable->drawRectangle(renderer, 960.0f - cut, 544.0f - (float)i * 4.0f - edge, 960,
                                             544.0f - (float)i * 4.0f, 0x000000, alpha, false);
        }
        for (int y = 3; y < 544; y += 6)
            renderer->vtable->drawRectangle(renderer, 0, (float)y, 960,
                                             (float)(y + 1), 0x000000, 0.10f, false);
    } else if (settings->screenFilterMode == 4) {
        // Sparse ordered-dither blend; no texture, shader or surface needed.
        for (int y = 2; y < 544; y += 16)
            for (int x = ((y / 16) & 1) ? 8 : 0; x < 960; x += 16)
                renderer->vtable->drawRectangle(renderer, (float)x, (float)y,
                                                 (float)(x + 2), (float)(y + 2),
                                                 0xFFFFFF, 0.055f, false);
    } else if (settings->screenFilterMode == 5) {
        // DELTARUNE exposes no per-pixel depth buffer. Use a visible focus-plane
        // approximation and preserve the center/character plane unchanged.
        for (int i = 0; i < 10; ++i) {
            float h = 14.0f + (float)i * 9.0f;
            float alpha = 0.035f + (float)i * 0.010f;
            renderer->vtable->drawRectangle(renderer, 0, 0, 960, h, 0x101018, alpha, false);
            renderer->vtable->drawRectangle(renderer, 0, 544 - h, 960, 544, 0x101018, alpha, false);
        }
    }
    renderer->vtable->endGUI(renderer);
    g_vitaPortOverlayFullScreen = previousOverlayMode;
}

// Display startup failures with the native Vita dialog instead of leaving the
// user at LiveArea with no explanation. VitaGL handles sceCommonDialogUpdate
// when vglSwapBuffers receives GL_TRUE, so this remains available even before
// the GameMaker renderer and its fonts have been created.
static void show_startup_error_and_exit(const char* reason, const char* path) {
    // Initialize loading screen first to draw behind the dialog
    loading_screen_init();

    // Draw loading screen immediately so the screen is not black/blank
    draw_loading_screen(false, 0.0f, 0);
    vglSwapBuffers(GL_TRUE);

    char message[512];
    snprintf(message, sizeof(message),
             "DELTARUNE Vita nao pode iniciar.\n\n%s\n\nCaminho esperado:\n%s\n\nCertifique-se de que os dados do jogo foram copiados para o caminho correto e que os plugins obrigatorios (como o NoTrpDrm para suporte a trofeus nativos) estao devidamente instalados e ativos no seu PS Vita.",
             reason != NULL ? reason : "Os dados do jogo estao incompletos.",
             path != NULL ? path : DATA_ROOT);

    SceAppUtilInitParam appUtilParam;
    SceAppUtilBootParam appUtilBootParam;
    memset(&appUtilParam, 0, sizeof(appUtilParam));
    memset(&appUtilBootParam, 0, sizeof(appUtilBootParam));
    sceAppUtilInit(&appUtilParam, &appUtilBootParam);

    SceCommonDialogConfigParam dialogConfig;
    sceCommonDialogConfigParamInit(&dialogConfig);
    int sysLang = SCE_SYSTEM_PARAM_LANG_ENGLISH_US;
    int enterBtn = SCE_SYSTEM_PARAM_ENTER_BUTTON_CROSS;
    sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, &sysLang);
    sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &enterBtn);
    dialogConfig.language = (SceSystemParamLang)sysLang;
    dialogConfig.enterButtonAssign = (SceSystemParamEnterButtonAssign)enterBtn;
    sceCommonDialogSetConfigParam(&dialogConfig);

    SceMsgDialogUserMessageParam userParam;
    memset(&userParam, 0, sizeof(userParam));
    userParam.buttonType = SCE_MSG_DIALOG_BUTTON_TYPE_OK;
    userParam.msg = (const SceChar8*)message;

    SceMsgDialogParam param;
    sceMsgDialogParamInit(&param);
    param.mode = SCE_MSG_DIALOG_MODE_USER_MSG;
    param.userMsgParam = &userParam;

    int dialogResult = sceMsgDialogInit(&param);
    char dialogLog[160];
    snprintf(dialogLog, sizeof(dialogLog),
             "STARTUP_DIALOG init=0x%08X", (unsigned)dialogResult);
    log_line(dialogLog);

    if (dialogResult >= 0) {
        int loopCount = 0;
        while (sceMsgDialogGetStatus() != SCE_COMMON_DIALOG_STATUS_FINISHED) {
            // Draw loading screen and spinner behind the native dialog
            draw_loading_screen(false, 0.0f, 0);
            vglSwapBuffers(GL_TRUE);
            loopCount++;
            if (loopCount > 36000) break; // Safeguard so it doesn't hang forever
        }
        SceMsgDialogResult result;
        memset(&result, 0, sizeof(result));
        sceMsgDialogGetResult(&result);
        sceMsgDialogTerm();
    } else {
        // Draw a RED screen for 5 seconds to visual-signal failure
        uint64_t start = sceKernelGetProcessTimeWide();
        while (sceKernelGetProcessTimeWide() - start < 5000000ULL) {
            glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            vglSwapBuffers(GL_FALSE);
        }
    }
    sceAppUtilShutdown();
    loading_screen_shutdown();
    sceKernelExitProcess(0);
}

// Feature 2: verify the essential game data exists BEFORE booting a chapter.
// main() creates the deltarunevita/ directory for save handling, so an empty
// directory alone is not proof the game data was installed. The real signal is
// the presence of chapter0/data.win (the base game) and the active chapter's
// data.win. When anything essential is missing we list every missing item in
// the native dialog and exit cleanly instead of proceeding into a crash (the
// garbage room-name dereference seen in the boot crash dump).
static bool g_initialRootExists = false;



static bool is_directory(const char* path) {
    SceIoStat st;
    memset(&st, 0, sizeof(st));
    if (sceIoGetstat(path, &st) < 0) return false;
    return SCE_S_ISDIR(st.st_mode);
}

static bool is_file_valid(const char* path) {
    SceIoStat st;
    memset(&st, 0, sizeof(st));
    if (sceIoGetstat(path, &st) < 0) return false;
    return !SCE_S_ISDIR(st.st_mode) && st.st_size >= 1024;
}

static void verify_essential_data_or_report(int active_chapter) {
    char missing[512] = {0};
    size_t used = 0;
    bool anyMissing = false;

    #define REPORT_APPEND(line) do { \
        int _w = snprintf(missing + used, sizeof(missing) - used, "%s", (line)); \
        if (_w > 0 && (size_t) _w < sizeof(missing) - used) used += (size_t) _w; \
    } while (0)

    REPORT_APPEND("Estrutura de dados incompleta:\n");

    // 1. Check ux0:data/deltarune/
    if (!is_directory("ux0:data/deltarune")) {
        REPORT_APPEND("- pasta ux0:data/deltarune/ ausente\n");
        anyMissing = true;
    }
    // 2. Check ux0:data/voidstranger/
    else if (!is_directory("ux0:data/deltarune/deltarunevita")) {
        REPORT_APPEND("- pasta deltarunevita/ ausente\n");
        anyMissing = true;
    }
    else {
        // Check if there is an old flat data.win (v0.64 indicator)
        SceIoStat oldStat;
        if (sceIoGetstat("ux0:data/voidstranger/data.win", &oldStat) >= 0) {
            log_line("STARTUP_FATAL=old_data_win_detected");
            show_startup_error_and_exit(
                "Detectamos arquivos de dados de uma versao anterior do DELTARUNE (como v0.64).\n\n"
                "Esta versao do VPK exige dados atualizados organizados em pastas por capitulo.",
                "ux0:data/voidstranger/data.win"
            );
        }

        // 3. Check chapter0 directory and data.win (launcher)
        bool has_chapter0 = is_file_valid("ux0:data/voidstranger/chapter0/data.win") ||
                            is_file_valid("app0:assets/chapter0/data.win");
        if (!has_chapter0) {
            REPORT_APPEND("- arquivo chapter0/data.win (launcher) ausente no console e no VPK\n");
            anyMissing = true;
        }

        // 4. Check music directory
        if (!is_directory("ux0:data/voidstranger/music")) {
            REPORT_APPEND("- pasta music/ (musicas) ausente\n");
            anyMissing = true;
        }

        // 5. Check sounds directory
        if (!is_directory("ux0:data/voidstranger/sounds")) {
            REPORT_APPEND("- pasta sounds/ (efeitos) ausente\n");
            anyMissing = true;
        }

        // 6. Check ui directory
        if (!is_directory("ux0:data/voidstranger/ui")) {
            REPORT_APPEND("- pasta ui/ (interface) ausente\n");
            anyMissing = true;
        }

        // 7. Check active chapter's files (if not chapter 0)
        if (active_chapter > 0) {
            char chDir[192];
            char chPath[192];
            snprintf(chDir, sizeof(chDir), "ux0:data/voidstranger/chapter%d", active_chapter);
            snprintf(chPath, sizeof(chPath), "ux0:data/voidstranger/chapter%d/data.win", active_chapter);

            if (!is_directory(chDir)) {
                char chLine[64];
                snprintf(chLine, sizeof(chLine), "- pasta chapter%d/ ausente\n", active_chapter);
                REPORT_APPEND(chLine);
                anyMissing = true;
            } else if (!is_file_valid(chPath)) {
                char chLine[64];
                snprintf(chLine, sizeof(chLine), "- arquivo chapter%d/data.win ausente ou corrompido\n", active_chapter);
                REPORT_APPEND(chLine);
                anyMissing = true;
            }
        }
    }

    #undef REPORT_APPEND

    if (anyMissing) {
        log_line("STARTUP_FATAL=data_structure_incomplete");
        show_startup_error_and_exit(missing, "ux0:data/voidstranger/");
    }
}

static void log_puzzle_diagnostics(Runner* runner) {
    if (dev_log_fd < 0 || runner == NULL || runner->currentRoom == NULL) return;
    const char* room = runner->currentRoom->name;
    if (room == NULL || strstr(room, "darkbulb") == NULL) return;
    char line[320];
    snprintf(line, sizeof(line), "PUZZLE_STATE room=%s frame=%u instances=%d",
             room, runner->frameCount, (int)arrlen(runner->instances));
    dev_log_write(line);
    for (ptrdiff_t i = 0; i < arrlen(runner->instances); ++i) {
        Instance* inst = runner->instances[i];
        if (inst == NULL || !inst->active || inst->objectIndex < 0 ||
            (uint32_t)inst->objectIndex >= runner->dataWin->objt.count) continue;
        const char* objectName = runner->dataWin->objt.objects[inst->objectIndex].name;
        if (objectName == NULL || (strcmp(objectName, "obj_shapepuzzle") != 0 &&
                                  strcmp(objectName, "obj_shapepuzzlepiece") != 0)) continue;
        const char* spriteName = "<none>";
        if (inst->spriteIndex >= 0 && (uint32_t)inst->spriteIndex < runner->dataWin->sprt.count &&
            runner->dataWin->sprt.sprites[inst->spriteIndex].name != NULL)
            spriteName = runner->dataWin->sprt.sprites[inst->spriteIndex].name;
        InstanceBBox bbox = Collision_computeBBox(runner, inst);
        snprintf(line, sizeof(line),
                 "PUZZLE_INSTANCE id=%u obj=%s sprite=%d:%s sub=%.3f pos=%.3f,%.3f scale=%.3f,%.3f rot=%.3f mask=%d bbox=%d:%.3f,%.3f,%.3f,%.3f",
                 inst->instanceId, objectName, inst->spriteIndex, spriteName,
                 (double)inst->imageIndex, (double)inst->x, (double)inst->y,
                 (double)inst->imageXscale, (double)inst->imageYscale,
                 (double)inst->imageAngle, inst->maskIndex, bbox.valid ? 1 : 0,
                 (double)bbox.left, (double)bbox.top, (double)bbox.right, (double)bbox.bottom);
        dev_log_write(line);
    }
    const char* surfaceDiagnostic = VMBuiltins_getShapePuzzleDiagnostic();
    if (surfaceDiagnostic != NULL) dev_log_write(surfaceDiagnostic);
}

typedef struct { GLfloat u, v, x, y; } LoadingVertex;

typedef struct {
    int loadingW, loadingH;
    int completeW, completeH;
    const char* code;
} LoadingLanguageInfo;

static const LoadingLanguageInfo kLoadingLanguageInfo[LOADING_LANG_COUNT] = {
    {368, 47, 341, 47, "en"},
    {444, 47, 470, 47, "ptbr"},
    {406, 47, 319, 47, "es"},
    {428, 42, 463, 47, "it"},
    {412, 47, 401, 47, "tr"},
};

static GLuint load_loading_texture(const char* path) {
    SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
    if (fd < 0) return 0;
    SceOff size = sceIoLseek(fd, 0, SCE_SEEK_END);
    sceIoLseek(fd, 0, SCE_SEEK_SET);
    if (size <= 0) { sceIoClose(fd); return 0; }
    unsigned char* data = (unsigned char*)malloc((size_t)size);
    if (data == NULL || sceIoRead(fd, data, (unsigned int)size) != size) {
        free(data); sceIoClose(fd); return 0;
    }
    sceIoClose(fd);
    int w = 0, h = 0, channels = 0;
    unsigned char* pixels = stbi_load_from_memory(data, (int)size, &w, &h, &channels, 4);
    free(data);
    if (pixels == NULL) return 0;
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    stbi_image_free(pixels);
    return texture;
}

static GLuint create_solid_texture_rgba(unsigned char r, unsigned char g,
                                        unsigned char b, unsigned char a) {
    const unsigned char pixel[4] = { r, g, b, a };
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    return texture;
}

static void loading_screen_set_language(const char* language) {
    loading_language = LOADING_LANG_ENGLISH;
    if (language == NULL) return;
    if (strcmp(language, "Portuguese-BR") == 0) loading_language = LOADING_LANG_PTBR;
    else if (strcmp(language, "Spanish") == 0) loading_language = LOADING_LANG_SPANISH;
    else if (strcmp(language, "Italian") == 0) loading_language = LOADING_LANG_ITALIAN;
    else if (strcmp(language, "Turkish") == 0) loading_language = LOADING_LANG_TURKISH;
}

static void loading_screen_init(void) {
    if (loading_sprite_atlas != 0) return;
    loading_sprite_atlas = load_loading_texture("app0:assets/loading/loading_sprites.png");
    loading_digits_texture = load_loading_texture("app0:assets/loading/loading_digits.png");
    char path[128];
    for (int i = 0; i < LOADING_LANG_COUNT; ++i) {
        snprintf(path, sizeof(path), "app0:assets/loading/loading_%s.png", kLoadingLanguageInfo[i].code);
        loading_prefix_textures[i] = load_loading_texture(path);
        snprintf(path, sizeof(path), "app0:assets/loading/complete_%s.png", kLoadingLanguageInfo[i].code);
        loading_complete_textures[i] = load_loading_texture(path);
    }
    char line[192];
    snprintf(line, sizeof(line), "LOADING_ASSETS sprite=%u digits=%u language=%d font_px=51",
             loading_sprite_atlas, loading_digits_texture, loading_language);
    log_line(line);
}

static void loading_screen_shutdown(void) {
    if (loading_sprite_atlas != 0) glDeleteTextures(1, &loading_sprite_atlas);
    if (loading_digits_texture != 0) glDeleteTextures(1, &loading_digits_texture);
    for (int i = 0; i < LOADING_LANG_COUNT; ++i) {
        if (loading_prefix_textures[i] != 0) glDeleteTextures(1, &loading_prefix_textures[i]);
        if (loading_complete_textures[i] != 0) glDeleteTextures(1, &loading_complete_textures[i]);
        loading_prefix_textures[i] = 0;
        loading_complete_textures[i] = 0;
    }
    loading_sprite_atlas = 0;
    loading_digits_texture = 0;
}

static void draw_loading_region(GLuint texture, int texW, int texH,
                                float srcX, float srcY, float srcW, float srcH,
                                float left, float top, float right, float bottom,
                                float alpha) {
    if (texture == 0 || texW <= 0 || texH <= 0 || alpha <= 0.0f) return;
    float x0 = left / 480.0f - 1.0f, x1 = right / 480.0f - 1.0f;
    float y0 = 1.0f - top / 272.0f, y1 = 1.0f - bottom / 272.0f;
    float u0 = srcX / (float)texW, u1 = (srcX + srcW) / (float)texW;
    float v0 = srcY / (float)texH, v1 = (srcY + srcH) / (float)texH;
    const LoadingVertex vertices[4] = {
        {u0, v0, x0, y0}, {u1, v0, x1, y0},
        {u1, v1, x1, y1}, {u0, v1, x0, y1}
    };
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 1.0f, 1.0f, alpha);
    glBindTexture(GL_TEXTURE_2D, texture);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, sizeof(LoadingVertex), &vertices[0].u);
    glVertexPointer(2, GL_FLOAT, sizeof(LoadingVertex), &vertices[0].x);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

static void draw_loading_sprite(int srcX, int srcY, int srcW, int srcH,
                                float centerX, float centerY, float scale, float alpha) {
    float w = srcW * scale, h = srcH * scale;
    draw_loading_region(loading_sprite_atlas, 256, 64,
                        (float)srcX, (float)srcY, (float)srcW, (float)srcH,
                        centerX - w * 0.5f, centerY - h * 0.5f,
                        centerX + w * 0.5f, centerY + h * 0.5f, alpha);
}

static int loading_percent_from_ratio(float ratio) {
    float clampedRatio = ratio < 0.0f ? 0.0f : (ratio > 1.0f ? 1.0f : ratio);
    if (clampedRatio >= 1.0f) return 100;
    return (int)floorf(clampedRatio * 100.0f);
}

static int loading_digit_index(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    return ch == '%' ? 10 : -1;
}

static void draw_loading_progress_text(float ratio, float alpha) {
    const LoadingLanguageInfo* info = &kLoadingLanguageInfo[loading_language];
    int percent = loading_percent_from_ratio(ratio);
    char number[8];
    snprintf(number, sizeof(number), "%d%%", percent);
    int chars = (int)strlen(number);
    const float digitW = 40.0f, digitH = 64.0f, gap = 8.0f;
    float totalW = (float)info->loadingW + gap + digitW * chars;
    float left = (960.0f - totalW) * 0.5f;
    float top = 382.0f;
    float labelTop = top + (64.0f - info->loadingH) * 0.5f;
    draw_loading_region(loading_prefix_textures[loading_language], info->loadingW, info->loadingH,
                        0, 0, (float)info->loadingW, (float)info->loadingH,
                        left, labelTop, left + info->loadingW, labelTop + info->loadingH, alpha);
    float x = left + info->loadingW + gap;
    for (int i = 0; i < chars; ++i) {
        int index = loading_digit_index(number[i]);
        if (index >= 0) {
            draw_loading_region(loading_digits_texture, 440, 64,
                                (float)(index * 40), 0, 40, 64,
                                x, top, x + digitW, top + digitH, alpha);
        }
        x += digitW;
    }
}

static void draw_loading_complete_text(float alpha) {
    const LoadingLanguageInfo* info = &kLoadingLanguageInfo[loading_language];
    float left = (960.0f - info->completeW) * 0.5f;
    float top = 382.0f + (64.0f - info->completeH) * 0.5f;
    draw_loading_region(loading_complete_textures[loading_language], info->completeW, info->completeH,
                        0, 0, (float)info->completeW, (float)info->completeH,
                        left, top, left + info->completeW, top + info->completeH, alpha);
}

static void dev_log_write(const char* text) {
    if (dev_log_fd < 0 || text == NULL) return;
    size_t text_len = strlen(text);
    size_t needed = dev_log_buffer_size + text_len + 1;
    if (needed > dev_log_buffer_capacity) {
        size_t capacity = dev_log_buffer_capacity > 0 ? dev_log_buffer_capacity : 16384;
        while (capacity < needed) capacity *= 2;
        char* grown = (char*)realloc(dev_log_buffer, capacity);
        if (grown == NULL) return;
        dev_log_buffer = grown;
        dev_log_buffer_capacity = capacity;
    }
    memcpy(dev_log_buffer + dev_log_buffer_size, text, text_len);
    dev_log_buffer_size += text_len;
    dev_log_buffer[dev_log_buffer_size++] = '\n';
}

static void dev_log_stop(void) {
    if (dev_log_fd < 0) return;
    dev_log_write("DEV_CAPTURE=end");
    if (dev_log_buffer != NULL && dev_log_buffer_size > 0)
        sceIoWrite(dev_log_fd, dev_log_buffer, dev_log_buffer_size);
    sceIoClose(dev_log_fd);
    dev_log_fd = -1;
    free(dev_log_buffer);
    dev_log_buffer = NULL;
    dev_log_buffer_size = 0;
    dev_log_buffer_capacity = 0;
}

static unsigned int dev_log_next_sequence(const char* chapterDir) {
    unsigned int highest = 0;
    SceUID dir = sceIoDopen(chapterDir);
    if (dir < 0) return 1;
    SceIoDirent entry;
    memset(&entry, 0, sizeof(entry));
    while (sceIoDread(dir, &entry) > 0) {
        const char* name = entry.d_name;
        unsigned int number = 0;
        size_t i = 0;
        while (name[i] >= '0' && name[i] <= '9') {
            number = number * 10U + (unsigned int)(name[i] - '0');
            i++;
        }
        if (i > 0 && name[i] == '_' && number > highest) highest = number;
        memset(&entry, 0, sizeof(entry));
    }
    sceIoDclose(dir);
    return highest + 1U;
}

static void dev_log_start(const char* room) {
    dev_log_stop();
    dev_log_buffer_size = 0;
    sceIoMkdir(DEV_LOG_ROOT, 0777);
    char chapterDir[192];
    snprintf(chapterDir, sizeof(chapterDir), DEV_LOG_ROOT "/chapter %d", g_vitaActiveChapter);
    sceIoMkdir(chapterDir, 0777);
    unsigned int sequence = dev_log_next_sequence(chapterDir);
    char safeRoom[64];
    size_t j = 0;
    const char* source = room != NULL ? room : "unknown";
    for (size_t i = 0; source[i] != '\0' && j + 1 < sizeof(safeRoom); ++i) {
        char c = source[i];
        safeRoom[j++] = ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                         (c >= '0' && c <= '9') || c == '_' || c == '-') ? c : '_';
    }
    safeRoom[j] = '\0';
    SceDateTime now;
    memset(&now, 0, sizeof(now));
    sceRtcGetCurrentClockLocalTime(&now);
    char path[384];
    snprintf(path, sizeof(path), "%s/%u_%s_%04d%02d%02d-%02d%02d%02d.log",
             chapterDir, sequence, safeRoom,
             now.year, now.month, now.day, now.hour, now.minute, now.second);
    dev_log_fd = sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
    if (dev_log_fd >= 0) {
        dev_log_write("DELTARUNE_VITA_DEV_CAPTURE=1");
        dev_log_write("BUILD_VERSION=" PORT_BUILD_VERSION);
        char line[512];
        snprintf(line, sizeof(line), "CHAPTER=%d LOG_SEQUENCE=%u START_ROOM=%s FILE=%s",
                 g_vitaActiveChapter, sequence, source, path);
        dev_log_write(line);
    }
}

static bool place_contact_sprite(const char* name) {
    if (name == NULL) return false;
    // Vessel creator assets. Keep this deliberately narrow: preloading every
    // dialogue "face" or every battle "body" would fill CDRAM before gameplay.
    return strncmp(name, "spr_face_b", 10) == 0 ||
           strcmp(name, "spr_face_tbody") == 0 ||
           strstr(name, "contact") != NULL || strstr(name, "vessel") != NULL;
}

static void preload_place_contact_atlases(DataWin* dw, GLLegacyRenderer* gl) {
    if (dw == NULL || gl == NULL) return;
    bool* seen = (bool*)calloc(dw->txtr.count, sizeof(bool));
    if (seen == NULL) return;
    uint32_t pages = 0;
    for (uint32_t i = 0; i < dw->sprt.count; ++i) {
        Sprite* sprite = &dw->sprt.sprites[i];
        if (!sprite->present || !place_contact_sprite(sprite->name)) continue;
        for (uint32_t frame = 0; frame < sprite->textureCount; ++frame) {
            int32_t tpagIndex = sprite->tpagIndices[frame];
            if (tpagIndex < 0 || (uint32_t)tpagIndex >= dw->tpag.count) continue;
            int32_t page = dw->tpag.items[tpagIndex].texturePageId;
            if (page < 0 || (uint32_t)page >= dw->txtr.count || seen[page]) continue;
            seen[page] = true;
            if (GLLegacyRenderer_ensureTextureLoaded(gl, (uint32_t)page)) pages++;
        }
    }
    char line[96];
    snprintf(line, sizeof(line), "PLACE_CONTACT_PRELOAD=complete pages=%u", pages);
    log_line(line);
    free(seen);
}

static void preload_chapter3_slide_atlases(DataWin* dw, GLLegacyRenderer* gl) {
    static const char* spriteNames[] = {
        "spr_krisd_slide", "spr_krisd_slide_heart", "spr_susie_slide",
        "spr_ralsei_slide", "spr_slidedust"
    };
    uint32_t loadedPages[16];
    uint32_t loadedCount = 0;
    for (uint32_t n = 0; n < sizeof(spriteNames) / sizeof(spriteNames[0]); ++n) {
        for (uint32_t i = 0; i < dw->sprt.count; ++i) {
            Sprite* sprite = &dw->sprt.sprites[i];
            if (!sprite->present || sprite->name == nullptr ||
                strcmp(sprite->name, spriteNames[n]) != 0) continue;
            for (uint32_t f = 0; f < sprite->textureCount; ++f) {
                int32_t tpagIndex = sprite->tpagIndices[f];
                if (tpagIndex < 0 || (uint32_t)tpagIndex >= dw->tpag.count) continue;
                int32_t page = dw->tpag.items[tpagIndex].texturePageId;
                if (page < 0 || (uint32_t)page >= gl->textureCount) continue;
                bool duplicate = false;
                for (uint32_t p = 0; p < loadedCount; ++p)
                    if (loadedPages[p] == (uint32_t)page) duplicate = true;
                if (duplicate) continue;
                if (GLLegacyRenderer_ensureTextureLoaded(gl, (uint32_t)page)) {
                    if (loadedCount < 16) loadedPages[loadedCount++] = (uint32_t)page;
                }
            }
            break;
        }
    }
    char line[96];
    snprintf(line, sizeof(line), "CH3_SLIDE_PRELOAD=complete pages=%u pinned=0", loadedCount);
    log_line(line);
}

static void preload_chapter3_finale_atlases(DataWin* dw, GLLegacyRenderer* gl) {
    static const char* spriteNames[] = {
        "spr_funnytext_amazing_01", "spr_funnytext_tears"
    };
    uint32_t pages[8];
    uint32_t pageCount = 0;

    // Include the default sprite used by every confetti instance.
    int32_t confettiSprite = -1;
    for (uint32_t i = 0; i < dw->objt.count; ++i) {
        GameObject* object = &dw->objt.objects[i];
        if (object->present && object->name != NULL &&
            strcmp(object->name, "obj_confetti_overworld") == 0) {
            confettiSprite = object->spriteId;
            break;
        }
    }

    for (uint32_t i = 0; i < dw->sprt.count; ++i) {
        Sprite* sprite = &dw->sprt.sprites[i];
        if (!sprite->present) continue;
        bool wanted = (int32_t)i == confettiSprite;
        for (uint32_t n = 0; !wanted && n < sizeof(spriteNames) / sizeof(spriteNames[0]); ++n)
            wanted = sprite->name != NULL && strcmp(sprite->name, spriteNames[n]) == 0;
        if (!wanted) continue;

        for (uint32_t f = 0; f < sprite->textureCount; ++f) {
            int32_t tpagIndex = sprite->tpagIndices[f];
            if (tpagIndex < 0 || (uint32_t)tpagIndex >= dw->tpag.count) continue;
            int32_t page = dw->tpag.items[tpagIndex].texturePageId;
            if (page < 0 || (uint32_t)page >= gl->textureCount) continue;
            bool duplicate = false;
            for (uint32_t p = 0; p < pageCount; ++p)
                if (pages[p] == (uint32_t)page) duplicate = true;
            if (duplicate) continue;
            if (GLLegacyRenderer_ensureTextureLoaded(gl, (uint32_t)page)) {
                // This event caused a single 554 ms upload immediately before
                // the video. Its tiny page set is safe to retain; unlike the
                // former five-page slide pin it does not crowd out room art.
                if (pageCount < 8) pages[pageCount++] = (uint32_t)page;
            }
        }
    }

    char line[96];
    snprintf(line, sizeof(line), "CH3_FINALE_PRELOAD=complete pages=%u residency=normal", pageCount);
    log_line(line);
}

// The Vita defaults the main thread to a 256 KiB stack. The first fixed-pipeline
// shader generated by vitaGL/ShaccCg needs substantially more and corrupted the
// return stack before the first draw. Keep this symbol global: the Vita runtime
// reads it while creating the main thread.
unsigned int sceUserMainThreadStackSize = 4 * 1024 * 1024;

typedef struct { uint32_t mask; int key; } KeyMap;
static const KeyMap KEY_MAP[] = {
    {SCE_CTRL_UP, VK_UP}, {SCE_CTRL_DOWN, VK_DOWN},
    {SCE_CTRL_LEFT, VK_LEFT}, {SCE_CTRL_RIGHT, VK_RIGHT},
    {SCE_CTRL_CROSS, 'Z'}, {SCE_CTRL_CIRCLE, 'X'},
    {SCE_CTRL_SQUARE, 'X'}, {SCE_CTRL_TRIANGLE, 'C'},
    {SCE_CTRL_START, VK_ENTER}, {SCE_CTRL_SELECT, VK_ESCAPE},
    {SCE_CTRL_LTRIGGER, VK_PAGEDOWN},
    {SCE_CTRL_RTRIGGER, VK_PAGEUP}
};

static void log_line(const char *text) {
    if (text == NULL) return;
    // Essential probe logging is always enabled, but uses one persistent FD so
    // room/perf/settings diagnostics do not repeatedly open the memory card.
    if (probe_log_fd < 0)
        probe_log_fd = sceIoOpen(LOG_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0666);
    if (probe_log_fd >= 0) {
        sceIoWrite(probe_log_fd, text, strlen(text));
        sceIoWrite(probe_log_fd, "\n", 1);
    }
}

static int config_devmode_enabled(void) {
    FILE* config = fopen("ux0:data/voidstranger/config.ini", "rb");
    if (config == NULL) return 0;
    char line[192];
    int enabled = 0;
    while (fgets(line, sizeof(line), config) != NULL) {
        char* key = line;
        while (*key == ' ' || *key == '\t') key++;
        if (strncmp(key, "devmode=", 8) == 0) {
            enabled = atoi(key + 8) == 1;
            break;
        }
    }
    fclose(config);
    return enabled;
}

static int config_modern_gl_enabled(void) {
    FILE* config = fopen("ux0:data/voidstranger/config.ini", "rb");
    if (config == NULL) return 0;
    char line[192];
    int enabled = 0;
    while (fgets(line, sizeof(line), config) != NULL) {
        char* key = line;
        while (*key == ' ' || *key == '\t') key++;
        if (strncmp(key, "renderer=", 9) == 0) {
            enabled = strncmp(key + 9, "modern-gl", 9) == 0;
            break;
        }
    }
    fclose(config);
    return enabled;
}

static void log_memory_snapshot(const char* phase, GLLegacyRenderer* renderer) {
    SceKernelFreeMemorySizeInfo systemMemory;
    memset(&systemMemory, 0, sizeof(systemMemory));
    systemMemory.size = sizeof(systemMemory);
    sceKernelGetFreeMemorySize(&systemMemory);

    VitaVideoMemoryStats videoMemory = {0};
    VitaVideo_getMemoryStats(&videoMemory);
    size_t vramTotal = vglMemTotal(VGL_MEM_VRAM);
    size_t vramFree = vglMemFree(VGL_MEM_VRAM);
    size_t ramTotal = vglMemTotal(VGL_MEM_RAM);
    size_t ramFree = vglMemFree(VGL_MEM_RAM);
    size_t phyTotal = vglMemTotal(VGL_MEM_SLOW);
    size_t phyFree = vglMemFree(VGL_MEM_SLOW);
    size_t budgetTotal = vglMemTotal(VGL_MEM_BUDGET);
    size_t budgetFree = vglMemFree(VGL_MEM_BUDGET);
    uint64_t atlasBytes = renderer != NULL ? renderer->residentTextureBytes : 0;
    uint64_t surfaceBytes = 0;
    uint64_t retiredTextureBytes = 0;
    uint32_t retiredTextureCount = 0;
    uint32_t activeSurfaces = 0;
    if (renderer != NULL) {
        for (uint32_t i = 0; i < renderer->surfaceCount; ++i) {
            if (renderer->surfaces[i] == 0 || renderer->surfaceTexture[i] == 0) continue;
            activeSurfaces++;
            surfaceBytes += (uint64_t)renderer->surfaceWidth[i] *
                            (uint64_t)renderer->surfaceHeight[i] * 4ULL;
        }
        for (int i = 0; i < VITA_RETIRED_TEXTURE_SLOTS; ++i) {
            if (renderer->retiredTextures[i] == 0) continue;
            retiredTextureCount++;
            retiredTextureBytes += renderer->retiredTextureBytes[i];
        }
    }

    char line[768];
    snprintf(line, sizeof(line),
             "MEMORY phase=%s atlas=%llu surfaces=%llu surface_count=%u retired=%llu retired_count=%u vgl_vram_used=%llu vgl_vram_free=%llu vgl_vram_total=%llu vgl_ram_used=%llu vgl_ram_free=%llu vgl_ram_total=%llu vgl_phy_used=%llu vgl_phy_free=%llu vgl_phy_total=%llu vgl_budget_used=%llu vgl_budget_free=%llu vgl_budget_total=%llu video_cpu=%llu video_cpu_peak=%llu video_gpu=%llu video_gpu_peak=%llu system_user_free=%u system_cdram_free=%u system_phy_free=%u",
             phase != NULL ? phase : "unknown",
             (unsigned long long)atlasBytes,
             (unsigned long long)surfaceBytes, activeSurfaces,
             (unsigned long long)retiredTextureBytes, retiredTextureCount,
             (unsigned long long)(vramTotal >= vramFree ? vramTotal - vramFree : 0),
             (unsigned long long)vramFree, (unsigned long long)vramTotal,
             (unsigned long long)(ramTotal >= ramFree ? ramTotal - ramFree : 0),
             (unsigned long long)ramFree, (unsigned long long)ramTotal,
             (unsigned long long)(phyTotal >= phyFree ? phyTotal - phyFree : 0),
             (unsigned long long)phyFree, (unsigned long long)phyTotal,
             (unsigned long long)(budgetTotal >= budgetFree ? budgetTotal - budgetFree : 0),
             (unsigned long long)budgetFree, (unsigned long long)budgetTotal,
             (unsigned long long)videoMemory.cpuBytes,
             (unsigned long long)videoMemory.cpuPeakBytes,
             (unsigned long long)videoMemory.gpuBytes,
             (unsigned long long)videoMemory.gpuPeakBytes,
             systemMemory.size_user, systemMemory.size_cdram,
             systemMemory.size_phycont);
    log_line(line);
}

void VitaProbe_logLine(const char *text) {
    log_line(text);
}

// Tiny always-on compatibility trace for isolated room-specific regressions.
// Unlike butterscotch-probe.log this is never called from the per-frame hot
// path except for the first three rm_e_022 steps.
void VitaCompat_logLine(const char *text) {
    if (text == NULL) return;
    SceUID fd = sceIoOpen(COMPAT_LOG_PATH,
                          SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0666);
    if (fd < 0) return;
    sceIoWrite(fd, text, strlen(text));
    sceIoWrite(fd, "\n", 1);
    sceIoClose(fd);
}

static void progress(const char *chunk, int index, int total, DataWin *dw, void *user) {
    (void)dw; (void)user;
    char line[96];
    snprintf(line, sizeof(line), "LOAD chunk=%.4s %d/%d", chunk, index + 1, total);
    log_line(line);
    sceKernelDelayThread(1000);
}

static void loading_begin_frame(void) {
    glViewport(0, 0, 960, 544);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

static void loading_end_frame(void) {
    vglSwapBuffers(GL_FALSE);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static float loading_cell_x(int cell) { return 128.0f + (float)cell * 64.0f; }

static void draw_loading_floor(int fullFloors, bool targetVisible, float alpha) {
    const float y = 248.0f;
    draw_loading_sprite(0, 0, 16, 16, loading_cell_x(0), y, 4.0f, alpha);
    draw_loading_sprite(32, 0, 16, 16, loading_cell_x(0), y + 64.0f, 4.0f, alpha);
    for (int i = 1; i <= fullFloors && i <= LOADING_FLOOR_COUNT; ++i) {
        draw_loading_sprite(16, 0, 16, 16, loading_cell_x(i), y, 4.0f, alpha);
        draw_loading_sprite(32, 0, 16, 16, loading_cell_x(i), y + 64.0f, 4.0f, alpha);
    }
    if (targetVisible && fullFloors < LOADING_FLOOR_COUNT) {
        int i = fullFloors + 1;
        draw_loading_sprite(16, 0, 16, 16, loading_cell_x(i), y, 4.0f, alpha);
        draw_loading_sprite(32, 0, 16, 16, loading_cell_x(i), y + 64.0f, 4.0f, alpha);
    }
}

static void draw_loading_swipe(int frame, float playerX, float playerY, float alpha) {
    frame %= 10;
    int sx = frame < 8 ? frame * 32 : (frame - 8) * 32;
    int sy = frame < 8 ? 32 : 48;
    draw_loading_sprite(sx, sy, 32, 16, playerX + 32.0f, playerY + 32.0f, 4.0f, alpha);
}

static void loading_animation_reset(void) {
    memset(&loading_animation, 0, sizeof(loading_animation));
}

static void loading_play_place_sound(void) {
    if (loading_audio_system == NULL || loading_audio_system->vtable == NULL ||
        loading_audio_system->vtable->playSound == NULL) return;
    loading_audio_system->vtable->playSound(loading_audio_system,
        VOIDSTRANGER_LOADING_PLACE_SOUND_ID, 1, false);
}

static void loading_play_fall_sound(void) {
    if (loading_audio_system == NULL || loading_audio_system->vtable == NULL ||
        loading_audio_system->vtable->playSound == NULL) return;
    loading_audio_system->vtable->playSound(loading_audio_system,
        VOIDSTRANGER_LOADING_FALL_SOUND_ID, 1, false);
}

static int32_t loading_play_stairs_sound(void) {
    if (loading_audio_system == NULL || loading_audio_system->vtable == NULL ||
        loading_audio_system->vtable->playSound == NULL) return -1;
    return loading_audio_system->vtable->playSound(loading_audio_system,
        VOIDSTRANGER_LOADING_STAIRS_SOUND_ID, 1, false);
}

static void loading_wait_for_sound(int32_t instanceId) {
    if (instanceId < 0 || loading_audio_system == NULL ||
        loading_audio_system->vtable == NULL ||
        loading_audio_system->vtable->isPlaying == NULL) return;
    const uint64_t timeoutUs = 3000000ULL;
    uint64_t startedUs = sceKernelGetProcessTimeWide();
    while (loading_audio_system->vtable->isPlaying(loading_audio_system, instanceId) &&
           sceKernelGetProcessTimeWide() - startedUs < timeoutUs) {
        sceKernelDelayThread(1000);
    }
}

static void draw_loading_intro_frame(float alpha) {
    loading_begin_frame();
    draw_loading_floor(0, false, alpha);
    loading_end_frame();
}

static void play_loading_intro_fade(void) {
    uint64_t startedUs = sceKernelGetProcessTimeWide();
    for (;;) {
        uint64_t elapsedUs = sceKernelGetProcessTimeWide() - startedUs;
        draw_loading_intro_frame(loading_intro_fade_alpha(elapsedUs));
        if (elapsedUs >= LOADING_INTRO_FADE_US) break;
        sceKernelDelayThread(16667);
    }
}

static void loading_set_progress(float ratio, int labelType) {
    float clamped = ratio < 0.0f ? 0.0f : (ratio > 1.0f ? 1.0f : ratio);
    loading_animation.ratio = clamped;
    loading_animation.labelType = labelType;
    int percent = loading_percent_from_ratio(clamped);
    int target = loading_target_cell_for_percent(percent, LOADING_FLOOR_COUNT);
    if (target > loading_animation.targetCell) loading_animation.targetCell = target;
}

static void loading_start_next_step(uint64_t nowUs) {
    if (loading_animation.stepActive ||
        loading_animation.currentCell >= loading_animation.targetCell ||
        loading_animation.currentCell >= LOADING_FLOOR_COUNT) return;
    loading_animation.stepActive = true;
    loading_animation.stepStartedUs = nowUs;
    loading_play_place_sound();
}

static bool loading_draw_animation_frame(bool showProgress) {
    uint64_t nowUs = sceKernelGetProcessTimeWide();
    if (loading_animation.stepActive) {
        uint64_t elapsed = nowUs - loading_animation.stepStartedUs;
        if (loading_step_phase(elapsed) == LOADING_STEP_DONE) {
            if (loading_animation.currentCell < LOADING_FLOOR_COUNT)
                loading_animation.currentCell++;
            loading_animation.stepActive = false;
        }
    }
    loading_start_next_step(nowUs);
    if (loading_animation.labelType == 0) {
        uint64_t visualStepElapsed = loading_animation.stepActive ?
            nowUs - loading_animation.stepStartedUs : 0ULL;
        loading_animation.ratio = loading_visual_ratio(
            loading_animation.currentCell, loading_animation.stepActive,
            visualStepElapsed, LOADING_FLOOR_COUNT);
    }

    loading_begin_frame();
    const float playerY = 248.0f;
    int cell = loading_animation.currentCell;
    if (cell < 0) cell = 0;
    if (cell > LOADING_FLOOR_COUNT) cell = LOADING_FLOOR_COUNT;

    if (!loading_animation.stepActive || cell >= LOADING_FLOOR_COUNT) {
        draw_loading_floor(cell, false, 1.0f);
        draw_loading_sprite(48, 0, 16, 16, loading_cell_x(cell), playerY, 4.0f, 1.0f);
    } else {
        uint64_t elapsed = nowUs - loading_animation.stepStartedUs;
        LoadingStepPhase phase = loading_step_phase(elapsed);
        float playerX = loading_cell_x(cell);
        float targetX = loading_cell_x(cell + 1);
        if (phase == LOADING_STEP_PLACE) {
            bool targetVisible = elapsed >= (LOADING_PLACE_DURATION_US * 3ULL) / 5ULL;
            draw_loading_floor(cell, targetVisible, 1.0f);
            int actionFrame = (int)(elapsed / 16667ULL);
            draw_loading_sprite(80 + (actionFrame & 1) * 16, 0, 16, 16,
                                playerX, playerY, 4.0f, 1.0f);
            draw_loading_sprite(144, 0, 16, 16, targetX, playerY, 4.0f, 1.0f);
            draw_loading_swipe(actionFrame, playerX, playerY, 1.0f);
            int sparkleFrame = (actionFrame * 2) & 7;
            draw_loading_sprite(sparkleFrame * 16, 16, 16, 16,
                                targetX, playerY, 4.0f, 1.0f);
        } else {
            draw_loading_floor(cell + 1, false, 1.0f);
            float t = loading_walk_progress(elapsed);
            playerX += 64.0f * t;
            int walkFrame = ((int)((elapsed - LOADING_PLACE_DURATION_US) / 70000ULL)) & 1;
            draw_loading_sprite(48 + walkFrame * 16, 0, 16, 16,
                                playerX, playerY, 4.0f, 1.0f);
        }
    }
    if (showProgress) draw_loading_progress_text(loading_animation.ratio, 1.0f);
    loading_end_frame();
    return !loading_animation.stepActive &&
           loading_animation.currentCell >= loading_animation.targetCell;
}

static void loading_run_to_target(bool showProgress) {
    while (!loading_draw_animation_frame(showProgress))
        sceKernelDelayThread(16667);
}

static void draw_loading_screen(bool showProgress, float ratio, int labelType) {
    loading_set_progress(ratio, labelType);
    loading_draw_animation_frame(showProgress);
}

static void texture_loading_progress(uint32_t current, uint32_t total, void *user) {
    (void)user;
    float ratio = total > 0 ? (float)current / (float)total : 1.0f;
    // Texture preparation is followed by the chapter battle core preload.
    // Keep the displayed percentage below 100 until that work is complete.
    if (ratio >= 1.0f) ratio = 0.99f;
    int oldTarget = loading_animation.targetCell;
    loading_set_progress(ratio, 1);
    if (loading_animation.targetCell > oldTarget)
        loading_run_to_target(true);
    else
        loading_draw_animation_frame(true);
}

static void texture_existing_progress(uint32_t current, uint32_t total, void *user) {
    /* Existing cache validation is usually much faster than the animation.
       Never expose that 0 -> 100 I/O jump to the player: Gray's place/walk
       sequence owns the visible percentage in this mode. */
    (void)current;
    (void)total;
    (void)user;
    loading_animation.labelType = 0;
    loading_animation.targetCell = LOADING_FLOOR_COUNT;
    loading_draw_animation_frame(true);
}

static void loading_finish_progress_animation(int labelType) {
    loading_animation.targetCell = LOADING_FLOOR_COUNT;
    if (labelType == 0) {
        loading_animation.labelType = 0;
        loading_run_to_target(true);
        loading_animation.ratio = 1.0f;
    } else {
        loading_set_progress(1.0f, labelType);
        loading_run_to_target(true);
    }
}

static void draw_loading_final_frame(uint64_t elapsedUs) {
    const uint64_t frontUs = 120000ULL;
    const uint64_t edgeUs = (uint64_t)LOADING_STATE23_TOTAL_FRAMES * 16667ULL;
    const uint64_t delayUs = (uint64_t)LOADING_FALL_DELAY_FRAMES * 16667ULL;
    const uint64_t fallUs = (uint64_t)LOADING_FALL_GAME_FRAMES * 16667ULL;
    const uint64_t completeStart = frontUs + edgeUs + delayUs + fallUs;
    const uint64_t fadeStart = completeStart + LOADING_COMPLETE_HOLD_US;
    const uint64_t finish = fadeStart + LOADING_FADEOUT_US;
    float alpha = 1.0f;
    if (elapsedUs >= fadeStart) {
        alpha = 1.0f - (float)(elapsedUs - fadeStart) / (float)LOADING_FADEOUT_US;
        if (alpha < 0.0f) alpha = 0.0f;
    }

    loading_begin_frame();
    draw_loading_floor(LOADING_FLOOR_COUNT, false, alpha);
    const float x = loading_cell_x(LOADING_FLOOR_COUNT);
    const float y = 248.0f;

    if (elapsedUs < frontUs) {
        int frame = (int)(elapsedUs / 60000ULL) & 1;
        draw_loading_sprite(112 + frame * 16, 0, 16, 16, x, y, 4.0f, alpha);
        draw_loading_progress_text(1.0f, alpha);
    } else if (elapsedUs < frontUs + edgeUs) {
        uint64_t localUs = elapsedUs - frontUs;
        int counter = (int)(localUs / 16667ULL);
        if (counter > LOADING_STATE23_TOTAL_FRAMES) counter = LOADING_STATE23_TOTAL_FRAMES;
        int moved = counter < LOADING_EDGE_SLIDE_FRAMES ? counter : LOADING_EDGE_SLIDE_FRAMES;
        float slideGamePx = -8.0f + (float)moved * 0.16f;
        float pitY = y + 16.0f * 4.0f;
        float py = pitY + slideGamePx * 4.0f;
        int frontFrame = (counter / 5) & 1;
        draw_loading_sprite(112 + frontFrame * 16, 0, 16, 16, x, py, 4.0f, alpha);
        if (counter >= 1) {
            int sweatFrame = ((int)(counter * 0.4f)) % 5;
            draw_loading_sprite(128 + sweatFrame * 16, 16, 16, 16,
                                x + 32.0f, py - 32.0f, 4.0f, alpha);
        }
        draw_loading_progress_text(1.0f, alpha);
    } else if (elapsedUs < frontUs + edgeUs + delayUs) {
        uint64_t localUs = elapsedUs - frontUs - edgeUs;
        int counter = (int)(localUs / 16667ULL) + 1;
        float pitY = y + 64.0f;
        // state 8 retains the -0.48 game-pixel slide left by state 23 until
        // obj_player_fall is spawned at the logical pit center on counter 4.
        float playerY = pitY - 0.48f * 4.0f;
        draw_loading_sprite(112 + (counter & 1) * 16, 0, 16, 16, x, playerY, 4.0f, alpha);
        if (counter < LOADING_FALL_DELAY_FRAMES) {
            int sweatFrame = ((int)((LOADING_STATE23_TOTAL_FRAMES + counter) * 0.4f)) % 5;
            draw_loading_sprite(128 + sweatFrame * 16, 16, 16, 16,
                                x + 32.0f, playerY - 32.0f, 4.0f, alpha);
        }
        draw_loading_progress_text(1.0f, alpha);
    } else if (elapsedUs < completeStart) {
        uint64_t localUs = elapsedUs - frontUs - edgeUs - delayUs;
        int gameFrame = (int)(localUs / 16667ULL);
        if (gameFrame >= LOADING_FALL_GAME_FRAMES) gameFrame = LOADING_FALL_GAME_FRAMES - 1;
        float imageSpeed = (float)gameFrame * 0.15f;
        int spriteFrame = (int)imageSpeed;
        if (spriteFrame > 5) spriteFrame = 5;
        float py = y + 64.0f + (float)gameFrame * 0.1f * 4.0f;
        draw_loading_sprite(160 + spriteFrame * 16, 0, 16, 16, x, py, 4.0f, alpha);
        draw_loading_progress_text(1.0f, alpha);
    } else {
        draw_loading_complete_text(alpha);
    }
    loading_end_frame();
    (void)finish;
}

static void play_loading_complete_sequence(void) {
    const uint64_t fallStartUs = 120000ULL +
        (uint64_t)LOADING_STATE23_TOTAL_FRAMES * 16667ULL +
        (uint64_t)LOADING_FALL_DELAY_FRAMES * 16667ULL;
    const uint64_t totalUs = fallStartUs +
        (uint64_t)LOADING_FALL_GAME_FRAMES * 16667ULL +
        LOADING_COMPLETE_HOLD_US + LOADING_FADEOUT_US;
    bool fallSoundPlayed = false;
    uint64_t started = sceKernelGetProcessTimeWide();
    do {
        uint64_t elapsed = sceKernelGetProcessTimeWide() - started;
        if (!fallSoundPlayed && elapsed >= fallStartUs) {
            loading_play_fall_sound();
            fallSoundPlayed = true;
        }
        draw_loading_final_frame(elapsed);
        sceKernelDelayThread(16667);
    } while (sceKernelGetProcessTimeWide() - started < totalUs);
    if (!fallSoundPlayed) loading_play_fall_sound();
    draw_loading_final_frame(totalUs);
}

static void set_key(RunnerKeyboardState *kb, int key, bool down, bool *previous) {
    if (down && !*previous) RunnerKeyboard_onKeyDown(kb, key);
    else if (!down && *previous) RunnerKeyboard_onKeyUp(kb, key);
    *previous = down;
}

static void set_key_pulse(RunnerKeyboardState *kb, int key, bool down, bool *physicalPrevious) {
    if (down && !*physicalPrevious) RunnerKeyboard_onKeyDown(kb, key);
    else if (*physicalPrevious && kb->keyDown[key]) RunnerKeyboard_onKeyUp(kb, key);
    *physicalPrevious = down;
}

static int consume_next_chapter(void) {
    char value = '0';
    SceUID fd = sceIoOpen(NEXT_CHAPTER_PATH, SCE_O_RDONLY, 0);
    if (fd >= 0) {
        sceIoRead(fd, &value, 1);
        sceIoClose(fd);
        sceIoRemove(NEXT_CHAPTER_PATH);
    }
    return value >= '0' && value <= '5' ? value - '0' : 0;
}

static bool restart_into_chapter(int chapter) {
    if (chapter < 0 || chapter > 5) return false;
    char value = (char)('0' + chapter);
    SceUID fd = sceIoOpen(NEXT_CHAPTER_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
    if (fd < 0) return false;
    bool written = sceIoWrite(fd, &value, 1) == 1;
    sceIoClose(fd);
    if (!written) return false;
    char line[80];
    snprintf(line, sizeof(line), "GAME_CHANGE=loadexec chapter=%d", chapter);
    log_line(line);
    glFinish();
    sceKernelDelayThread(100000);
    int result = sceAppMgrLoadExec("app0:eboot.bin", NULL, NULL);
    snprintf(line, sizeof(line), "GAME_CHANGE=loadexec_returned result=0x%08X", result);
    log_line(line);
    return result >= 0;
}

static int chapter_from_request(const char *working_directory) {
    if (working_directory == NULL) return -1;
    if (strstr(working_directory, "launcher") != NULL) return 0;
    const char *chapter = strstr(working_directory, "chapter");
    if (chapter == NULL) return -1;
    chapter += strlen("chapter");
    return chapter[0] >= '0' && chapter[0] <= '5' ? chapter[0] - '0' : -1;
}

static void migrate_save_directory(const char* old_save_dir) {
    SceUID dfd = sceIoDopen(old_save_dir);
    if (dfd >= 0) {
        SceIoDirent dir;
        while (sceIoDread(dfd, &dir) > 0) {
            if (strstr(dir.d_name, "filech") == dir.d_name ||
                strcmp(dir.d_name, "dr.ini") == 0 ||
                strcmp(dir.d_name, "true_config.ini") == 0) {
                char old_path[512];
                char new_path[512];
                snprintf(old_path, sizeof(old_path), "%s%s", old_save_dir, dir.d_name);
                snprintf(new_path, sizeof(new_path), "%s%s", SAVE_PATH, dir.d_name);
                SceIoStat stat;
                if (sceIoGetstat(new_path, &stat) < 0) {
                    sceIoRename(old_path, new_path);
                }
            }
        }
        sceIoDclose(dfd);
    }
}

static void migrate_old_saves(void) {
    // v0.53-v0.56 used this mixed-case directory. Copy its individual save
    // files into the new stable path without overwriting newer saves.
    migrate_save_directory("ux0:data/DeltaruneSaves/");
    migrate_save_directory("ux0:data/deltarune/");
}

static void trophy_event_unlock(int id, const char* name, const char* source) {
    char line[192];
    snprintf(line, sizeof(line), "TROPHY_EVENT id=%d name=%s source=%s",
             id, name != NULL ? name : "<null>", source != NULL ? source : "<null>");
    log_line(line);
    VitaTrophies_unlock(id);
}

static Instance* runner_find_active_object(Runner* runner, const char* name) {
    if (runner == NULL || name == NULL || runner->assetsByName == NULL ||
        runner->instancesByObject == NULL) return NULL;
    ptrdiff_t assetSlot = shgeti(runner->assetsByName, (char*)name);
    if (assetSlot < 0) return NULL;
    int32_t objectIndex = runner->assetsByName[assetSlot].value;
    if (objectIndex < 0 || (uint32_t)objectIndex >= runner->dataWin->objt.count) return NULL;
    Instance** bucket = runner->instancesByObject[objectIndex];
    for (ptrdiff_t i = 0; i < arrlen(bucket); ++i) {
        Instance* inst = bucket[i];
        if (inst != NULL && inst->active && !inst->destroyed) return inst;
    }
    return NULL;
}

static bool runner_object_bool_variable(Runner* runner, const char* objectName,
                                        const char* variableName) {
    Instance* inst = runner_find_active_object(runner, objectName);
    if (inst == NULL || runner->vmContext == NULL ||
        runner->vmContext->varNameMap == NULL || variableName == NULL) return false;
    ptrdiff_t varSlot = shgeti(runner->vmContext->varNameMap, (char*)variableName);
    if (varSlot < 0) return false;
    int32_t varId = runner->vmContext->varNameMap[varSlot].value;
    return RValue_toBool(Instance_getSelfVar(inst, varId));
}

static bool runner_has_active_object(Runner* runner, const char* name) {
    return runner_find_active_object(runner, name) != NULL;
}

static bool play_startup_video_file(const char* path, const char* label,
                                    bool skippable, uint64_t timeoutUs) {
    VitaVideo_setLooping(false);
    VitaVideo_setVolume(1.0f);
    int openResult = VitaVideo_open(path);
    if (openResult < 0) {
        char line[192];
        snprintf(line, sizeof(line), "STARTUP_VIDEO=%s open_failed result=0x%08X path=%s",
                 label, (unsigned int)openResult, path);
        log_line(line);
        return false;
    }

    const uint64_t openedAt = sceKernelGetProcessTimeWide();
    uint64_t firstPresentedAt = 0;
    uint64_t skipFadeStartedAt = 0;
    const uint64_t skipFadeDurationUs = 500000ULL;
    bool presented = false;
    bool skipped = false;
    bool skipFading = false;
    bool timedOut = false;
    SceCtrlData pad;
    memset(&pad, 0, sizeof(pad));
    sceCtrlPeekBufferPositive(0, &pad, 1);
    bool previousCross = (pad.buttons & SCE_CTRL_CROSS) != 0;

    for (;;) {
        bool active = VitaVideo_updateFrame();
        (void)VitaVideo_consumeEvent();
        uint64_t now = sceKernelGetProcessTimeWide();

        if (skippable && !skipFading) {
            memset(&pad, 0, sizeof(pad));
            sceCtrlPeekBufferPositive(0, &pad, 1);
            bool cross = (pad.buttons & SCE_CTRL_CROSS) != 0;
            // Require a fresh X press; holding X while launching the game must
            // not skip the long intro accidentally.
            if (presented && cross && !previousCross) {
                skipped = true;
                skipFading = true;
                skipFadeStartedAt = now;
                char line[160];
                snprintf(line, sizeof(line), "STARTUP_VIDEO=%s skip_requested elapsed_us=%llu",
                         label, (unsigned long long)(now - openedAt));
                log_line(line);
            }
            previousCross = cross;
        }

        bool haveFrame = VitaVideo_hasDecodedFrame();
        if (haveFrame) {
            presented = true;
            if (firstPresentedAt == 0) firstPresentedAt = now;
            VitaVideo_drawHost(0.0f, 0.0f, 960.0f, 544.0f);

            // LongVideo skip hint: invisible for the first second, 1s fade-in,
            // three seconds fully visible, then a 1s fade-out ending at 6s.
            if (skippable && startup_skip_texture != 0 && firstPresentedAt != 0 && !skipFading) {
                uint64_t visibleUs = now - firstPresentedAt;
                float hintAlpha = 0.0f;
                if (visibleUs >= 1000000ULL && visibleUs < 2000000ULL)
                    hintAlpha = (float)(visibleUs - 1000000ULL) / 1000000.0f;
                else if (visibleUs >= 2000000ULL && visibleUs < 5000000ULL)
                    hintAlpha = 1.0f;
                else if (visibleUs >= 5000000ULL && visibleUs < 6000000ULL)
                    hintAlpha = 1.0f - (float)(visibleUs - 5000000ULL) / 1000000.0f;
                if (hintAlpha > 0.0f)
                    VitaVideo_drawTextureHost(startup_skip_texture, 0.0f, 0.0f, 960.0f, 544.0f, hintAlpha);
            }
        } else if (!presented && !skipFading) {
            // Keep the system-provided pic0 visible until a real decoded frame
            // exists. A broken/corrupt file must not stall startup forever.
            if (now - openedAt >= 5000000ULL) {
                timedOut = true;
                break;
            }
            sceKernelDelayThread(1000);
        }

        // A skip never tears SceAvPlayer down on top of a visible movie frame.
        // Keep decoding/presenting while fading picture + audio to black, then
        // leave the last swapped framebuffer fully black before Close/Join.
        if (skipFading) {
            uint64_t fadeUs = now - skipFadeStartedAt;
            float fade = (float)fadeUs / (float)skipFadeDurationUs;
            if (fade < 0.0f) fade = 0.0f;
            if (fade > 1.0f) fade = 1.0f;
            VitaVideo_setVolume(1.0f - fade);
            if (startup_black_texture != 0)
                VitaVideo_drawTextureHost(startup_black_texture, 0.0f, 0.0f, 960.0f, 544.0f, fade);
            vglSwapBuffers(GL_FALSE);
            if (fadeUs >= skipFadeDurationUs) {
                char line[128];
                snprintf(line, sizeof(line), "STARTUP_VIDEO=%s skip_fade_complete", label);
                log_line(line);
                break;
            }
            // Do not evaluate normal end/timeout while the visual skip fade is running.
            continue;
        }

        if (haveFrame) vglSwapBuffers(GL_FALSE);

        if (!active || VitaVideo_getStatus() == VITA_VIDEO_STATUS_NONE)
            break;
        if (now - openedAt >= timeoutUs) {
            timedOut = true;
            break;
        }
    }

    const uint64_t elapsed = sceKernelGetProcessTimeWide() - openedAt;
    if (strcmp(label, "long") == 0 && presented && !timedOut) {
        if (skipped) startup_long_skipped = true;
        else startup_long_watched = true;
    }
    if (skipped) VitaVideo_setVolume(0.0f);
    VitaVideo_close();
    VitaVideo_setVolume(1.0f);
    char line[224];
    snprintf(line, sizeof(line),
             "STARTUP_VIDEO=%s complete presented=%d skipped=%d timeout=%d elapsed_us=%llu",
             label, presented ? 1 : 0, skipped ? 1 : 0, timedOut ? 1 : 0,
             (unsigned long long)elapsed);
    log_line(line);
    return presented && !timedOut;
}

static bool play_startup_video_sequence(void) {
    int initResult = VitaVideo_init();
    if (initResult < 0) {
        char line[128];
        snprintf(line, sizeof(line), "STARTUP_VIDEO=init_failed result=0x%08X",
                 (unsigned int)initResult);
        log_line(line);
        VitaVideo_shutdown();
        return false;
    }

    // pic0.png is owned by the Vita shell. Startup movies live in external
    // Vita data so they can be updated without reinstalling the VPK.
    const char* introPath = "ux0:data/voidstranger/intro/IntroVoidStranger.mp4";
    const char* longPath = "ux0:data/voidstranger/intro/LongVideo.mp4";

    bool introOk = play_startup_video_file(introPath,
                                           "intro", false, 15000000ULL);

    startup_skip_texture = load_loading_texture("ux0:data/voidstranger/intro/skip.png");
    startup_black_texture = create_solid_texture_rgba(0, 0, 0, 255);
    {
        char line[192];
        snprintf(line, sizeof(line),
                 "STARTUP_SKIP_OVERLAY loaded=%d texture=%u path=ux0:data/voidstranger/intro/skip.png",
                 startup_skip_texture != 0 ? 1 : 0, (unsigned)startup_skip_texture);
        log_line(line);
    }

    bool longOk = play_startup_video_file(longPath,
                                          "long", true, 50000000ULL);
    if (startup_skip_texture != 0) glDeleteTextures(1, &startup_skip_texture);
    if (startup_black_texture != 0) glDeleteTextures(1, &startup_black_texture);
    startup_skip_texture = 0;
    startup_black_texture = 0;
    VitaVideo_shutdown();
    return introOk && longOk;
}
int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    // Record whether the external Void Stranger data folder already exists.
    SceIoStat rootStat;
    memset(&rootStat, 0, sizeof(rootStat));
    g_initialRootExists = (sceIoGetstat("ux0:data/voidstranger", &rootStat) >= 0) &&
                          SCE_S_ISDIR(rootStat.st_mode);

    sceIoMkdir("ux0:data/voidstranger", 0777);
    sceIoMkdir(DATA_ROOT, 0777);
    sceIoMkdir(SAVE_PATH, 0777);
    /* Saves are isolated; never migrate or alter another port's files. */
    /* Probe logging is intentionally opt-in. Repeated memory-card writes from
       Draw/Step diagnostics materially disturb frame pacing on real hardware.
       Set devmode=1 in ux0:data/voidstranger/config.ini when a trace is needed. */
    g_vitaProbeLoggingEnabled = config_devmode_enabled();
    sceIoRemove(LOG_PATH);
    sceIoRemove(COMPAT_LOG_PATH);
    /* Void Stranger is a single-game port. Keep 1 only as the runtime profile. */
    int active_chapter = 1;
    g_vitaActiveChapter = active_chapter;

    log_line("Void Stranger 1.1.3 + Butterscotch VitaRenderer " PORT_BUILD_VERSION);
    log_line("MAIN_STACK=4194304");
    char startup_line[96];
    snprintf(startup_line, sizeof(startup_line), "AUDIO=openal ENTRY=chapter%d CONTROLS=vita+touch", active_chapter);
    log_line(startup_line);
    if (active_chapter == 5)
    log_line("CH5_OPT=legacy_lut_skipped sunshadows_krisyard_enabled original_atlas_min=2048 pvr_source=ux0_steam_v0.0.253 compression=bc3");

    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
    // The generated fixed-pipeline shader used by the legacy renderer trips the
    // aggressive runtime optimizer on Vita. O0 is slower only during the first
    // compilation and avoids that optimizer path entirely.
    vglSetupRuntimeShaderCompiler(SHARK_OPT_SLOW, 0, 0, 0);
    log_line("SHARK=opt_slow fastmath=0 fastprecision=0 fastint=0");
    log_line("RENDERER=vita_arrays_v2 immediate_bridge=all STDIO=unbuffered");
    // Keep the runner at the standard high-performance clocks used by mature
    // Vita ports. The standalone runner did not inherit this initialization
    // from the old loaders, leaving GPU-heavy legacy-GL rooms underclocked.
    int clock_arm = scePowerSetArmClockFrequency(444);
    int clock_bus = scePowerSetBusClockFrequency(222);
    int clock_gpu = scePowerSetGpuClockFrequency(222);
    int clock_xbar = scePowerSetGpuXbarClockFrequency(166);
    char clock_log[160];
    snprintf(clock_log, sizeof(clock_log),
             "CLOCKS=arm444:%d bus222:%d gpu222:%d xbar166:%d",
             clock_arm, clock_bus, clock_gpu, clock_xbar);
    log_line(clock_log);

    // vglInitExtended consumes nearly every heap it sees after applying only
    // one RAM threshold. On some relaunches that made its allocation depend on
    // system fragmentation and it stopped inside initialization before the
    // first framebuffer was shown. Use the stable heap totals measured in the
    // working builds, capped by the memory actually available at this launch.
    SceKernelFreeMemorySizeInfo preInitMemory;
    memset(&preInitMemory, 0, sizeof(preInitMemory));
    preInitMemory.size = sizeof(preInitMemory);
    sceKernelGetFreeMemorySize(&preInitMemory);
    // Chapter 3's hardware MP4 decoder allocates its output frames through
    // VGL_MEM_SLOW. A 48 MiB pool was already down to ~3 MiB in the video
    // room, while roughly 30 MiB of ordinary user RAM remained unreachable by
    // VitaGL. Give this chapter another 12 MiB without changing CDRAM/PHY.
    const int modernGlRequested = config_modern_gl_enabled();
    uint32_t ramPoolCap = modernGlRequested ? 64U * 1024U * 1024U :
                          (active_chapter == 5 ? 44U * 1024U * 1024U :
                          (active_chapter == 3 ? 60U * 1024U * 1024U :
                                                 48U * 1024U * 1024U));
    uint32_t ramPool = preInitMemory.size_user > 40U * 1024U * 1024U ?
                       preInitMemory.size_user - (modernGlRequested ? 20U : 32U) * 1024U * 1024U :
                       8U * 1024U * 1024U;
    if (ramPool > ramPoolCap) ramPool = ramPoolCap;
    uint32_t cdramPool = preInitMemory.size_cdram > 16U * 1024U * 1024U ?
                         preInitMemory.size_cdram - 16U * 1024U * 1024U :
                         preInitMemory.size_cdram / 2U;
    if (cdramPool > 96U * 1024U * 1024U) cdramPool = 96U * 1024U * 1024U;
    // Keep PHYCONT mostly outside VitaGL so SceAvPlayer can allocate its
    // decoded 960x544 YUV frames from dedicated contiguous blocks. The former
    // 26 MiB reservation starved the hardware decoder before its first frame.
    uint32_t phyPool = 0;
    char vitaglInitLog[192];
    snprintf(vitaglInitLog, sizeof(vitaglInitLog),
             "VITAGL=init_begin fixed ram=%u cdram=%u phy=%u free_user=%u free_cdram=%u free_phy=%u",
             (unsigned int)ramPool, (unsigned int)cdramPool, (unsigned int)phyPool,
             (unsigned int)preInitMemory.size_user,
             (unsigned int)preInitMemory.size_cdram,
             (unsigned int)preInitMemory.size_phycont);
    log_line(vitaglInitLog);
    vglInitWithCustomSizes(0, 960, 544, ramPool, cdramPool, phyPool, 0,
                           SCE_GXM_MULTISAMPLE_NONE);
    log_line("VITAGL=init_complete");

    // The video presentation path itself still needs a small VitaGL PHYCONT
    // heap. Eight MiB leaves the majority of contiguous memory available to
    // SceAvPlayer while remaining useful for the rest of this single-process
    // port after the startup movies finish.
    uint32_t startupPhyPool = preInitMemory.size_phycont >= 4U * 1024U * 1024U ?
                              4U * 1024U * 1024U : (uint32_t)preInitMemory.size_phycont;
    if (startupPhyPool > 0) {
        vglPhycontMemLazyInit(startupPhyPool);
        char phyLog[128];
        snprintf(phyLog, sizeof(phyLog),
                 "VITAGL=startup_phycont size=%u free_slow=%u",
                 (unsigned int)startupPhyPool, (unsigned int)vglMemFree(VGL_MEM_SLOW));
        log_line(phyLog);
    }

    // Vita shell pic0 -> mandatory short intro -> skippable long intro -> game.
    (void)play_startup_video_sequence();

    // The commercial game data is intentionally not bundled in the VPK. If
    // the required external files are absent, reproduce the Sera-style prompt
    // using VPK-owned assets before the runner starts parsing data.win.
    MissingDataInspection dataInspection;
    MissingDataLayoutState dataLayout = MissingDataScene_inspect(&dataInspection);
    if (dataLayout == MISSING_DATA_LAYOUT_ORGANIZABLE ||
        dataLayout == MISSING_DATA_LAYOUT_AMBIGUOUS) {
        char organizeLine[128];
        snprintf(organizeLine, sizeof(organizeLine),
                 "MISSING_DATA_ORGANIZATION=begin layout=%d missing=0x%X misplaced=0x%X duplicate=0x%X",
                 (int)dataLayout, (unsigned)dataInspection.rootMissingMask,
                 (unsigned)dataInspection.misplacedMask, (unsigned)dataInspection.duplicateMask);
        log_line(organizeLine);
        int organizeResult = MissingDataScene_runOrganization(&dataInspection);
        if (organizeResult == 0) {
            log_line("MISSING_DATA_ORGANIZATION=user_exit");
            sceKernelExitProcess(0);
            return 0;
        }
        if (organizeResult < 0) {
            log_line("MISSING_DATA_ORGANIZATION=failed");
            show_startup_error_and_exit("The game files were found, but their folder structure could not be fixed safely.", DATA_ROOT);
        }
        log_line("MISSING_DATA_ORGANIZATION=fixed");
        dataLayout = MissingDataScene_inspect(&dataInspection);
    }

    if (dataLayout != MISSING_DATA_LAYOUT_READY) {
        uint32_t missingDataMask = dataInspection.rootMissingMask;
        char missingLine[96];
        snprintf(missingLine, sizeof(missingLine), "MISSING_DATA_SCENE=begin mask=0x%X", (unsigned)missingDataMask);
        log_line(missingLine);
        int sceneResult = MissingDataScene_run(missingDataMask);
        if (sceneResult == 0) {
            log_line("MISSING_DATA_SCENE=user_exit");
            sceKernelExitProcess(0);
            return 0;
        }
        if (sceneResult < 0) {
            log_line("MISSING_DATA_SCENE=asset_or_runtime_failure");
            show_startup_error_and_exit("Required game data is missing and the install-help scene could not be loaded.", DATA_ROOT);
        }
        log_line("MISSING_DATA_SCENE=data_ready_after_recheck");
    }
    // Feature 2: fail fast with a readable report if the game data is not
    // installed, instead of proceeding and crashing later in the boot path.
    /* The direct data.win check below is sufficient for this single-game layout. */

    bool nativeTrophies = VitaTrophies_init();
    if (nativeTrophies) {
        if (startup_long_watched) trophy_event_unlock(2, "The Long Way", "startup_long_watched");
        if (startup_long_skipped) trophy_event_unlock(5, "Cut to the Chase", "startup_long_skipped");
    }
    char trophyLog[192];
    snprintf(trophyLog, sizeof(trophyLog),
             "TROPHIES_NATIVE=%s stage=%s result=0x%08X %s",
             nativeTrophies ? "ready" : "unavailable",
             VitaTrophies_lastStage(), (unsigned int)VitaTrophies_lastResult(),
             nativeTrophies ? "notrpdrm_pack_available" : "local_fallback_active");
    log_line(trophyLog);
    log_memory_snapshot("after_vitagl_init", NULL);

    VitaSettings settings;
    VitaSettings_load(&settings);
    loading_screen_set_language(settings.activeLanguage);
    /* The inherited Game Settings UI is Deltarune-specific. Retain only its
       internal defaults used by audio/input/rendering; never expose the UI. */
    settings.showSettings = false;
    settings.open = false;
    // Keep devmode from ux0:data/voidstranger/config.ini. The previous forced
    // false made every Dev Mode shortcut unreachable even with devmode=1.
    settings.touchEnabled = false;
    log_line("TOUCH_MODE=fruit_scene_only physical_cross_preserved");
    // This port has no Deltarune Game Settings overlay, so an old/default
    // fps_mode=0 silently locked Void Stranger to 30 FPS. Always pace the
    // Vita build at the game's intended 60 Hz; frames that genuinely exceed
    // the budget are still reported by PERF_DETAIL.
    settings.fpsTargetMode = 2;
    settings.vsyncEnabled = true;
    g_vitaGameVsyncEnabled = 1;
    // vglSwapBuffers()'s boolean argument controls Common Dialog updates, not
    // synchronization.  Configure VitaGL's real swap interval explicitly.
    vglWaitVblankStart(g_vitaGameVsyncEnabled ? GL_TRUE : GL_FALSE);
    char settingsLog[256];
    snprintf(settingsLog, sizeof(settingsLog),
             "SETTINGS_LOAD master=%d music=%d sfx=%d disabled=%d vsync=%d fps=60 mod=%s",
             settings.masterVolume, settings.musicVolume, settings.sfxVolume,
             settings.audioDisabled ? 1 : 0, settings.vsyncEnabled ? 1 : 0,
             (settings.modIndex >= 0 && settings.modIndex < settings.modCount) ? settings.modNames[settings.modIndex] : "Original");
    log_line(settingsLog);
    // Reconcile the persistent in-game trophy file with the Vita database on
    // every boot. Existing system trophies are skipped by VitaTrophies, while
    // local-only unlocks are queued without requiring the player to earn them
    // again.
    uint32_t localTrophyMask = 0;
    unsigned int localTrophyCount = 0;
    for (int trophy = 0; trophy < 30; ++trophy) {
        if (!settings.trophiesUnlocked[trophy]) continue;
        localTrophyMask |= 1U << trophy;
        ++localTrophyCount;
    }
    unsigned int trophiesQueued = VitaTrophies_syncMask(localTrophyMask);
    snprintf(trophyLog, sizeof(trophyLog),
             "TROPHIES_SYNC=boot local=%u queued_native=%u native=%s",
             localTrophyCount, trophiesQueued, nativeTrophies ? "ready" : "unavailable");
    log_line(trophyLog);
    VitaSettings_forceLegacyRenderer(&settings);
    VitaSettings_setActiveChapter(active_chapter);
    VitaSettings_setLauncherMode(active_chapter == 0);
    VitaBorders_init(active_chapter);

    int next_chapter = -1;
    char game_path[192];
    char bundle_path[128];
    snprintf(game_path, sizeof(game_path), DATA_ROOT "data.win");
    snprintf(bundle_path, sizeof(bundle_path), DATA_ROOT);
    if (active_chapter == 0) {
        SceIoStat st;
        if (sceIoGetstat(game_path, &st) < 0) {
            snprintf(game_path, sizeof(game_path), "app0:assets/chapter0/data.win");
            snprintf(bundle_path, sizeof(bundle_path), "app0:assets/chapter0/");
            log_line("LAUNCHER=app0_fallback_loaded");
        }
    }
    // A mod can provide a dedicated launcher translation at
    // mods/Lang/<name>/chapter0/data.win. Keep the original launcher as fallback
    // when that file is absent; never reuse another chapter's data here.
    if (settings.modIndex > 0 && settings.modIndex < settings.modCount) {
        SceIoStat mod_stat;
        char mod_game[160];
        snprintf(mod_game, sizeof(mod_game), DATA_ROOT "mods/Lang/%s/chapter%d/data.win", settings.modNames[settings.modIndex], active_chapter);
        if (sceIoGetstat(mod_game, &mod_stat) >= 0) {
            snprintf(game_path, sizeof(game_path), "%s", mod_game);
            log_line("MOD=alternate_game=enabled");
        } else if (active_chapter == 0) {
            // Compatibility with data generated before the Lang/<name>
            // migration.  Without this, selecting Portuguese-BR could
            // silently boot the English Chapter 0 launcher.
            const char* legacyName = strcmp(settings.modNames[settings.modIndex], "Portuguese-BR") == 0 ?
                                     "PTBR" : settings.modNames[settings.modIndex];
            snprintf(mod_game, sizeof(mod_game), DATA_ROOT "mods/%s/chapter0/data.win", legacyName);
            if (sceIoGetstat(mod_game, &mod_stat) >= 0) {
                snprintf(game_path, sizeof(game_path), "%s", mod_game);
                log_line("MOD=alternate_game=legacy_chapter0_fallback");
            } else {
                log_line("MOD=alternate_game=missing_chapter0_using_english");
            }
        }
    }



    SceIoStat gameStat;
    memset(&gameStat, 0, sizeof(gameStat));
    if (sceIoGetstat(game_path, &gameStat) < 0) {
        char failure[160];
        snprintf(failure, sizeof(failure),
                 "Arquivo obrigatorio ausente: data.win");
        log_line("STARTUP_FATAL=missing_data_win");
        show_startup_error_and_exit(failure, game_path);
    }
    if (gameStat.st_size < 1024) {
        char failure[160];
        snprintf(failure, sizeof(failure),
                 "Arquivo invalido ou incompleto: data.win");
        log_line("STARTUP_FATAL=invalid_data_win_size");
        show_startup_error_and_exit(failure, game_path);
    }

    DataWinParserOptions options = {0};
    options.parseGen8 = true; options.parseOptn = true; options.parseLang = true;
    options.parseExtn = true; options.parseSond = true; options.parseAgrp = true;
    options.parseSprt = true; options.parseBgnd = true; options.parsePath = true;
    options.parseScpt = true; options.parseGlob = true; options.parseShdr = true;
    options.parseFont = true; options.parseTmln = true; options.parseObjt = true;
    options.parseRoom = true; options.parseTpag = true; options.parseCode = true;
    options.parseVari = true; options.parseFunc = true; options.parseStrg = true;
    options.parseTxtr = true; options.parseAudo = true;
    options.skipLoadingPreciseMasksForNonPreciseSprites = true;
    options.lazyLoadRooms = true;
    options.lazyLoadTextures = true;
    options.lazyLoadAudio = true;
    options.loadType = DATAWINLOADTYPE_LOAD_PER_CHUNK;
    options.progressCallback = progress;

    log_line("DATAWIN=parse_begin");
    DataWin *dw = DataWin_parse(game_path, options);
    if (dw == NULL) {
        log_line("STARTUP_FATAL=data_win_parse_failed");
        show_startup_error_and_exit(
            "Nao foi possivel interpretar data.win. O arquivo pode estar corrompido ou ser de uma versao incompativel.",
            game_path);
    }
    log_line("DATAWIN=parse_complete");
    // Keep the normal room payload. The alternate "original" room was once
    // swapped in solely to obtain draw_path, but that also replaced collision
    // and instance data. draw_path is implemented by the runner now.
    VMContext *vm = VM_create(dw);
    log_line("VM=create_complete");
    OverlayFileSystem *fs = OverlayFileSystem_create(bundle_path, SAVE_PATH);
    char mod_path[160];
    if (settings.modIndex > 0 && settings.modIndex < settings.modCount) {
        snprintf(mod_path, sizeof(mod_path), DATA_ROOT "mods/Lang/%s/chapter%d/", settings.modNames[settings.modIndex], active_chapter);
        OverlayFileSystem_setModPath(fs, mod_path);
        log_line("MOD=overlay=enabled");
    } else {
        log_line("MOD=Original overlay=disabled");
    }
    Renderer *renderer = NULL;
    if (settings.modernGlEnabled) {
        // Present the normal loader before creating the programmable renderer.
        // At this point fixed-pipeline state is still isolated and safe. The
        // framebuffer remains visible while Modern GL initializes lazily.
        if (active_chapter > 0) {
            loading_screen_init();
            draw_loading_screen(true, 0.0f, g_vitaBc3OnlyEnabled ? 0 : 1);
            loading_screen_shutdown();
            log_line("CHAPTER_LOADING=visible renderer=modern-gl phase=pre_renderer");
        }
        char modernReason[128] = {0};
        bool modernReady = GLRenderer_preflight(dw, modernReason, sizeof(modernReason));
        char modernLog[192];
        snprintf(modernLog, sizeof(modernLog),
                 "MODERN_GL_PREFLIGHT=%s reason=%s",
                 modernReady ? "pass" : "fail", modernReason);
        log_line(modernLog);
        renderer = modernReady ? GLRenderer_create() : NULL;
        if (renderer != NULL) {
            g_vitaModernGlActive = true;
            log_line("RENDERER_ACTIVE=modern-gl bootstrap=legacy_loading_overlay_disabled");
        } else {
            log_line("MODERN_GL=create_failed fallback=legacy-gl");
        }
    }
    if (renderer == NULL) {
        renderer = GLLegacyRenderer_create();
        g_vitaModernGlActive = false;
        log_line("RENDERER_ACTIVE=legacy-gl");
    }
    AudioSystem *audio = (AudioSystem *)AlAudioSystem_create();
    log_line("SUBSYSTEMS=create_complete");

    log_line("RUNNER=create_begin");
    Runner *runner = Runner_create(dw, vm, renderer, (FileSystem *)fs, audio);
    loading_audio_system = audio;
    log_line("RUNNER=create_returned");
    const bool spanishModActive = settings.modIndex > 0 && settings.modIndex < settings.modCount &&
                                  strcmp(settings.modNames[settings.modIndex], "Spanish") == 0;
    g_vitaSpanishModActive = spanishModActive && active_chapter > 0 ? 1 : 0;
    if (spanishModActive && active_chapter > 0) {
        // The Spanish package includes PC-oriented trophy and console-border
        // overlays. Vita supplies both facilities itself; allowing the mod
        // objects to run changes the application-surface layout and squeezes
        // the game into the upper/right PC coordinates.
        Runner_setObjectDisabled(runner, "obj_trophy_manager", true);
        Runner_setObjectDisabled(runner, "obj_border_manager", true);
        Runner_setObjectDisabled(runner, "obj_custom_config", true);
        Runner_setObjectDisabled(runner, "obj_spanish_config", true);
        Runner_setObjectDisabled(runner, "obj_border_controller", true);
        log_line("SPANISH_OVERLAYS=disabled trophy_manager,border_manager,border_controller,custom_config,spanish_config");
    }
    // The Steam launcher requires the Windows branch to emit game_change for
    // chapter selection. Reporting os_psvita leaves it in PLACE_CHAPTER_SELECT.
    runner->osType = OS_WINDOWS;
    log_line("PLATFORM=os_windows steam_launcher_compatible");
    VitaSettings_applyAudio(&settings, audio);
    char *launcher_args[] = {"eboot.bin", "-game", "data.win"};
    char *chapter_args[] = {"eboot.bin", "-game", "data.win", "launcher", "switch_-1", "returning_0"};
    if (active_chapter == 0) {
        Runner_setGameArgs(runner, launcher_args, 3);
        log_line("ARGS=eboot.bin|-game|data.win");
    } else {
        Runner_setGameArgs(runner, chapter_args, 6);
        log_line("ARGS=eboot.bin|-game|data.win|launcher|switch_-1|returning_0");
    }
    runner->debugMode = false;
    log_line("RUNNER=create_complete");
    // Some official chapter data keeps ordinary effects in audiogroup1.dat.
    // The PT-BR data happens to request that group early, while the original
    // scripts can play an effect before their deferred group-load event. Load
    // every declared auxiliary group lazily now; AUDO payloads remain on disk
    // until the individual SFX is actually used.
    if (audio->vtable->groupLoad != NULL && dw->agrp.count > 1) {
        for (uint32_t group = 1; group < dw->agrp.count; ++group) {
            if (audio->vtable->groupIsLoaded == NULL ||
                !audio->vtable->groupIsLoaded(audio, (int32_t)group))
                audio->vtable->groupLoad(audio, (int32_t)group);
        }
        log_line("AUDIO_GROUPS=auxiliary_groups_loaded_lazy");
    }
    if (active_chapter > 0) {
        // Chapter bootstrap can create and replace a music stream before the
        // first playable/menu room is presented. Keep that work inaudible;
        // otherwise the loading screen exposes a short start/stop/start pop.
        AlAudioSystem_setCategoryGains((AlAudioSystem*)audio, 0.0f,
            (float)settings.sfxVolume / 10.0f);
        log_line("STARTUP_AUDIO=music_muted_loading_sfx_enabled_until_first_presented_frame");
        // Always replace the black chapter-start gap with the loading artwork.
        // The progress bar remains exclusive to actual cache preparation.
        if (!g_vitaModernGlActive) {
            loading_screen_init();
            log_line("LOADING_INTRO=stairs_sfx_begin sound=snd_stairs id=110");
            int32_t stairsSoundInstance = loading_play_stairs_sound();
            loading_wait_for_sound(stairsSoundInstance);
            log_line("LOADING_INTRO=stairs_sfx_complete first_block_fade_begin");
            play_loading_intro_fade();
            loading_animation_reset();
            draw_loading_screen(false, 0.0f, 0);
            log_line("LOADING_INTRO=first_block_fade_complete");
            log_line("CHAPTER_LOADING=visible bar=hidden renderer=legacy-gl");
        } else {
            // The custom loader uses fixed-pipeline matrices/client arrays and
            // swaps VitaGL directly. Running it inside the programmable
            // renderer corrupted its state and hard-locked the GPU.
            log_line("CHAPTER_LOADING=skipped renderer=modern-gl reason=fixed_pipeline_overlay");
        }
    }
    Runner_initFirstRoom(runner);
    log_line("RUNNER=first_room_complete");
    log_line("PERF_CAPTURE=compact interval_frames=120 profiler=disabled");
    bool startup_audio_paused = false;
    if (active_chapter > 0 && audio->vtable->pauseAll != NULL) {
        // Category gain alone muted the chapter bootstrap but allowed voices
        // to keep advancing asynchronously. Chapter 5's spoken "Deltarune"
        // therefore resumed at "arune". Freeze active sources until the first
        // rendered frame, then resume them at the beginning of the fade.
        audio->vtable->pauseAll(audio);
        startup_audio_paused = true;
        log_line("STARTUP_AUDIO=paused_until_first_presented_frame");
    }
    if (!g_vitaModernGlActive && active_chapter == 1 && runner->currentRoom != NULL && runner->currentRoom->name != NULL &&
        strstr(runner->currentRoom->name, "PLACE_CONTACT") != NULL) {
        preload_place_contact_atlases(dw, (GLLegacyRenderer*)renderer);
    }
    if (settings.devMode && settings.debugDevEnabled) {
        dev_log_start(runner->currentRoom != NULL ? runner->currentRoom->name : NULL);
        Profiler_setEnabled(&runner->vmContext->profiler, true);
        char shaderCatalog[128];
        snprintf(shaderCatalog, sizeof(shaderCatalog), "SHADER_CATALOG count=%u renderer=%s",
                 dw->shdr.count, g_vitaModernGlActive ? "modern-gl" : "legacy-gl");
        dev_log_write(shaderCatalog);
        for (uint32_t i = 0; i < dw->shdr.count; ++i) {
            char shaderLine[256];
            snprintf(shaderLine, sizeof(shaderLine), "SHADER_DEF index=%u name=%s version=%d",
                     i, dw->shdr.shaders[i].name != NULL ? dw->shdr.shaders[i].name : "<unnamed>",
                     dw->shdr.shaders[i].version);
            dev_log_write(shaderLine);
        }
    }
    if (active_chapter > 0) {
        log_line("AUDIO_PRELOAD=begin scope=active_chapter");
        // Chapter 2's Mansion battle logs show an 18 ms first-use snd_play
        // spike. Its current Steam SOND layout has been validated by the
        // selective priority list, so decode those small battle/UI effects
        // while the loading screen is still visible. Other chapter variants
        // remain on the conservative deferred path.
        bool preloadSfxBuffers = false;
        uint32_t audioCached = AlAudioSystem_preloadChapterSfx((AlAudioSystem*)audio,
                                                               preloadSfxBuffers);
        char audioCacheLine[96];
        snprintf(audioCacheLine, sizeof(audioCacheLine),
                 "AUDIO_PRELOAD=complete cached=%u capacity=%u decode=%s", audioCached,
                 (unsigned)MAX_SFX_BUFFER_CACHE, preloadSfxBuffers ? "enabled" : "deferred");
        log_line(audioCacheLine);
        // The VitaGL fixed-pipeline bridge and the runner must exist before the
        // loading screen draws. Running this directly after DataWin_parse made
        // the first progress frame touch an uninitialized GL pipeline.
        log_line("TEXTURE_PRELOAD=begin stage=runner_ready");
        
        // Select the artwork from the actual cache state. Previously every
        // mixed/RGBA4444 profile displayed Generating even when complete.vtc
        // was valid and prepareTextureCache returned immediately.
        bool textureCacheComplete = !g_vitaModernGlActive &&
            GLLegacyRenderer_textureCacheIsComplete(dw);
        bool textureGenerationRequired = !g_vitaModernGlActive &&
            !textureCacheComplete;
        char textureModeLine[112];
        snprintf(textureModeLine, sizeof(textureModeLine),
                 "TEXTURE_PRELOAD=mode %s cache_complete=%d bc3_only=%d",
                 textureGenerationRequired ? "generating" : "loading",
                 textureCacheComplete ? 1 : 0, g_vitaBc3OnlyEnabled ? 1 : 0);
        log_line(textureModeLine);
        uint64_t textureLoadingStartedUs = sceKernelGetProcessTimeWide();
        if (!g_vitaModernGlActive) {
            loading_animation_reset();
            draw_loading_screen(true, 0.0f, textureGenerationRequired ? 1 : 0);
        }
        
        VitaTexturePrepareProgress textureProgress = g_vitaModernGlActive ? NULL :
            (textureGenerationRequired ? texture_loading_progress : texture_existing_progress);
        uint32_t cached = 0;
        if (g_vitaModernGlActive) {
            // Modern GL already uploads pages lazily through its own texture
            // path. Preparing every page here decoded the whole chapter into
            // RGBA cache files before frame 0 and exhausted memory around
            // page 6 on Chapters 3/5 (the same pattern seen in Pizza Tower).
            log_line("TEXTURE_PRELOAD=skipped renderer=modern-gl mode=lazy_on_demand");
        } else {
            cached = GLLegacyRenderer_prepareTextureCache(dw, textureProgress, NULL);
            uint32_t battleCorePages =
                GLLegacyRenderer_preloadChapterBattleCore((GLLegacyRenderer*)renderer);
            if (battleCorePages > 0) {
                char battleCoreLine[96];
                snprintf(battleCoreLine, sizeof(battleCoreLine),
                         "BATTLE_PRELOAD=complete chapter=%d pages=%u",
                         active_chapter, battleCorePages);
                log_line(battleCoreLine);
            }
        }
        char cache_line[96];
        snprintf(cache_line, sizeof(cache_line), "TEXTURE_PRELOAD=complete prepared=%u total=%u",
                 cached, dw->txtr.count);
        log_line(cache_line);
        if (!g_vitaModernGlActive && active_chapter == 3) {
            preload_chapter3_slide_atlases(dw, (GLLegacyRenderer*)renderer);
            preload_chapter3_finale_atlases(dw, (GLLegacyRenderer*)renderer);
        }
        // Existing BC3 files can be validated in only a few frames. Keep the
        // requested Loading/Generating artwork visible for at least one
        // second, otherwise the user sees only a black transition and never
        // has time to read the state label.
        if (!g_vitaModernGlActive) {
            // Cache validation can finish almost instantly. The loading scene is
            // intentionally allowed to finish all queued 8%% steps at normal speed
            // before the edge/fall sequence, rather than stretching movement over
            // the raw 0..100%% cache ratio.
            loading_finish_progress_animation(textureGenerationRequired ? 1 : 0);
            const uint64_t minimumLoadingUs = 1000000ULL;
            while (sceKernelGetProcessTimeWide() - textureLoadingStartedUs < minimumLoadingUs) {
                loading_draw_animation_frame(true);
                sceKernelDelayThread(16667);
            }

            play_loading_complete_sequence();
            log_line("LOADING_FINAL_SEQUENCE=front edge_approach fall complete fadeout");
            loading_screen_shutdown();
        }
    }
    char display_line[160];
    snprintf(display_line, sizeof(display_line), "DISPLAY=gen8_%dx%d applied_%dx%d host_960x544 mode=native_centered",
             (int)dw->gen8.defaultWindowWidth, (int)dw->gen8.defaultWindowHeight,
             (int)dw->gen8.defaultWindowWidth, (int)dw->gen8.defaultWindowHeight);
    log_line(display_line);

    bool previous[sizeof(KEY_MAP) / sizeof(KEY_MAP[0])] = {0};
    bool gp_previous[20] = {0};
    uint32_t frame = 0;
    uint64_t last_time = sceKernelGetProcessTimeWide();
    uint64_t next_frame_deadline = last_time;
    bool exit_requested = false;
    int logged_game_w = -1;
    int logged_game_h = -1;
    int logged_room_index = -999;
    uint64_t perf_window_start = last_time;
    uint64_t perf_total_us = 0, perf_max_us = 0;
    uint32_t perf_frames = 0, perf_drops = 0, perf_severe = 0;
    uint64_t last_slow_log = 0;
    bool save_load_fade = false;
    uint64_t save_load_fade_start = last_time;
    const uint64_t save_load_fade_duration = 1000000ULL;
    // The initial chapter audio was muted above. Do not start its fade on a
    // wall-clock timer until the first game frame has actually reached the
    // display; texture upload time must not make music audible over black.
    int audio_present_wait_frames = active_chapter > 0 ? 1 : 0;
    if (audio_present_wait_frames > 0) log_line("STARTUP_AUDIO_FADE=waiting_for_presented_frame");
    bool border_cycle_dpad_held = false;
    bool dev_room_shortcut_held = false;
    bool dev_room_nav_held = false;
    bool settings_touch_held = false;
    float settings_touch_last_y = 0.0f;
    int32_t dev_room_target = runner->currentRoomIndex;
    bool dev_force_move = false;
    uint32_t failure_black_frames = 0;
    bool failure_exit_held = false;
    bool gameplay_input_armed = false;
    uint32_t gameplay_input_neutral_frames = 0;
    bool trophy_first_move = false;
    bool trophy_first_room_change = false;
    bool trophy_memories_opened = false;
    bool fruit_touch_held = false;

    while (!exit_requested && !runner->shouldExit) {
        RunnerKeyboard_beginFrame(runner->keyboard);
        RunnerGamepad_beginFrame(runner->gamepads);
        // Expose the Vita as a real GameMaker gamepad. The game uses direct
        // gamepad_* calls for controller discovery/remapping and the title UI.
        // Keyboard compatibility is still fed below for the few keyboard-only
        // Steam code paths, while obj_game prioritizes the gamepad branch so a
        // Vita press is counted only once by the normal input state machine.
        runner->gamepads->connectedCount = 1;
        runner->gamepads->slots[0].connected = true;
        strcpy(runner->gamepads->slots[0].description, "Sony DualShock 4");
        RunnerMouse_beginFrame(runner->mouse);
        SceCtrlData pad = {0};
        sceCtrlPeekBufferPositive(0, &pad, 1);

        // The Steam scripts only expose the shortened game-over exit path on
        // PS4/PS5/Switch.  The Vita identifies as Windows to retain normal
        // filesystem/save behavior, so "Give up" otherwise waits on a black
        // screen until the long AUDIO_DARKNESS timeout expires.  Once the
        // failure scene has stopped drawing its dialogue, accept a face button
        // like the console builds and keep a bounded fallback for audio streams
        // that never report completion.
        const char* input_room = runner->currentRoom != NULL ? runner->currentRoom->name : NULL;
        uint32_t submittedPrimitives = g_vitaModernGlActive ?
            ((GLRenderer*)renderer)->frameSubmittedPrimitives :
            ((GLLegacyRenderer*)renderer)->frameSubmittedPrimitives;
        bool failure_black = active_chapter > 0 && input_room != NULL &&
                             strcmp(input_room, "PLACE_FAILURE") == 0 &&
                             submittedPrimitives <= 2;
        if (failure_black) {
            if (failure_black_frames < UINT32_MAX) failure_black_frames++;
            bool face_down = (pad.buttons & (SCE_CTRL_CROSS | SCE_CTRL_CIRCLE |
                                             SCE_CTRL_TRIANGLE | SCE_CTRL_SQUARE)) != 0;
            if (failure_black_frames >= 90 && face_down && !failure_exit_held) {
                log_line("GAME_CHANGE=failure_face_button_return_to_launcher");
                runner->shouldExit = true;
            } else if (failure_black_frames >= 2100) {
                log_line("GAME_CHANGE=failure_timeout_return_to_launcher");
                runner->shouldExit = true;
            }
            failure_exit_held = face_down;
        } else {
            failure_black_frames = 0;
            failure_exit_held = false;
        }

        // The settings UI remains touch-capable even when gameplay touch is
        // disabled. Vita touch coordinates are twice the 960x544 UI space.
        if (settings.open) {
            SceTouchData menu_touch = {0};
            bool touching = sceTouchPeek(SCE_TOUCH_PORT_FRONT, &menu_touch, 1) > 0 &&
                            menu_touch.reportNum > 0;
            if (touching && settings.trophiesMenuOpen) {
                float ty = (float)menu_touch.report[0].y * 0.5f;
                if (!settings_touch_held) {
                    settings_touch_last_y = ty;
                    if (ty >= 190.0f && ty <= 420.0f) {
                        int row = (int)((ty - 190.0f) / 72.0f);
                        int item = settings.trophiesScroll + row;
                        if (row >= 0 && row < 3 && item < 30) settings.trophiesSelected = item;
                    }
                } else {
                    float delta = ty - settings_touch_last_y;
                    if (delta <= -34.0f) {
                        settings.trophiesSelected = (settings.trophiesSelected + 1) % 30;
                        settings_touch_last_y = ty;
                    } else if (delta >= 34.0f) {
                        settings.trophiesSelected = (settings.trophiesSelected + 29) % 30;
                        settings_touch_last_y = ty;
                    }
                }
            } else if (touching && settings.controlEditMode) {
                float tx = (float)menu_touch.report[0].x * 0.5f;
                float ty = (float)menu_touch.report[0].y * 0.5f;
                if (!settings_touch_held) {
                    int best = 0;
                    int bestDistance = 0x7fffffff;
                    for (int i = 0; i < 4; ++i) {
                        int dx = (int)tx - settings.touchControlX[i];
                        int dy = (int)ty - settings.touchControlY[i];
                        int distance = dx * dx + dy * dy;
                        if (distance < bestDistance) { best = i; bestDistance = distance; }
                    }
                    settings.selectedTouchControl = best;
                }
                settings.touchControlX[settings.selectedTouchControl] = (int)tx;
                settings.touchControlY[settings.selectedTouchControl] = (int)ty;
            } else if (touching) {
                float tx = (float)menu_touch.report[0].x * 0.5f;
                float ty = (float)menu_touch.report[0].y * 0.5f;
                if (!settings_touch_held && ty >= 118.0f && ty <= 166.0f && tx >= 120.0f && tx <= 840.0f) {
                    int categoryCount = settings.devMode ? 5 : 4;
                    float categorySpacing = categoryCount == 5 ? 140.0f : 180.0f;
                    float categoryStart = 480.0f - categorySpacing * (categoryCount - 1) * 0.5f;
                    int category = (int)(((tx - categoryStart) / categorySpacing) + 0.5f);
                    if (category < 0) category = 0;
                    if (category >= categoryCount) category = categoryCount - 1;
                    settings.category = category;
                    settings.selected = 0;
                } else if (tx >= 180.0f && tx <= 780.0f && ty >= 168.0f && ty <= 420.0f) {
                    int count = VitaSettings_itemCount(settings.category);
                    float firstY = count == 5 ? 184.0f : (count == 4 ? 200.0f : (count == 3 ? 206.0f : 214.0f));
                    float spacing = count == 5 ? 38.0f : (count == 4 ? 42.0f : (count == 3 ? 50.0f : 64.0f));
                    int item = (int)((ty - firstY + spacing * 0.5f) / spacing);
                    if (item >= 0 && item < count) {
                        settings.selected = item;
                        int logicalCategory = settings.category + (!settings.devMode ? 1 : 0);
                        int dataIndex = logicalCategory == 2 ?
                            (item >= 3 && settings.consoleBorderMode != 1 ? item + 1 : item) : item;
                        bool slider = (logicalCategory == 3 && dataIndex < 3) ||
                                      (logicalCategory == 2 && dataIndex == 4);
                        if (slider) {
                            VitaSettings_setSliderFromTouch(&settings, audio,
                                                            logicalCategory, dataIndex, tx);
                        } else if (!settings_touch_held) {
                            // Route buttons through the regular input path so
                            // persistence and SFX remain identical to X.
                            pad.buttons |= SCE_CTRL_CROSS;
                        }
                    }
                }
            }
            settings_touch_held = touching;
        } else {
            settings_touch_held = false;
        }

        // Dev Mode quick room navigation: devmode=1 is enough. Hold Circle
        // and tap L/R to move through the real GEN8 room order, matching the
        // desktop debug runner's previous/next room semantics. Consume the
        // full combo so Circle/PageUp/PageDown never leak into game input.
        bool circle_room_nav = (pad.buttons & SCE_CTRL_CIRCLE) != 0 &&
                               ((pad.buttons & SCE_CTRL_LTRIGGER) != 0 ||
                                (pad.buttons & SCE_CTRL_RTRIGGER) != 0);
        if (!settings.open && settings.devMode && circle_room_nav) {
            bool l = (pad.buttons & SCE_CTRL_LTRIGGER) != 0;
            bool r = (pad.buttons & SCE_CTRL_RTRIGGER) != 0;
            if (!dev_room_shortcut_held && l != r && runner->pendingRoom == -1) {
                int direction = l ? -1 : 1;
                int32_t targetOrderPosition = vita_dev_room_order_target(
                    runner->currentRoomOrderPosition, dw->gen8.roomOrderCount, direction);
                if (targetOrderPosition >= 0) {
                    int32_t targetRoom = dw->gen8.roomOrder[targetOrderPosition];
                    if (targetRoom >= 0 && (uint32_t)targetRoom < dw->room.count) {
                        const char* targetName = dw->room.rooms[targetRoom].name;
                        char jump[256];
                        snprintf(jump, sizeof(jump),
                                 "DEV_ROOM_SHORTCUT combo=O+%c from=%d to=%d name=%s order=%d->%d",
                                 l ? 'L' : 'R', runner->currentRoomIndex, targetRoom,
                                 targetName != NULL ? targetName : "<null>",
                                 runner->currentRoomOrderPosition, targetOrderPosition);
                        log_line(jump);
                        dev_log_write(jump);
                        runner->pendingRoom = targetRoom;
                    }
                }
            }
            dev_room_shortcut_held = true;
            pad.buttons &= ~(SCE_CTRL_CIRCLE | SCE_CTRL_LTRIGGER | SCE_CTRL_RTRIGGER);
        } else {
            dev_room_shortcut_held = false;
        }

        if (!settings.open && settings.devMode && settings.devRoomNavEnabled) {
            bool confirmRoom = (pad.buttons & SCE_CTRL_LTRIGGER) &&
                               (pad.buttons & SCE_CTRL_CROSS);
            bool navInput = (pad.buttons & (SCE_CTRL_UP | SCE_CTRL_DOWN |
                                            SCE_CTRL_LEFT | SCE_CTRL_RIGHT)) != 0 ||
                            confirmRoom;
            if (navInput && !dev_room_nav_held) {
                if (pad.buttons & SCE_CTRL_UP) dev_room_target--;
                else if (pad.buttons & SCE_CTRL_DOWN) dev_room_target++;
                else if (pad.buttons & SCE_CTRL_LEFT) dev_room_target -= 10;
                else if (pad.buttons & SCE_CTRL_RIGHT) dev_room_target += 10;
                while (dev_room_target < 0) dev_room_target += (int32_t)dw->room.count;
                while ((uint32_t)dev_room_target >= dw->room.count)
                    dev_room_target -= (int32_t)dw->room.count;
                if (confirmRoom) {
                    const char* targetName = dw->room.rooms[dev_room_target].name;
                    char jump[256];
                    snprintf(jump, sizeof(jump), "DEV_ROOM_JUMP from=%d to=%d name=%s",
                             runner->currentRoomIndex, dev_room_target,
                             targetName != NULL ? targetName : "<null>");
                    log_line(jump);
                    dev_log_write(jump);
                    runner->pendingRoom = dev_room_target;
                    dev_force_move = true;
                }
            }
            dev_room_nav_held = navInput;
            // Debug navigation must not leak into the game controls.
            pad.buttons &= ~(SCE_CTRL_UP | SCE_CTRL_DOWN |
                             SCE_CTRL_LEFT | SCE_CTRL_RIGHT);
            if (confirmRoom)
                pad.buttons &= ~(SCE_CTRL_LTRIGGER | SCE_CTRL_CROSS);
        } else {
            dev_room_nav_held = false;
            if (!settings.devMode || !settings.devRoomNavEnabled) {
                dev_room_target = runner->currentRoomIndex;
                dev_force_move = false;
            }
        }

        if (!settings.open && settings.debugCollisionMasks) {
            SceTouchData game_touch = {0};
            if (sceTouchPeek(SCE_TOUCH_PORT_FRONT, &game_touch, 1) > 0 && game_touch.reportNum > 0) {
                float sx = (float)game_touch.report[0].x * (640.0f / 1920.0f);
                float sy = (float)game_touch.report[0].y * (480.0f / 1080.0f);
                float wx = sx + (float)runner->defaultCameras[0].viewX;
                float wy = sy + (float)runner->defaultCameras[0].viewY;
                DebugOverlay_selectInstanceAtPoint(runner, wx, wy);
            }
            static bool l_trigger_was_held = false;
            bool l_pressed = (pad.buttons & SCE_CTRL_LTRIGGER) != 0;
            if (l_pressed) {
                static bool obj_inspect_held = false;
                bool navInput = (pad.buttons & (SCE_CTRL_LEFT | SCE_CTRL_RIGHT | SCE_CTRL_UP | SCE_CTRL_DOWN | SCE_CTRL_SQUARE)) != 0;
                if (navInput && !obj_inspect_held) {
                    if (pad.buttons & (SCE_CTRL_LEFT | SCE_CTRL_UP)) DebugOverlay_selectNextInstance(runner, -1);
                    else if (pad.buttons & (SCE_CTRL_RIGHT | SCE_CTRL_DOWN | SCE_CTRL_SQUARE)) DebugOverlay_selectNextInstance(runner, 1);
                }
                obj_inspect_held = navInput;
                // Prevent L + D-Pad object inspection inputs from moving the player character
                pad.buttons &= ~(SCE_CTRL_LTRIGGER | SCE_CTRL_UP | SCE_CTRL_DOWN | SCE_CTRL_LEFT | SCE_CTRL_RIGHT | SCE_CTRL_SQUARE | SCE_CTRL_CROSS);
            } else if (l_trigger_was_held) {
                // Save object log on releasing L-TRIGGER
                DebugOverlay_saveSelectedObjectLog(runner, active_chapter);
            }
            l_trigger_was_held = l_pressed;
        }
        
        bool r_trigger = (pad.buttons & SCE_CTRL_RTRIGGER);
        bool border_cycled = false;
        if (!settings.open && r_trigger && settings.devMode) {
            if ((pad.buttons & (SCE_CTRL_RIGHT | SCE_CTRL_DOWN))) {
                if (!border_cycle_dpad_held) {
                    VitaBorders_cycleCurrent(1);
                    border_cycle_dpad_held = true;
                }
                border_cycled = true;
            } else if ((pad.buttons & (SCE_CTRL_LEFT | SCE_CTRL_UP))) {
                if (!border_cycle_dpad_held) {
                    VitaBorders_cycleCurrent(-1);
                    border_cycle_dpad_held = true;
                }
                border_cycled = true;
            } else {
                border_cycle_dpad_held = false;
            }
        } else {
            border_cycle_dpad_held = false;
        }

        if (border_cycled) {
            // Mask the complete shortcut so neither movement nor PageUp leaks
            // into DELTARUNE while a border is being selected.
            pad.buttons &= ~(SCE_CTRL_LEFT | SCE_CTRL_RIGHT | SCE_CTRL_UP |
                             SCE_CTRL_DOWN | SCE_CTRL_RTRIGGER);
        }
        
        // SELECT opens the native Vita settings overlay. Circle consistently
        // acts as Back there and closing changes that require it restarts the
        // runner through the normal, clean path below.
        // Select belongs to Void Stranger as Escape/Back. Intercepting it for
        // the inherited native settings overlay made Back/Resume and several
        // in-game settings appear unresponsive.
        bool restart_for_settings = false;
        if (settings.debugDevChanged) {
            const char* room = runner->currentRoom != NULL ? runner->currentRoom->name : NULL;
            if (settings.debugDevEnabled) {
                dev_log_start(room);
                Profiler_setEnabled(&runner->vmContext->profiler, true);
            } else {
                dev_log_stop();
                Profiler_setEnabled(&runner->vmContext->profiler, false);
            }
            settings.debugDevChanged = false;
        }
        if (restart_for_settings) {
            int settings_target = settings.returnToChapterSelect ? 0 : active_chapter;
            log_line(settings.returnToChapterSelect ? "SETTINGS=return_to_chapter_select" : "SETTINGS=restart_for_mod");
            if (restart_into_chapter(settings_target)) {
                // Release the OpenAL device/SceAudioOut port before the process is
                // replaced. Without this, the next process's alcOpenDevice() can
                // crash inside alcResetDeviceSOFT on the still-open hardware port.
                audio->vtable->destroy(audio);
                sceKernelDelayThread(500000);
                sceKernelExitProcess(0);
            }
        }
        int dx = (int)pad.lx - 128, dy = (int)pad.ly - 128;
        bool fruit_can_eat = !settings.open &&
            runner_object_bool_variable(runner, "obj_orange", "can_eat");
        SceTouchData fruit_touch = {0};
        bool fruit_touching = false;
        if (fruit_can_eat && sceTouchPeek(SCE_TOUCH_PORT_FRONT, &fruit_touch, 1) > 0)
            fruit_touching = fruit_touch.reportNum > 0;
        bool fruit_touch_confirm = fruit_touch_should_pulse(
            fruit_can_eat, fruit_touching, fruit_touch_held);
        fruit_touch_held = fruit_can_eat ? fruit_touching : false;

        SceTouchData touch = {0};
        bool touch_up = false, touch_down = false, touch_left = false, touch_right = false;
        float touch_axis_x = 0.0f, touch_axis_y = 0.0f;
        bool touch_confirm = false, touch_cancel = false, touch_menu = false;
        if (settings.touchEnabled && !settings.open && sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1) > 0) {
            for (unsigned i = 0; i < touch.reportNum; ++i) {
                int tx = touch.report[i].x;
                int ty = touch.report[i].y;
                int stickCenterX = settings.touchControlX[0] * 2;
                int stickCenterY = settings.touchControlY[0] * 2;
                int stickRadius = 210 * settings.touchControlScale[0] / 100;
                int interactRadius = stickRadius * 3; // Much more tolerant
                int stickDx = tx - stickCenterX;
                int stickDy = ty - stickCenterY;
                if (stickDx * stickDx + stickDy * stickDy <= interactRadius * interactRadius) {
                    int tdx = stickDx;
                    int tdy = stickDy;
                    if (tdx < -stickRadius) tdx = -stickRadius;
                    if (tdx > stickRadius) tdx = stickRadius;
                    if (tdy < -stickRadius) tdy = -stickRadius;
                    if (tdy > stickRadius) tdy = stickRadius;
                    touch_axis_x = (float)tdx / (float)stickRadius;
                    touch_axis_y = (float)tdy / (float)stickRadius;
                    if (abs(tdx) > abs(tdy)) {
                        touch_left |= tdx < -(stickRadius * 3 / 7);
                        touch_right |= tdx > (stickRadius * 3 / 7);
                    } else {
                        touch_up |= tdy < -(stickRadius * 3 / 7);
                        touch_down |= tdy > (stickRadius * 3 / 7);
                    }
                } else {
                    // Front-touch coordinates are 1920x1088 while the overlay
                    // is drawn at 960x544. Match the exact on-screen centers of
                    // Z (850,385), X (755,455) and C (755,340). The former
                    // horizontal bands made C and X share the same hit region.
                    const int centersX[3] = {settings.touchControlX[1] * 2, settings.touchControlX[2] * 2, settings.touchControlX[3] * 2};
                    const int centersY[3] = {settings.touchControlY[1] * 2, settings.touchControlY[2] * 2, settings.touchControlY[3] * 2};
                    int best = -1;
                    int bestDistance = 0x7fffffff;
                    for (int button = 0; button < 3; ++button) {
                        int hitRadius = 180 * settings.touchControlScale[button + 1] / 100;
                        int hitRadiusSquared = hitRadius * hitRadius;
                        int bdx = tx - centersX[button];
                        int bdy = ty - centersY[button];
                        int distance = bdx * bdx + bdy * bdy;
                        if (distance <= hitRadiusSquared && distance < bestDistance) {
                            best = button;
                            bestDistance = distance;
                        }
                    }
                    if (best == 0) touch_confirm = true;
                    else if (best == 1) touch_cancel = true;
                    else if (best == 2) touch_menu = true;
                }
            }
        }
        float visual_x = dx < -24 || dx > 24 ? (float)dx / 127.0f : touch_axis_x;
        float visual_y = dy < -24 || dy > 24 ? (float)dy / 127.0f : touch_axis_y;
        VitaSettings_setTouchVisuals(&settings, visual_x, visual_y,
                                     (pad.buttons & SCE_CTRL_CROSS) || touch_confirm,
                                     (pad.buttons & (SCE_CTRL_CIRCLE | SCE_CTRL_SQUARE)) || touch_cancel,
                                     (pad.buttons & SCE_CTRL_TRIANGLE) || touch_menu);
        for (ptrdiff_t i = 0; i < arrlen(runner->instances); ++i) {
            Instance* inst = runner->instances[i];
            if (inst == NULL || inst->objectIndex < 0 || (uint32_t)inst->objectIndex >= dw->objt.count) continue;
            const char* object_name = dw->objt.objects[inst->objectIndex].name;
            if (object_name != NULL && strcmp(object_name, "obj_mobilecontroller") == 0) {
                // The Android controller uses its own fixed coordinates and
                // fights the Vita editor by snapping a second joystick back
                // to the original layout.  The Vita overlay already draws the
                // same game sprites at the edited coordinates.
                inst->visible = false;
            }
        }

        // Ignore stale pad/touch state inherited from the loader. Require a
        // short neutral period before exposing any gameplay key edge; without
        // this, Cross could be reported as a fresh Z/X press on the first room.
        bool gameplay_input_neutral =
            (pad.buttons & (SCE_CTRL_UP | SCE_CTRL_DOWN | SCE_CTRL_LEFT | SCE_CTRL_RIGHT |
                            SCE_CTRL_CROSS | SCE_CTRL_CIRCLE | SCE_CTRL_SQUARE |
                            SCE_CTRL_TRIANGLE | SCE_CTRL_START | SCE_CTRL_SELECT |
                            SCE_CTRL_LTRIGGER | SCE_CTRL_RTRIGGER)) == 0 &&
            dx >= -48 && dx <= 48 && dy >= -48 && dy <= 48 &&
            !touch_up && !touch_down && !touch_left && !touch_right &&
            !touch_confirm && !touch_cancel && !touch_menu;
        if (!gameplay_input_armed) {
            gameplay_input_neutral_frames = gameplay_input_neutral ?
                gameplay_input_neutral_frames + 1 : 0;
            if (gameplay_input_neutral_frames >= 3) {
                gameplay_input_armed = true;
                log_line("INPUT=armed_after_neutral");
            }
        }
        bool controls_enabled = gameplay_input_armed && !settings.open &&
                                !settings.adjustMode && settings.inputCooldown == 0;
        
        bool gp_up = controls_enabled && !dev_force_move && ((pad.buttons & SCE_CTRL_UP) || dy < -48 || touch_up);
        bool gp_down = controls_enabled && !dev_force_move && ((pad.buttons & SCE_CTRL_DOWN) || dy > 48 || touch_down);
        bool gp_left = controls_enabled && !dev_force_move && ((pad.buttons & SCE_CTRL_LEFT) || dx < -48 || touch_left);
        bool gp_right = controls_enabled && !dev_force_move && ((pad.buttons & SCE_CTRL_RIGHT) || dx > 48 || touch_right);
        if (!trophy_first_move && (gp_up || gp_down || gp_left || gp_right)) {
            trophy_first_move = true;
            trophy_event_unlock(6, "First Steps", "first movement");
        }
        RunnerGamepad_setButton(runner->gamepads, 0, GP_PADU, gp_up, &gp_previous[0]);
        RunnerGamepad_setButton(runner->gamepads, 0, GP_PADD, gp_down, &gp_previous[1]);
        RunnerGamepad_setButton(runner->gamepads, 0, GP_PADL, gp_left, &gp_previous[2]);
        RunnerGamepad_setButton(runner->gamepads, 0, GP_PADR, gp_right, &gp_previous[3]);
        
        bool gp_cross = controls_enabled &&
            ((pad.buttons & SCE_CTRL_CROSS) || touch_confirm || fruit_touch_confirm);
        RunnerGamepad_setButton(runner->gamepads, 0, GP_FACE1, gp_cross, &gp_previous[4]);
        
        bool gp_circle = controls_enabled && ((pad.buttons & SCE_CTRL_CIRCLE) || touch_cancel);
        RunnerGamepad_setButton(runner->gamepads, 0, GP_FACE2, gp_circle, &gp_previous[5]);

        // Keep every Vita face button distinct. Void Stranger uses gp_face3
        // (Square) for the secondary action/staff; merging it into Circle made
        // the staff animation run without placing a tile.
        bool gp_square = controls_enabled && (pad.buttons & SCE_CTRL_SQUARE);
        RunnerGamepad_setButton(runner->gamepads, 0, GP_FACE3, gp_square, &gp_previous[6]);
        
        bool gp_triangle = controls_enabled && ((pad.buttons & SCE_CTRL_TRIANGLE) || touch_menu);
        RunnerGamepad_setButton(runner->gamepads, 0, GP_FACE4, gp_triangle, &gp_previous[7]);
        
        bool gp_l = controls_enabled && (pad.buttons & SCE_CTRL_LTRIGGER);
        RunnerGamepad_setButton(runner->gamepads, 0, GP_SHOULDERL, gp_l, &gp_previous[8]);
        
        bool gp_r = controls_enabled && (pad.buttons & SCE_CTRL_RTRIGGER);
        RunnerGamepad_setButton(runner->gamepads, 0, GP_SHOULDERR, gp_r, &gp_previous[9]);

        RunnerGamepad_setAxis(runner->gamepads, 0, GP_AXIS_LH, visual_x);
        RunnerGamepad_setAxis(runner->gamepads, 0, GP_AXIS_LV, visual_y);

        // Void Stranger still has keyboard-only movement paths in a few rooms
        // and menu states. Mirror the unified Vita direction state to arrows as
        // a compatibility fallback, while the GML controller path remains the
        // preferred source and keeps the UI in controller mode.
        set_key(runner->keyboard, VK_UP, gp_up, &previous[0]);
        set_key(runner->keyboard, VK_DOWN, gp_down, &previous[1]);
        set_key(runner->keyboard, VK_LEFT, gp_left, &previous[2]);
        set_key(runner->keyboard, VK_RIGHT, gp_right, &previous[3]);        // Log only movement state transitions. This is intentionally not a
        // per-frame trace, so it remains useful on retail hardware without
        // recreating the old probe-I/O frametime regression.
        {
            static uint32_t lastMoveButtons = UINT32_MAX;
            static int lastAnalogX = 99;
            static int lastAnalogY = 99;
            uint32_t moveButtons = pad.buttons &
                (SCE_CTRL_UP | SCE_CTRL_DOWN | SCE_CTRL_LEFT | SCE_CTRL_RIGHT);
            int analogX = dx < -48 ? -1 : (dx > 48 ? 1 : 0);
            int analogY = dy < -48 ? -1 : (dy > 48 ? 1 : 0);
            if (moveButtons != lastMoveButtons || analogX != lastAnalogX || analogY != lastAnalogY) {
                char inputLog[256];
                snprintf(inputLog, sizeof(inputLog),
                         "INPUT_RAW dpad=0x%08X analog=%d,%d gp=%d%d%d%d axis=%.2f,%.2f key=%d%d%d%d",
                         (unsigned int)moveButtons, analogX, analogY,
                         RunnerGamepad_buttonCheck(runner->gamepads, 0, GP_PADU) ? 1 : 0,
                         RunnerGamepad_buttonCheck(runner->gamepads, 0, GP_PADD) ? 1 : 0,
                         RunnerGamepad_buttonCheck(runner->gamepads, 0, GP_PADL) ? 1 : 0,
                         RunnerGamepad_buttonCheck(runner->gamepads, 0, GP_PADR) ? 1 : 0,
                         RunnerGamepad_axisValue(runner->gamepads, 0, GP_AXIS_LH),
                         RunnerGamepad_axisValue(runner->gamepads, 0, GP_AXIS_LV),
                         gp_up ? 1 : 0, gp_down ? 1 : 0, gp_left ? 1 : 0, gp_right ? 1 : 0);
                log_line(inputLog);
                lastMoveButtons = moveButtons;
                lastAnalogX = analogX;
                lastAnalogY = analogY;
            }
        }
        // Both Vita face-button assignments confirm reliably. Square remains
        // the distinct X/staff action and Select remains Esc/back.
        set_key(runner->keyboard, 'Z', gp_cross, &previous[4]);
        set_key(runner->keyboard, 'X', gp_square, &previous[5]);
        
        if (settings.shortcutSkipDialogs) {
            set_key(runner->keyboard, 'C', gp_triangle, &previous[7]);
        } else {
            set_key_pulse(runner->keyboard, 'C', gp_triangle, &previous[7]);
        }
        
        set_key(runner->keyboard, VK_PAGEDOWN, gp_l, &previous[8]);
        set_key(runner->keyboard, VK_PAGEUP, gp_r, &previous[9]);
        bool key_start = controls_enabled && (pad.buttons & SCE_CTRL_START);
        bool key_select = controls_enabled && (pad.buttons & SCE_CTRL_SELECT);
        set_key(runner->keyboard, VK_ENTER, key_start, &previous[10]);
        set_key(runner->keyboard, VK_ESCAPE, key_select, &previous[11]);

        uint64_t frame_begin = sceKernelGetProcessTimeWide();
        uint64_t now = frame_begin;
        runner->deltaTime = (double)(now - last_time);
        last_time = now;
        uint64_t step_begin = sceKernelGetProcessTimeWide();
        bool profiler_sample_frame = false;
        if (runner->vmContext->profiler != NULL)
            Profiler_setSampling(runner->vmContext->profiler, profiler_sample_frame);
        if (profiler_sample_frame)
            Profiler_reset(runner->vmContext->profiler);
        if (!settings.open && !settings.adjustMode) {
            if (frame == 0) log_line("FRAME0=step_begin");
            Runner_step(runner);
            // DELTARUNE only enables its event-manager trophies on PS4/PS5.
            // Keep os_windows for launcher compatibility and bridge the same
            // event list to Vita's persistent trophy state instead.
            if ((frame % 30U) == 0U) {
                bool gameTrophies[30];
                Runner_enableGameTrophies(runner);
                Runner_getGameTrophies(runner, gameTrophies);
                uint32_t unlockedTrophies = VitaSettings_syncGameTrophies(&settings, gameTrophies);
                for (int trophy = 0; trophy < 30; ++trophy)
                    if ((unlockedTrophies & (1U << trophy)) != 0) VitaTrophies_unlock(trophy);
            }
            int video_event = VitaVideo_consumeEvent();
            if (video_event == VITA_VIDEO_EVENT_START) {
                // Preserve GameMaker's video contract.  DELTARUNE enables
                // obj_ch3_couch_video only after this async callback, then
                // calls video_draw() and draws the returned surface itself.
                // The Vita backend exposes its decoded frame through the
                // reserved VITA_VIDEO_SURFACE_ID handled by draw_surface().
                Runner_dispatchVideoAsync(runner, "video_start");
                log_line("VIDEO_ASYNC=video_start_dispatched");
            } else if (video_event == VITA_VIDEO_EVENT_END) {
                log_line("VIDEO_ASYNC=video_end");
                Runner_dispatchVideoAsync(runner, "video_end");
            }
            if (dev_force_move && settings.devMode && settings.devRoomNavEnabled && !(pad.buttons & SCE_CTRL_LTRIGGER)) {
                float moveX = dx < -32 || dx > 32 ? (float)dx / 32.0f :
                              ((pad.buttons & SCE_CTRL_LEFT) ? -4.0f : ((pad.buttons & SCE_CTRL_RIGHT) ? 4.0f : 0.0f));
                float moveY = dy < -32 || dy > 32 ? (float)dy / 32.0f :
                              ((pad.buttons & SCE_CTRL_UP) ? -4.0f : ((pad.buttons & SCE_CTRL_DOWN) ? 4.0f : 0.0f));
                if (moveX != 0.0f || moveY != 0.0f) {
                    for (ptrdiff_t i = 0; i < arrlen(runner->instances); ++i) {
                        Instance* inst = runner->instances[i];
                        if (inst == NULL || inst->objectIndex < 0 || (uint32_t)inst->objectIndex >= dw->objt.count) continue;
                        const char* objectName = dw->objt.objects[inst->objectIndex].name;
                        if (objectName != NULL && strcmp(objectName, "obj_mainchara") == 0) {
                            inst->active = true;
                            inst->visible = true;
                            inst->x += moveX;
                            inst->y += moveY;
                            SpatialGrid_markInstanceAsDirty(runner->spatialGrid, inst);
                            break;
                        }
                    }
                }
            }
            if (frame == 0) log_line("FRAME0=step_complete");
        }
        uint64_t step_us = sceKernelGetProcessTimeWide() - step_begin;
        if (runner->pendingWorkingDirectory != NULL) {
            int requested_chapter = chapter_from_request(runner->pendingWorkingDirectory);
            char line[160];
            snprintf(line, sizeof(line), "GAME_CHANGE=request cwd=%s params=%s parsed=%d",
                     runner->pendingWorkingDirectory,
                     runner->pendingLaunchParameters ? runner->pendingLaunchParameters : "<null>",
                     requested_chapter);
            log_line(line);
            if (requested_chapter >= 0 && requested_chapter <= 5) {
                next_chapter = requested_chapter;
                if (restart_into_chapter(next_chapter)) {
                    audio->vtable->destroy(audio);
                    sceKernelDelayThread(500000);
                    sceKernelExitProcess(0);
                }
                log_line("GAME_CHANGE=loadexec_failed");
            } else if (active_chapter > 0) {
                next_chapter = 0;
                log_line("GAME_CHANGE=chapter_select_fallback_launcher");
                if (restart_into_chapter(0)) {
                    audio->vtable->destroy(audio);
                    sceKernelDelayThread(500000);
                    sceKernelExitProcess(0);
                }
                log_line("GAME_CHANGE=launcher_loadexec_failed");
            } else log_line("GAME_CHANGE=invalid_chapter");
            exit_requested = true;
            continue;
        }
        uint64_t audio_begin = sceKernelGetProcessTimeWide();
        runner->audioSystem->vtable->update(runner->audioSystem, (float)(runner->deltaTime / 1000000.0));
        uint64_t audio_us = sceKernelGetProcessTimeWide() - audio_begin;

        uint64_t render_begin = sceKernelGetProcessTimeWide();
        uint64_t render_phase_begin = render_begin;
        uint64_t render_pre_us = 0, render_begin_frame_us = 0;
        uint64_t render_views_us = 0, render_composite_us = 0;
        uint64_t render_gui_us = 0, render_overlay_us = 0, render_swap_us = 0;
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        int game_w = runner->applicationWidth > 0 ? runner->applicationWidth : (int)dw->gen8.defaultWindowWidth;
        int game_h = runner->applicationHeight > 0 ? runner->applicationHeight : (int)dw->gen8.defaultWindowHeight;
        /* Preserve Void Stranger's complete 896x576 logical canvas. Several
         * effects and surfaces use absolute Steam-window coordinates; reducing
         * the application surface crops those effects and looks like a strong
         * zoom. Only the final compositor scales it into the Vita display. */
        int render_w = game_w;
        int render_h = game_h;
        runner->widescreenExtraWidth = 0;
        runner->widescreenExtraHeight = 0;
        if (game_w != logged_game_w || game_h != logged_game_h) {
            char runtime_display[128];
            snprintf(runtime_display, sizeof(runtime_display),
                     "DISPLAY=runtime_%dx%d host_960x544 centered", game_w, game_h);
            log_line(runtime_display);
            logged_game_w = game_w;
            logged_game_h = game_h;
        }
        if (frame == 0) log_line("FRAME0=draw_pre_begin");
        Runner_drawPre(runner, 960, 544);
        render_pre_us = sceKernelGetProcessTimeWide() - render_phase_begin;
        render_phase_begin = sceKernelGetProcessTimeWide();
        if (frame == 0) log_line("FRAME0=draw_pre_complete");
        Runner_beginFrame(runner, render_w, render_h, 960, 544, 960, 544);
        render_begin_frame_us = sceKernelGetProcessTimeWide() - render_phase_begin;
        render_phase_begin = sceKernelGetProcessTimeWide();
        if (frame == 0) log_line("FRAME0=begin_frame_complete");
        Runner_drawViews(runner, render_w, render_h, settings.debugCollisionMasks);
        render_views_us = sceKernelGetProcessTimeWide() - render_phase_begin;
        render_phase_begin = sceKernelGetProcessTimeWide();
        if (settings.debugCollisionMasks && (frame % 30U) == 0U)
            log_puzzle_diagnostics(runner);
        if (dev_log_fd >= 0 && (frame % 30U) == 0U) {
            char shaderActivity[512];
            if (VMBuiltins_takeShaderDiagnostic(shaderActivity, sizeof(shaderActivity)))
                dev_log_write(shaderActivity);
        }
        if (frame == 0) log_line("FRAME0=draw_views_complete");
        VitaBorders_updateRoom(runner->currentRoom != NULL ? runner->currentRoom->name : NULL);
        // Keep the active gameplay border behind Game Settings. The overlay
        // does not replace or resize the game framebuffer, so suppressing the
        // border here only produced an unnecessary black flash. Calibration
        // still uses black margins while the user adjusts the viewport.
        // Do not submit the border in a frame that already requested a room
        // change. The subsequent cache trim may reclaim textures immediately;
        // keeping the border out of that unswapped frame avoids a GXM hazard.
        VitaBorders_setSuppressed(settings.adjustMode || runner->pendingRoom != -1);
        // Mixed Chapter 5 pages contain characters, fonts and scenery. They
        // cannot be downscaled or evicted without corruption or an immediate
        // reload. Prefer black side areas when this mandatory set is already
        // too close to the physical graphics-memory limit.
        uint64_t borderPressureMiB = g_vitaActiveChapter == 5 ? 44ULL : 105ULL;
        size_t borderVramFree = vglMemFree(VGL_MEM_VRAM);
        size_t borderMappedFree = borderVramFree +
                                  vglMemFree(VGL_MEM_RAM) +
                                  vglMemFree(VGL_MEM_SLOW);
        uint64_t borderResidentBytes = g_vitaModernGlActive ?
            ((GLRenderer*)renderer)->residentTextureBytes :
            ((GLLegacyRenderer*)renderer)->residentTextureBytes;
        VitaBorders_setMemoryPressure(
            borderResidentBytes > borderPressureMiB * 1024ULL * 1024ULL ||
            borderMappedFree < 2ULL * 1024ULL * 1024ULL);
        // Border textures and room surfaces are outside residentTextureBytes.
        // Retire atlases left by PLACE_MENU before the first border frame so
        // Chapter 5 keeps headroom for its framebuffer and border allocation.
        // Pinned fonts and dialogue pages survive this stale-page trim.
        if (VitaBorders_needsMemoryTrim() && !g_vitaModernGlActive) {
            GLLegacyRenderer* borderLegacy = (GLLegacyRenderer*)renderer;
            // Match the Chapter 5 renderer ceiling; borders and five room
            // surfaces live outside its resident-atlas accounting.
            // Use the same ceiling as the Chapter 2 renderer. The previous
            // 88 MiB border preparation repeatedly discarded page 2/8 while
            // the border was waiting to upload, causing visible room art to
            // vanish and be recreated on following frames.
            uint64_t borderTrimMiB = g_vitaActiveChapter == 5 ? 44ULL :
                (g_vitaActiveChapter == 3 ? 80ULL :
                 (g_vitaActiveChapter == 2 ? 104ULL : 88ULL));
            GLLegacyRenderer_trimTextureCacheForRoomChange(
                (GLLegacyRenderer*)renderer,
                borderTrimMiB * 1024ULL * 1024ULL, false);
            VitaBorders_setMemoryPressure(
                borderLegacy->residentTextureBytes > borderPressureMiB * 1024ULL * 1024ULL ||
                vglMemFree(VGL_MEM_VRAM) + vglMemFree(VGL_MEM_RAM) +
                    vglMemFree(VGL_MEM_SLOW) < 8ULL * 1024ULL * 1024ULL);
            char borderTrimLog[128];
            snprintf(borderTrimLog, sizeof(borderTrimLog),
                     "CONSOLE_BORDER=preload_texture_trim target_mb=%llu resident=%llu",
                     (unsigned long long)borderTrimMiB,
                     (unsigned long long)borderLegacy->residentTextureBytes);
            log_line(borderTrimLog);
        }
        /* Loading/settings overlays temporarily request a full-screen host
         * viewport.  Never let that state leak into the application-surface
         * composite after a save/room load, which looked like the game
         * shrinking once when the first normal frame was presented. */
        g_vitaPortOverlayFullScreen = 0;
        renderer->vtable->endFrameInit(renderer);
        if (frame == 0) log_line("FRAME0=end_frame_init_complete");
        if (frame == 0) log_line("FRAME0=draw_post_begin");
        Runner_drawPost(runner, 960, 544);
        if (frame == 0) log_line("FRAME0=draw_post_complete");
        renderer->vtable->endFrameEnd(renderer);
        render_composite_us = sceKernelGetProcessTimeWide() - render_phase_begin;
        render_phase_begin = sceKernelGetProcessTimeWide();
        if (frame == 0) log_line("FRAME0=end_frame_end_complete");
        Runner_drawGUI(runner, 960, 544, render_w, render_h);
        render_gui_us = sceKernelGetProcessTimeWide() - render_phase_begin;
        render_phase_begin = sceKernelGetProcessTimeWide();
        if (frame == 0) log_line("FRAME0=draw_gui_complete");
        // Advance SceAvPlayer even while it is preparing its first frame.
        // Once video_start has been dispatched, DELTARUNE presents the frame
        // through video_draw()/draw_surface() in its normal Draw event.
        VitaVideo_updateFrame();
        // Apply after the game, GUI and console border have been composited,
        // but before Game Settings so the brightness control remains readable.
        VitaSettings_drawBrightness(&settings, renderer);
        bool fruit_hint_visible_now = fruit_touch_hint_visible(
            runner_object_bool_variable(runner, "obj_orange", "can_eat"));
        VitaSettings_drawFruitTouchHint(&settings, renderer, fruit_hint_visible_now);
        VitaSettings_drawTouchControls(&settings, renderer);
        VitaSettings_updateTrophies(&settings);
        VitaSettings_drawTrophyNotification(&settings, renderer);
        if (frame == 0) log_line("FRAME0=touch_overlay_complete");
        /* No Deltarune Game Settings overlay in Void Stranger. */
        VitaSettings_drawCalibration(&settings, renderer);
        uint64_t rendererGpuBytes = 0;
        uint32_t rendererEvictions = 0, rendererDeferred = 0, rendererRamHits = 0;
        uint32_t rendererPrimitives = 0, rendererFlushes = 0;
        GLLegacyRenderer* legacy = (GLLegacyRenderer*)renderer;
        if (g_vitaModernGlActive) {
            GLRenderer* modern = (GLRenderer*)renderer;
            rendererGpuBytes = modern->residentTextureBytes;
            rendererEvictions = modern->vitaTextureEvictions;
            rendererDeferred = modern->vitaTextureDeferred;
            rendererRamHits = modern->vitaTextureRamHits;
            rendererPrimitives = modern->frameSubmittedPrimitives;
            rendererFlushes = modern->frameFlushCount;
        } else {
            rendererGpuBytes = legacy->residentTextureBytes;
            rendererEvictions = legacy->vitaTextureEvictions;
            rendererDeferred = legacy->vitaTextureDeferred;
            rendererRamHits = legacy->vitaTextureRamHits;
            rendererPrimitives = legacy->frameSubmittedPrimitives;
            rendererFlushes = legacy->frameFlushCount;
        }
        uint64_t dev_total_us = sceKernelGetProcessTimeWide() - frame_begin;
        uint64_t dev_render_us = sceKernelGetProcessTimeWide() - render_begin;
        float dev_fps = dev_total_us > 0 ? 1000000.0f / (float)dev_total_us : 0.0f;
        VitaSettings_drawDevOverlay(&settings, renderer,
            runner->currentRoom != NULL ? runner->currentRoom->name : NULL,
            dev_fps, step_us, audio_us, dev_render_us, rendererGpuBytes,
            rendererEvictions, rendererDeferred, rendererRamHits,
            dev_room_target >= 0 && (uint32_t)dev_room_target < dw->room.count ? dw->room.rooms[dev_room_target].name : NULL,
            dev_room_target);
        render_overlay_us = sceKernelGetProcessTimeWide() - render_phase_begin;
        if (frame == 0) log_line("FRAME0=calibration_overlay_complete");
        if (save_load_fade) {
            uint64_t fade_now = sceKernelGetProcessTimeWide();
            uint64_t elapsed = fade_now - save_load_fade_start;
            float progress = (float)elapsed / (float)save_load_fade_duration;
            if (progress >= 1.0f) {
                progress = 1.0f;
                save_load_fade = false;
                log_line("SAVE_LOAD_FADE=complete");
            }
            AlAudioSystem_setCategoryGains((AlAudioSystem*)audio,
                ((float)settings.musicVolume / 10.0f) * progress,
                (float)settings.sfxVolume / 10.0f);
            if (progress < 1.0f) {
                // Draw the fade in a fresh host-framebuffer GUI pass. Reusing
                // DELTARUNE's active 4:3 viewport produced a smaller black
                // rectangle offset inside the 960x544 Vita display.
                extern int g_vitaPortOverlayFullScreen;
                int previousOverlayMode = g_vitaPortOverlayFullScreen;
                g_vitaPortOverlayFullScreen = 1;
                renderer->vtable->beginGUI(renderer, 960, 544, 0, 0, 960, 544,
                                            RENDER_TARGET_HOST_FRAMEBUFFER);
                renderer->vtable->drawRectangle(renderer, 0.0f, 0.0f, 960.0f, 544.0f,
                                                 0x000000, 1.0f - progress, false);
                renderer->vtable->endGUI(renderer);
                g_vitaPortOverlayFullScreen = previousOverlayMode;
            }
        }
        draw_screen_filter(&settings, renderer);
        if (runner->pendingRoom == -1) {
            if (frame == 0) log_line("FRAME0=swap_begin");
            // Frame pacing below already enforces the game's 30 FPS cadence.
            // Waiting for VitaGL's 60 Hz swap as well quantized heavier Chapter
            // 5 scenes to ~50 ms (20 FPS). Submit immediately and keep a single
            // pacing mechanism instead of stacking two waits.
            uint64_t swap_begin = sceKernelGetProcessTimeWide();
            // vglSwapBuffers() takes "has common dialog", not a VSync flag.
            // Update the actual VitaGL swap interval here so a live change to
            // FPS Limit takes effect immediately: capped modes synchronize;
            // A 40 Hz presentation cadence cannot divide evenly into Vita's
            // 60 Hz VBlank. Waiting here produced an alternating 16/33 ms
            // cadence while the software limiter also tried to hold 25 ms.
            // Keep synchronized presentation for the exact 30/60 divisors;
            // 40 FPS uses only the precise software deadline below.
            // Do not stack VitaGL's VBlank wait with the monotonic limiter at
            // the end of this loop. The double wait quantized 30 FPS gameplay
            // to roughly 20 FPS whenever Step+Draw crossed one refresh slot.
            // At 60 FPS let VitaGL block on the display's VBlank. This yields
            // the CPU cleanly to the OpenAL output thread; the former software
            // deadline woke into a busy-spin tail every frame and the Vita
            // mixer could accept sources while producing no audible output.
            vglWaitVblankStart(g_vitaGameVsyncEnabled ? GL_TRUE : GL_FALSE);
            vglSwapBuffers(GL_FALSE);
            render_swap_us = sceKernelGetProcessTimeWide() - swap_begin;
            if (frame == 0) log_line("FRAME0=swap_complete");
            if (runner->vitaAudioTransitionWaitFrames > 0) {
                runner->vitaAudioTransitionWaitFrames--;
                if (runner->vitaAudioTransitionWaitFrames == 0) {
                    AlAudioSystem_setTransitionHold((AlAudioSystem*)audio, false);
                    log_line("ROOM_AUDIO=new_streams_released_after_presented_frame");
                }
            }
            if (audio_present_wait_frames > 0) {
                audio_present_wait_frames--;
                if (audio_present_wait_frames == 0) {
                    AlAudioSystem_setTransitionHold((AlAudioSystem*)audio, false);
                    if (startup_audio_paused && audio->vtable->resumeAll != NULL) {
                        audio->vtable->resumeAll(audio);
                        startup_audio_paused = false;
                        log_line("STARTUP_AUDIO=resumed_at_first_presented_frame");
                    }
                    // Restore category gains atomically. The former wall-clock
                    // fade could leave both categories at zero after the 60 Hz
                    // pacing change, producing a fully silent session.
                    AlAudioSystem_setCategoryGains((AlAudioSystem*)audio,
                        (float)settings.musicVolume / 10.0f,
                        (float)settings.sfxVolume / 10.0f);
                    save_load_fade = false;
                    log_line("STARTUP_AUDIO=gains_restored_after_present");
                }
            }
        }
        // Report after Step and Draw so GPU-heavy rooms identify the GML Draw
        // code that submitted their primitives. Previously this ran directly
        // after Step and therefore omitted every Draw event from GML_TOP.
        const char* previous_room_name = runner->currentRoom != NULL ? runner->currentRoom->name : NULL;
        bool leaving_save_menu = previous_room_name != NULL &&
            (strcmp(previous_room_name, "PLACE_MENU") == 0 ||
             strstr(previous_room_name, "PLACE_MENU") != NULL);
        int room_before_pending = runner->currentRoomIndex;
        bool starting_save_load = active_chapter > 0 && leaving_save_menu && runner->pendingRoom >= 0;
        const char* pending_room_name = runner->pendingRoom >= 0 &&
            (uint32_t)runner->pendingRoom < dw->room.count ? dw->room.rooms[runner->pendingRoom].name : NULL;
        bool entering_place_menu = active_chapter > 0 && pending_room_name != NULL &&
            strstr(pending_room_name, "PLACE_MENU") != NULL;
        bool queue_new_room_audio = active_chapter > 0 && runner->pendingRoom >= 0 &&
                                    !starting_save_load && !entering_place_menu;
        if (starting_save_load || entering_place_menu) {
            // Mute before Room End/room construction/Room Start. Music created by
            // the destination room therefore starts silent instead of playing a
            // short burst and being muted one frame later.
            AlAudioSystem_setCategoryGains((AlAudioSystem*)audio, 0.0f,
                                           (float)settings.sfxVolume / 10.0f);
            AlAudioSystem_setTransitionHold((AlAudioSystem*)audio, true);
            log_line("SAVE_LOAD_FADE=muted_before_room_change");
            
            // Clear the host framebuffer directly. Drawing this mask through
            // beginGUI inherited DELTARUNE's adjusted 4:3 viewport, so the
            // temporary black save-load frame appeared offset on the Vita.
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDisable(GL_SCISSOR_TEST);
            glViewport(0, 0, 960, 544);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            vglSwapBuffers(GL_FALSE);
        }
        if (queue_new_room_audio) {
            // Runner_handlePendingRoomChange owns this hold and its two-frame
            // release countdown. Setting transitionHold here first prevented
            // Runner from claiming it, leaving the hold enabled indefinitely;
            // later dialogue/SFX were queued but never started.
            log_line("ROOM_AUDIO=hold_delegated_to_runner");
        }
        Runner_handlePendingRoomChange(runner);
        if (!trophy_memories_opened && runner_has_active_object(runner, "obj_memories_album")) {
            trophy_memories_opened = true;
            trophy_event_unlock(19, "Memory Lane", "obj_memories_album");
        }
        // Runner_handlePendingRoomChange keeps destination streams queued
        // until two complete frames of the new room have reached the display.
        // Releasing here made music play during late texture uploads, underrun
        // under the black fade and restart from the beginning afterwards.
        if ((starting_save_load || entering_place_menu) && runner->currentRoomIndex != room_before_pending) {
            // Wait for the destination to render and swap once. Starting the
            // fade here lets room construction/first-use texture uploads consume
            // most of it, causing a short audible burst before PLACE_MENU.
            save_load_fade = false;
            // The first swapped frame can still be followed by late atlas
            // uploads. Keep music muted until three destination frames have
            // actually reached the display so it does not start, underrun and
            // restart during the transition.
            audio_present_wait_frames = 3;
            log_line(entering_place_menu ?
                     "PLACE_MENU_AUDIO=waiting_for_presented_frame" :
                     "SAVE_LOAD_FADE=waiting_for_presented_frame");
        }
        if (runner->profRoomTotalUs > 0) {
            char roomPerf[320];
            snprintf(roomPerf, sizeof(roomPerf),
                     "ROOM_LOAD_TIMING from=%d to=%d parse_us=%llu create_us=%llu end_events_us=%llu start_events_us=%llu cleanup_us=%llu total_us=%llu",
                     room_before_pending, runner->currentRoomIndex,
                     (unsigned long long)runner->profRoomParseUs,
                     (unsigned long long)runner->profRoomCreateUs,
                     (unsigned long long)runner->profRoomEndEventsUs,
                     (unsigned long long)runner->profRoomStartEventsUs,
                     (unsigned long long)runner->profRoomCleanupUs,
                     (unsigned long long)runner->profRoomTotalUs);
            log_line(roomPerf);
            dev_log_write(roomPerf);
        }

        uint64_t work_us = sceKernelGetProcessTimeWide() - frame_begin;
        uint64_t render_us = sceKernelGetProcessTimeWide() - render_begin;
        perf_total_us += work_us;
        if (work_us > perf_max_us) perf_max_us = work_us;
        perf_frames++;
        if (work_us > 40000) perf_drops++;
        if (work_us > 80000) perf_severe++;
        if (runner->currentRoomIndex != logged_room_index) {
            if (logged_room_index != -999 && !trophy_first_room_change) {
                trophy_first_room_change = true;
                trophy_event_unlock(18, "First Descent", "ROOM_CHANGE");
            }
            char room_change[192];
            snprintf(room_change, sizeof(room_change), "ROOM_CHANGE frame=%u from=%d to=%d name=%s work_us=%llu",
                     frame, logged_room_index, runner->currentRoomIndex,
                     runner->currentRoom && runner->currentRoom->name ? runner->currentRoom->name : "<null>",
                     (unsigned long long)work_us);
            log_line(room_change);
            dev_log_write(room_change);
            log_memory_snapshot("room_change_complete",
                                g_vitaModernGlActive ? NULL : (GLLegacyRenderer*)renderer);
            logged_room_index = runner->currentRoomIndex;
        }
        if (dev_log_fd >= 0 && (frame % 30U) == 0U) {
            char dev_sample[512];
            snprintf(dev_sample, sizeof(dev_sample),
                     "DEV_SAMPLE frame=%u room=%s fps=%.2f total_us=%llu step_us=%llu audio_us=%llu render_us=%llu gpu_bytes=%llu evictions=%u deferred=%u ram_hits=%u quads=%u flushes=%u masks=%d events_us=%llu alarms_us=%llu spatial_us=%llu collision_us=%llu other_us=%llu",
                     frame, runner->currentRoom && runner->currentRoom->name ? runner->currentRoom->name : "<null>",
                     work_us > 0 ? 1000000.0 / (double)work_us : 0.0,
                     (unsigned long long)work_us, (unsigned long long)step_us,
                     (unsigned long long)audio_us, (unsigned long long)render_us,
                     (unsigned long long)rendererGpuBytes,
                     rendererEvictions,
                     rendererDeferred,
                     rendererRamHits,
                     rendererPrimitives,
                     rendererFlushes,
                     settings.debugCollisionMasks ? 1 : 0,
                     (unsigned long long)runner->profStepEventsUs,
                     (unsigned long long)runner->profStepAlarmsUs,
                     (unsigned long long)runner->profStepSpatialUs,
                     (unsigned long long)runner->profStepCollisionUs,
                     (unsigned long long)runner->profStepOtherUs);
            dev_log_write(dev_sample);
            char render_phase[256];
            snprintf(render_phase, sizeof(render_phase),
                     "RENDER_PHASE frame=%u pre_us=%llu begin_us=%llu views_us=%llu composite_us=%llu gui_us=%llu overlay_us=%llu swap_us=%llu",
                     frame,
                     (unsigned long long)render_pre_us,
                     (unsigned long long)render_begin_frame_us,
                     (unsigned long long)render_views_us,
                     (unsigned long long)render_composite_us,
                     (unsigned long long)render_gui_us,
                     (unsigned long long)render_overlay_us,
                     (unsigned long long)render_swap_us);
            dev_log_write(render_phase);
        }
        /* The normal probe log is always collected by users. Emit a compact
         * phase sample every 120 frames so dialogue/cutscene captures remain
         * useful without requiring Dev Mode or writing on every frame. */
        if ((frame % 120U) == 0U) {
            uint32_t active_instances = 0;
            uint32_t dialogue_instances = 0;
            char dialogue_names[192];
            dialogue_names[0] = '\0';
            for (ptrdiff_t i = 0; i < arrlen(runner->instances); ++i) {
                Instance* inst = runner->instances[i];
                if (inst == NULL || !inst->active) continue;
                active_instances++;
                if (inst->objectIndex < 0 || (uint32_t)inst->objectIndex >= dw->objt.count) continue;
                const char* name = dw->objt.objects[inst->objectIndex].name;
                if (name == NULL || (strstr(name, "text") == NULL &&
                    strstr(name, "message") == NULL && strstr(name, "dialog") == NULL &&
                    strstr(name, "cif") == NULL && strstr(name, "crawl") == NULL)) continue;
                dialogue_instances++;
                if (strstr(dialogue_names, name) == NULL) {
                    size_t used = strlen(dialogue_names);
                    snprintf(dialogue_names + used, sizeof(dialogue_names) - used,
                             "%s%s", used > 0 ? "," : "", name);
                }
            }
            char detail[768];
            snprintf(detail, sizeof(detail),
                     "PERF_DETAIL frame=%u room=%s fps=%.2f total_us=%llu step_us=%llu audio_us=%llu render_us=%llu pre_us=%llu begin_us=%llu views_us=%llu composite_us=%llu gui_us=%llu overlay_us=%llu swap_us=%llu quads=%u flushes=%u instances=%u dialogue_instances=%u dialogue=%s gpu_bytes=%llu evict=%u defer=%u ramhit=%u",
                     frame, runner->currentRoom && runner->currentRoom->name ? runner->currentRoom->name : "<null>",
                     work_us > 0 ? 1000000.0 / (double)work_us : 0.0,
                     (unsigned long long)work_us, (unsigned long long)step_us,
                     (unsigned long long)audio_us, (unsigned long long)render_us,
                     (unsigned long long)render_pre_us,
                     (unsigned long long)render_begin_frame_us,
                     (unsigned long long)render_views_us,
                     (unsigned long long)render_composite_us,
                     (unsigned long long)render_gui_us,
                     (unsigned long long)render_overlay_us,
                     (unsigned long long)render_swap_us,
                     rendererPrimitives, rendererFlushes, active_instances,
                     dialogue_instances, dialogue_names[0] ? dialogue_names : "none",
                     (unsigned long long)rendererGpuBytes, rendererEvictions,
                     rendererDeferred, rendererRamHits);
            log_line(detail);
        }
        if (work_us > 50000 && frame_begin - last_slow_log > 1000000ULL) {
            char slow[384];
            snprintf(slow, sizeof(slow), "PERF_SLOW frame=%u total_us=%llu step_us=%llu audio_us=%llu render_us=%llu gpu_bytes=%llu events_us=%llu alarms_us=%llu collision_us=%llu other_us=%llu room=%s index=%d",
                     frame, (unsigned long long)work_us, (unsigned long long)step_us,
                     (unsigned long long)audio_us, (unsigned long long)render_us,
                     (unsigned long long)rendererGpuBytes,
                     (unsigned long long)runner->profStepEventsUs,
                     (unsigned long long)runner->profStepAlarmsUs,
                     (unsigned long long)runner->profStepCollisionUs,
                     (unsigned long long)runner->profStepOtherUs,
                     runner->currentRoom && runner->currentRoom->name ? runner->currentRoom->name : "<null>",
                     runner->currentRoomIndex);
            log_line(slow);
            last_slow_log = frame_begin;
        }
        if (frame_begin - perf_window_start >= 5000000ULL && perf_frames > 0) {
            char summary[192];
            snprintf(summary, sizeof(summary), "PERF_SUMMARY frames=%u avg_us=%llu max_us=%llu drops40=%u severe80=%u room=%s",
                     perf_frames, (unsigned long long)(perf_total_us / perf_frames),
                     (unsigned long long)perf_max_us, perf_drops, perf_severe,
                     runner->currentRoom && runner->currentRoom->name ? runner->currentRoom->name : "<null>");
            log_line(summary);
            log_memory_snapshot("perf_summary",
                                g_vitaModernGlActive ? NULL : (GLLegacyRenderer*)renderer);
            perf_window_start = frame_begin;
            perf_total_us = perf_max_us = 0;
            perf_frames = perf_drops = perf_severe = 0;
        }

        frame++;
        if (frame == 1) log_line("FRAME=first_complete");
        if (frame == 60) {
            char room_line[128];
            snprintf(room_line, sizeof(room_line), "FRAME=60_complete chapter=%d room=%s index=%d",
                     active_chapter,
                     runner->currentRoom && runner->currentRoom->name ? runner->currentRoom->name : "<null>",
                     runner->currentRoomIndex);
            log_line(room_line);
        }
        uint32_t targetFps = 0;
        switch (settings.fpsTargetMode) {
            case 0: targetFps = 30; break;
            case 1: targetFps = 40; break;
            case 2: targetFps = 0;  break; // 60 Hz is paced by VitaGL VBlank.
            case 3: targetFps = 0;  break; // Unlock / Uncapped
            default: targetFps = 30; break;
        }

        if (targetFps > 0) {
            const uint64_t target = 1000000ULL / targetFps;
            next_frame_deadline += target;
            uint64_t current_time = sceKernelGetProcessTimeWide();
            if (next_frame_deadline > current_time) {
                // sceKernelDelayThread can overshoot short waits. Sleep most of
                // the interval, then finish against the monotonic deadline for
                // stable 33.3/25/16.7 ms pacing.
                uint64_t remaining = next_frame_deadline - current_time;
                if (remaining > 1200)
                    sceKernelDelayThread((unsigned int)(remaining - 1000));
                while (sceKernelGetProcessTimeWide() < next_frame_deadline) {
                    // The bounded tail is at most ~1 ms and avoids another
                    // scheduler quantum without changing game timing.
                }
            } else {
                // Never attempt to catch up a missed deadline with a short
                // follow-up frame; that was the source of visible oscillation
                // after texture/audio hitches.
                next_frame_deadline = current_time;
            }
        } else {
            next_frame_deadline = sceKernelGetProcessTimeWide();
        }
    }

    /* Native desktop GameMaker dispatches Game End before tearing the runner
       down. Void Stranger writes settings.vs from that event; skipping it can
       lose settings or leave a corrupt save after returning to LiveArea. */
    log_line(next_chapter >= 0 ? "EXIT=chapter_switch" : "EXIT=runner_requested");
    Runner_executeEventForAll(runner, EVENT_OTHER, OTHER_GAME_END);
    log_line("EXIT=game_end_event_complete");
    Runner_free(runner);
    runner = NULL;
    dev_log_stop();
    audio->vtable->destroy(audio);
    VitaBorders_shutdown();
    VitaVideo_shutdown();
    renderer->vtable->destroy(renderer);
    OverlayFileSystem_destroy(fs);
    VM_free(vm);
    DataWin_free(dw);
    log_line("PROCESS=exit_clean");
    if (probe_log_fd >= 0) {
        sceIoClose(probe_log_fd);
        probe_log_fd = -1;
    }
    sceKernelExitProcess(0);
    return 0;
}

