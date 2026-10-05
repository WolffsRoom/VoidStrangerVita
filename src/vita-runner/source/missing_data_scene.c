#include "missing_data_scene.h"
#include "missing_data_scene_policy.h"
#include "missing_data_font_metrics.h"

#include <psp2/ctrl.h>
#include <psp2/audioout.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <vitaGL.h>
#include <openssl/sha.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stb_image.h"
#include "audio/openal/wave.h"

#define MDS_ROOT "ux0:data/voidstranger/"
#define MDS_ASSET "app0:assets/info_load/"
#define MDS_PI 3.14159265358979323846f
#define MDS_TYPEWRITER_TICK_US 16667ULL
#define MDS_DIALOG_LEFT 78.0f
#define MDS_DIALOG_TOP 381.0f
#define MDS_DIALOG_MAX_WIDTH 826

typedef struct SceneTexture {
    GLuint id;
    int w;
    int h;
} SceneTexture;

typedef struct SceneVertex {
    float u, v;
    float x, y;
} SceneVertex;

typedef struct SceneAssets {
    SceneTexture background;
    SceneTexture effect;
    SceneTexture textbox;
    SceneTexture portraits[3];
    SceneTexture fontAtlas;
    SceneTexture sera[4];
    /* Only question/menu overlays stay resident; dialogue is rendered glyph-by-glyph. */
    SceneTexture overlay;
    SceneTexture qr;
    SceneTexture url;
} SceneAssets;

typedef struct VoicePlayer {
    int port;
    SceUID thread;
    int16_t* pcm;
    uint32_t frames;
    uint32_t sampleRate;
    uint16_t channels;
    volatile uint32_t requestSerial;
    volatile uint32_t playedSerial;
    volatile bool running;
    bool ready;
} VoicePlayer;

static VoicePlayer* g_mds_voice = NULL;
static const char* mds_portrait_mood_name(MissingDataPortraitMood mood);

typedef struct DialogInfo {
    const char* text;
    MissingDataPortraitMood mood;
} DialogInfo;

static bool mds_file_stat(const char* path, SceIoStat* st) {
    if (path == NULL || st == NULL) return false;
    memset(st, 0, sizeof(*st));
    return sceIoGetstat(path, st) >= 0 && SCE_S_ISREG(st->st_mode);
}

static bool mds_sha256_file_hex(const char* path, char outHex[65]) {
    if (path == NULL || outHex == NULL) return false;
    SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
    if (fd < 0) return false;
    SHA256_CTX ctx;
    if (SHA256_Init(&ctx) != 1) {
        sceIoClose(fd);
        return false;
    }
    unsigned char buffer[64 * 1024];
    for (;;) {
        int got = sceIoRead(fd, buffer, sizeof(buffer));
        if (got < 0) { sceIoClose(fd); return false; }
        if (got == 0) break;
        if (SHA256_Update(&ctx, buffer, (size_t)got) != 1) {
            sceIoClose(fd);
            return false;
        }
    }
    sceIoClose(fd);
    unsigned char digest[SHA256_DIGEST_LENGTH];
    if (SHA256_Final(digest, &ctx) != 1) return false;
    static const char hex[] = "0123456789ABCDEF";
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        outHex[i * 2] = hex[(digest[i] >> 4) & 0x0F];
        outHex[i * 2 + 1] = hex[digest[i] & 0x0F];
    }
    outHex[64] = '\0';
    return true;
}

static bool mds_is_current_data_win(const char* path) {
    SceIoStat st;
    if (!mds_file_stat(path, &st)) return false;
    if ((uint64_t)st.st_size != MISSING_DATA_CURRENT_DATA_WIN_SIZE) return false;
    char digest[65];
    return mds_sha256_file_hex(path, digest) &&
           missing_data_sha256_hex_is_current(digest);
}

static bool mds_is_game_data_win_candidate(const char* path) {
    SceIoStat st;
    if (!mds_file_stat(path, &st) || st.st_size < 1024) return false;
    SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
    if (fd < 0) return false;
    unsigned char header[4] = {0, 0, 0, 0};
    int got = sceIoRead(fd, header, sizeof(header));
    sceIoClose(fd);
    return got == 4 && memcmp(header, "FORM", 4) == 0;
}
uint32_t MissingDataScene_requiredMask(void) {
    SceIoStat dataWin, audio1, audio2, csv;
    bool hasData = mds_file_stat(MDS_ROOT "data.win", &dataWin);
    bool dataHashOk = hasData && mds_is_current_data_win(MDS_ROOT "data.win");
    bool hasAudio1 = mds_file_stat(MDS_ROOT "audiogroup1.dat", &audio1);
    bool hasAudio2 = mds_file_stat(MDS_ROOT "audiogroup2.dat", &audio2);
    bool hasCsv = mds_file_stat(MDS_ROOT "voidstranger_data.csv", &csv);
    return missing_data_required_mask(hasData, dataHashOk, hasAudio1, hasAudio2, hasCsv);
}
static const char* kMdsRequiredNames[MISSING_DATA_REQUIRED_COUNT] = {
    "data.win", "audiogroup1.dat", "audiogroup2.dat", "voidstranger_data.csv"
};

static const uint32_t kMdsRequiredBits[MISSING_DATA_REQUIRED_COUNT] = {
    MISSING_DATA_REQ_DATA_WIN, MISSING_DATA_REQ_AUDIOGROUP1,
    MISSING_DATA_REQ_AUDIOGROUP2, MISSING_DATA_REQ_CSV
};

static void mds_scan_directory_recursive(const char* directory, int depth,
                                         MissingDataInspection* inspection) {
    if (directory == NULL || inspection == NULL || depth > 8) return;
    SceUID dir = sceIoDopen(directory);
    if (dir < 0) return;
    SceIoDirent entry;
    memset(&entry, 0, sizeof(entry));
    while (sceIoDread(dir, &entry) > 0) {
        const char* name = entry.d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            memset(&entry, 0, sizeof(entry));
            continue;
        }
        char child[MISSING_DATA_PATH_MAX];
        size_t len = strlen(directory);
        snprintf(child, sizeof(child), "%s%s%s", directory,
                 (len > 0 && directory[len - 1] == '/') ? "" : "/", name);
        if (SCE_S_ISDIR(entry.d_stat.st_mode)) {
            mds_scan_directory_recursive(child, depth + 1, inspection);
        } else if (SCE_S_ISREG(entry.d_stat.st_mode)) {
            for (int i = 0; i < MISSING_DATA_REQUIRED_COUNT; ++i) {
                uint32_t bit = kMdsRequiredBits[i];
                if ((inspection->rootMissingMask & bit) == 0u) continue;
                if (bit == MISSING_DATA_REQ_DATA_WIN) {
                    /* An invalid data.win already at the correct root is a version/hash
                       problem, not an organization problem. Only alternate .win paths
                       are candidates for automatic normalization. */
                    if (strcmp(child, MDS_ROOT "data.win") == 0) continue;
                    if (!missing_data_candidate_name_is_win(name) ||
                        !mds_is_game_data_win_candidate(child)) continue;
                } else if (strcmp(name, kMdsRequiredNames[i]) != 0) {
                    continue;
                }
                if ((inspection->misplacedMask & bit) == 0u) {
                    inspection->misplacedMask |= bit;
                    snprintf(inspection->misplacedPaths[i], MISSING_DATA_PATH_MAX, "%s", child);
                } else if (strcmp(inspection->misplacedPaths[i], child) != 0) {
                    inspection->duplicateMask |= bit;
                }
            }
        }
        memset(&entry, 0, sizeof(entry));
    }
    sceIoDclose(dir);
}

MissingDataLayoutState MissingDataScene_inspect(MissingDataInspection* inspection) {
    if (inspection == NULL) return MISSING_DATA_LAYOUT_MISSING;
    memset(inspection, 0, sizeof(*inspection));
    inspection->rootMissingMask = MissingDataScene_requiredMask();
    if (inspection->rootMissingMask == 0u) return MISSING_DATA_LAYOUT_READY;
    mds_scan_directory_recursive(MDS_ROOT, 0, inspection);
    return missing_data_layout_state(inspection->rootMissingMask,
                                     inspection->misplacedMask,
                                     inspection->duplicateMask);
}

bool MissingDataScene_fixOrganization(const MissingDataInspection* inspection) {
    if (inspection == NULL || inspection->duplicateMask != 0u) return false;
    for (int i = 0; i < MISSING_DATA_REQUIRED_COUNT; ++i) {
        uint32_t bit = kMdsRequiredBits[i];
        if ((inspection->rootMissingMask & bit) == 0u) continue;
        if ((inspection->misplacedMask & bit) == 0u || inspection->misplacedPaths[i][0] == '\0')
            return false;
        char destination[MISSING_DATA_PATH_MAX];
        snprintf(destination, sizeof(destination), MDS_ROOT "%s", kMdsRequiredNames[i]);
        SceIoStat existing;
        if (mds_file_stat(destination, &existing)) continue;
        if (sceIoRename(inspection->misplacedPaths[i], destination) < 0) return false;
    }
    return MissingDataScene_requiredMask() == 0u;
}

static SceneTexture mds_load_texture(const char* path) {
    SceneTexture result = {0, 0, 0};
    SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
    if (fd < 0) return result;
    SceOff size = sceIoLseek(fd, 0, SCE_SEEK_END);
    sceIoLseek(fd, 0, SCE_SEEK_SET);
    if (size <= 0) { sceIoClose(fd); return result; }
    unsigned char* encoded = (unsigned char*)malloc((size_t)size);
    if (encoded == NULL) { sceIoClose(fd); return result; }
    int read = sceIoRead(fd, encoded, (unsigned int)size);
    sceIoClose(fd);
    if (read != size) { free(encoded); return result; }

    int channels = 0;
    unsigned char* pixels = stbi_load_from_memory(encoded, (int)size,
                                                   &result.w, &result.h, &channels, 4);
    free(encoded);
    if (pixels == NULL) { result.w = result.h = 0; return result; }

    glGenTextures(1, &result.id);
    glBindTexture(GL_TEXTURE_2D, result.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, result.w, result.h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    stbi_image_free(pixels);
    return result;
}

static void mds_delete_texture(SceneTexture* t) {
    if (t != NULL && t->id != 0) glDeleteTextures(1, &t->id);
    if (t != NULL) { t->id = 0; t->w = t->h = 0; }
}

static void mds_draw_region(SceneTexture t,
                            float srcX, float srcY, float srcW, float srcH,
                            float left, float top, float right, float bottom,
                            float alpha) {
    if (t.id == 0 || t.w <= 0 || t.h <= 0 || alpha <= 0.0f || srcW <= 0.0f || srcH <= 0.0f) return;
    float x0 = left / 480.0f - 1.0f;
    float x1 = right / 480.0f - 1.0f;
    float y0 = 1.0f - top / 272.0f;
    float y1 = 1.0f - bottom / 272.0f;
    float u0 = srcX / (float)t.w;
    float u1 = (srcX + srcW) / (float)t.w;
    float v0 = srcY / (float)t.h;
    float v1 = (srcY + srcH) / (float)t.h;
    const SceneVertex vertices[4] = {
        {u0, v0, x0, y0}, {u1, v0, x1, y0},
        {u1, v1, x1, y1}, {u0, v1, x0, y1}
    };
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 1.0f, 1.0f, alpha);
    glBindTexture(GL_TEXTURE_2D, t.id);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, sizeof(SceneVertex), &vertices[0].u);
    glVertexPointer(2, GL_FLOAT, sizeof(SceneVertex), &vertices[0].x);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

static void mds_draw_full(SceneTexture t, float alpha) {
    mds_draw_region(t, 0, 0, (float)t.w, (float)t.h, 0, 0, 960, 544, alpha);
}

static void mds_draw_scaled(SceneTexture t, float left, float top, float width, float height, float alpha) {
    mds_draw_region(t, 0, 0, (float)t.w, (float)t.h, left, top, left + width, top + height, alpha);
}


static const MissingDataGlyphMetric* mds_glyph_metric(unsigned char ch) {
    if (ch < MDS_FONT_FIRST_CHAR || ch > MDS_FONT_LAST_CHAR) ch = '?';
    return &kMdsGlyphMetrics[ch - MDS_FONT_FIRST_CHAR];
}

static int mds_text_width(const char* text, int length) {
    int width = 0;
    for (int i = 0; i < length; ++i) {
        unsigned char ch = (unsigned char)text[i];
        if (ch == '\n') break;
        width += mds_glyph_metric(ch)->advance;
    }
    return width;
}

static int mds_wrap_dialog_text(const char* text, char* output, int capacity) {
    if (text == NULL || output == NULL || capacity <= 1) return 0;
    int out = 0;
    int lineWidth = 0;
    int spaceWidth = mds_glyph_metric(' ')->advance;
    const char* cursor = text;
    while (*cursor != '\0' && out < capacity - 1) {
        while (*cursor == ' ') cursor++;
        if (*cursor == '\0') break;
        const char* word = cursor;
        while (*cursor != '\0' && *cursor != ' ') cursor++;
        int wordLen = (int)(cursor - word);
        int wordWidth = mds_text_width(word, wordLen);
        if (lineWidth > 0 && lineWidth + spaceWidth + wordWidth > MDS_DIALOG_MAX_WIDTH) {
            output[out++] = '\n';
            lineWidth = 0;
        } else if (lineWidth > 0 && out < capacity - 1) {
            output[out++] = ' ';
            lineWidth += spaceWidth;
        }
        for (int i = 0; i < wordLen && out < capacity - 1; ++i)
            output[out++] = word[i];
        lineWidth += wordWidth;
    }
    output[out] = '\0';
    return out;
}

static void mds_draw_dialog_chars(SceneAssets* assets, const char* wrappedText, int visibleChars) {
    if (assets == NULL || assets->fontAtlas.id == 0 || wrappedText == NULL || visibleChars <= 0) return;
    float penX = MDS_DIALOG_LEFT;
    float lineY = MDS_DIALOG_TOP;
    for (int i = 0; wrappedText[i] != '\0' && i < visibleChars; ++i) {
        unsigned char ch = (unsigned char)wrappedText[i];
        if (ch == '\n') {
            penX = MDS_DIALOG_LEFT;
            lineY += MDS_FONT_LINE_HEIGHT;
            continue;
        }
        const MissingDataGlyphMetric* g = mds_glyph_metric(ch);
        if (g->w > 0 && g->h > 0 && ch != ' ') {
            float left = penX + g->bearingX;
            float top = lineY + g->bearingY;
            mds_draw_region(assets->fontAtlas,
                            (float)g->x, (float)g->y, (float)g->w, (float)g->h,
                            left, top, left + g->w, top + g->h, 1.0f);
        }
        penX += g->advance;
    }
}

static bool mds_load_assets(SceneAssets* a) {
    memset(a, 0, sizeof(*a));
#define LOAD(field, path) do { \
    printf("MDS_LOAD begin %s\n", path); \
    a->field = mds_load_texture(MDS_ASSET path); \
    if (a->field.id == 0) return false; \
    printf("MDS_LOAD ok %s %dx%d id=%u\n", path, a->field.w, a->field.h, (unsigned)a->field.id); \
} while (0)
    LOAD(background, "background/background.png");
    LOAD(effect, "background/effect.png");
    LOAD(textbox, "generated/textbox_crop.png");
    LOAD(portraits[MISSING_DATA_PORTRAIT_NEUTRAL], "portrait/spr_sera_port_neutral_000.png");
    LOAD(portraits[MISSING_DATA_PORTRAIT_HAPPY], "portrait/spr_sera_port_happy.png");
    LOAD(portraits[MISSING_DATA_PORTRAIT_SIGH], "portrait/spr_sera_port_sigh.png");
    LOAD(fontAtlas, "generated/alkhemikal_dialog_atlas.png");
    LOAD(sera[0], "character/spr_sera_000.png");
    LOAD(sera[1], "character/spr_sera_001.png");
    LOAD(sera[2], "character/spr_sera_002.png");
    LOAD(sera[3], "character/spr_sera_003.png");
#undef LOAD
    return true;
}

static void mds_unload_assets(SceneAssets* a) {
    SceneTexture* list[] = {
        &a->background, &a->effect, &a->textbox,
        &a->portraits[0], &a->portraits[1], &a->portraits[2], &a->fontAtlas,
        &a->sera[0], &a->sera[1], &a->sera[2], &a->sera[3],
        &a->overlay, &a->qr, &a->url
    };
    for (unsigned int i = 0; i < sizeof(list) / sizeof(list[0]); ++i)
        mds_delete_texture(list[i]);
}

static const char* mds_overlay_path(MissingDataStage stage, int helpPage, int selection) {
    (void)helpPage;
    if (stage == MISSING_DATA_STAGE_QUESTION)
        return selection == 0 ? "generated/question_yes.png" : "generated/question_no.png";
    return NULL;
}

static bool mds_reload_overlay(SceneAssets* a, MissingDataStage stage, int helpPage, int selection) {
    const char* path = mds_overlay_path(stage, helpPage, selection);
    mds_delete_texture(&a->overlay);
    if (path == NULL) return true;
    printf("MDS_OVERLAY begin %s\n", path);
    char fullPath[192];
    snprintf(fullPath, sizeof(fullPath), "%s%s", MDS_ASSET, path);
    a->overlay = mds_load_texture(fullPath);
    printf("MDS_OVERLAY %s %s %dx%d id=%u\n", a->overlay.id ? "ok" : "fail", path,
           a->overlay.w, a->overlay.h, (unsigned)a->overlay.id);
    return a->overlay.id != 0;
}

static bool mds_load_help_extras(SceneAssets* a) {
    if (a->qr.id == 0) a->qr = mds_load_texture(MDS_ASSET "generated/github_qr.png");
    if (a->url.id == 0) a->url = mds_load_texture(MDS_ASSET "generated/github_url.png");
    return a->qr.id != 0 && a->url.id != 0;
}

static void mds_voice_shutdown(VoicePlayer* v) {
    if (v == NULL) return;
    v->running = false;
    if (v->thread >= 0) {
        sceKernelWaitThreadEnd(v->thread, NULL, NULL);
        sceKernelDeleteThread(v->thread);
    }
    if (v->port >= 0) sceAudioOutReleasePort(v->port);
    free(v->pcm);
    if (g_mds_voice == v) g_mds_voice = NULL;
    memset(v, 0, sizeof(*v));
    v->port = -1;
    v->thread = -1;
}

static bool mds_voice_init(VoicePlayer* v) {
    /* Do not open a new SceAudioOut port from the pre-data assistant. Vita3K
       currently crashes inside sceAudioOutOpenPort at this point in boot.
       Keep the scene fully functional and visual-only when no safe shared
       voice backend exists; snd_lev.wav remains packaged for the follow-up
       audio backend work. */
    memset(v, 0, sizeof(*v));
    v->port = -1;
    v->thread = -1;
    v->ready = false;
    return false;
}
static void mds_voice_blip(VoicePlayer* v) {
    if (v == NULL || !v->ready) return;
    v->requestSerial++;
}

static DialogInfo mds_dialog_for_stage(MissingDataStage stage, int helpPage,
                                       MissingDataPortraitMood mood) {
    DialogInfo d = {NULL, mood};
    switch (stage) {
        case MISSING_DATA_STAGE_HELLO:
            d.text = "Hello.";
            break;
        case MISSING_DATA_STAGE_NEED_FILES:
            d.text = "You need the game files, you know?";
            break;
        case MISSING_DATA_STAGE_YES_PLACE:
            d.text = "Then make sure they're in the right place.";
            break;
        case MISSING_DATA_STAGE_YES_CHECK:
            d.text = "I'll check them for you.";
            break;
        case MISSING_DATA_STAGE_MISSING:
            d.text = "Hm. Something is missing.";
            break;
        case MISSING_DATA_STAGE_INSTALL_HELP:
            if (helpPage <= 0)
                d.text = "You should prepare the game files again.";
            else if (helpPage == 1)
                d.text = "You'll need a legitimate copy of Void Stranger.";
            else if (helpPage == 2)
                d.text = "Use the Vita Patcher to prepare the required files.";
            else
                d.text = "Remember to place the generated files in \"ux0:data/voidstranger\". Have fun!";
            break;
        default:
            break;
    }
    return d;
}

static bool mds_is_dialog_stage(MissingDataStage stage) {
    return stage == MISSING_DATA_STAGE_HELLO || stage == MISSING_DATA_STAGE_NEED_FILES ||
           stage == MISSING_DATA_STAGE_YES_PLACE || stage == MISSING_DATA_STAGE_YES_CHECK ||
           stage == MISSING_DATA_STAGE_MISSING || stage == MISSING_DATA_STAGE_INSTALL_HELP;
}

static int mds_prepare_dialog(DialogInfo info, char wrapped[512]) {
    if (info.text == NULL) { wrapped[0] = '\0'; return 0; }
    return mds_wrap_dialog_text(info.text, wrapped, 512);
}

static void mds_draw_scene(SceneAssets* a, MissingDataStage stage, int helpPage,
                           int selection, uint64_t sceneElapsedUs,
                           int visibleChars, MissingDataPortraitMood mood) {
    (void)selection;
    float sceneSeconds = (float)sceneElapsedUs / 1000000.0f;
    float effectAlpha = 0.10f + 0.05f * sinf((sceneSeconds / 3.5f) * 2.0f * MDS_PI);
    int seraFrame = ((int)(sceneSeconds * 12.0f)) & 3;

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    mds_draw_full(a->background, 1.0f);
    mds_draw_full(a->effect, effectAlpha);

    float seraCenterY = 210.0f;
    if (sceneElapsedUs < 3200000ULL) {
        float p = (float)sceneElapsedUs / 3200000.0f;
        seraCenterY = -32.0f + (210.0f + 32.0f) * p;
    }
    if (stage == MISSING_DATA_STAGE_QUESTION) seraCenterY = 228.0f;
    mds_draw_scaled(a->sera[seraFrame], 480.0f - 32.0f, seraCenterY - 32.0f, 64, 64, 1.0f);

    if (sceneElapsedUs < 4000000ULL && stage == MISSING_DATA_STAGE_HELLO) return;

    if (stage == MISSING_DATA_STAGE_QUESTION) {
        mds_draw_scaled(a->overlay, 114, 87, 728, 342, 1.0f);
        return;
    }

    if (mds_is_dialog_stage(stage)) {
        DialogInfo d = mds_dialog_for_stage(stage, helpPage, mood);
        char wrapped[512];
        int textLength = mds_prepare_dialog(d, wrapped);
        if (visibleChars > textLength) visibleChars = textLength;
        mds_draw_scaled(a->textbox, 32, 344, 896, 184, 1.0f);
        int portraitIndex = (int)d.mood;
        if (portraitIndex < 0 || portraitIndex > 2) portraitIndex = MISSING_DATA_PORTRAIT_NEUTRAL;
        /* 288x288 is 50% larger than the previous 192x192 presentation. */
        mds_draw_scaled(a->portraits[portraitIndex], 336, 40, 288, 288, 1.0f);
        mds_draw_dialog_chars(a, wrapped, visibleChars);
        if (stage == MISSING_DATA_STAGE_INSTALL_HELP && helpPage == 2 &&
            visibleChars >= textLength) {
            mds_draw_scaled(a->qr, 772, 140, 156, 156, 1.0f);
            mds_draw_scaled(a->url, 489, 318, 449, 23, 1.0f);
        }
    }
}

int MissingDataScene_run(uint32_t initialMask) {
    if (initialMask == 0u) return 1;

    SceneAssets assets;
    if (!mds_load_assets(&assets)) {
        mds_unload_assets(&assets);
        return -1;
    }
    if (!mds_reload_overlay(&assets, MISSING_DATA_STAGE_HELLO, 1, 0)) {
        mds_unload_assets(&assets);
        return -1;
    }

    VoicePlayer voice;
    (void)mds_voice_init(&voice); /* Scene remains visual-only if AudioOut is unavailable. */

    MissingDataStage stage = MISSING_DATA_STAGE_HELLO;
    int selection = 0;
    int helpPage = 1;
    MissingDataPortraitMood mood = MISSING_DATA_PORTRAIT_NEUTRAL;
    MissingDataTypewriter typewriter;
    missing_data_typewriter_reset(&typewriter);
    uint64_t sceneStart = sceKernelGetProcessTimeWide();
    uint64_t stageStart = sceneStart + 4000000ULL; /* 3.2 s entrance + original-like 0.8 s hold. */
    uint64_t typewriterTick = stageStart;
    uint32_t previousButtons = 0;
    SceCtrlData pad;
    memset(&pad, 0, sizeof(pad));
    sceCtrlPeekBufferPositive(0, &pad, 1);
    previousButtons = pad.buttons;

    for (;;) {
        uint64_t now = sceKernelGetProcessTimeWide();
        uint64_t sceneElapsed = now - sceneStart;
        bool stageActive = now >= stageStart;

        memset(&pad, 0, sizeof(pad));
        sceCtrlPeekBufferPositive(0, &pad, 1);
        uint32_t pressed = pad.buttons & ~previousButtons;
        previousButtons = pad.buttons;

        DialogInfo dialog = mds_dialog_for_stage(stage, helpPage, mood);
        char wrapped[512];
        int textLength = mds_prepare_dialog(dialog, wrapped);
        if (stageActive && mds_is_dialog_stage(stage) && textLength > 0) {
            while (typewriter.visibleChars < textLength &&
                   now - typewriterTick >= MDS_TYPEWRITER_TICK_US) {
                typewriterTick += MDS_TYPEWRITER_TICK_US;
                if (missing_data_typewriter_step(&typewriter, wrapped, textLength))
                    mds_voice_blip(&voice);
            }
        }

        if ((pressed & SCE_CTRL_CIRCLE) != 0) {
            mds_voice_shutdown(&voice);
            mds_unload_assets(&assets);
            return 0;
        }

        if (stageActive && stage == MISSING_DATA_STAGE_QUESTION) {
            if ((pressed & (SCE_CTRL_UP | SCE_CTRL_DOWN)) != 0) {
                selection ^= 1;
                if (!mds_reload_overlay(&assets, stage, helpPage, selection)) {
                    mds_voice_shutdown(&voice); mds_unload_assets(&assets); return -1;
                }
            }
            if ((pressed & SCE_CTRL_CROSS) != 0) {
                MissingDataAction action = selection == 0 ? MISSING_DATA_ACTION_YES : MISSING_DATA_ACTION_NO;
                stage = missing_data_next_stage(stage, action);
                mood = missing_data_portrait_mood(false, action == MISSING_DATA_ACTION_NO);
                printf("MDS_PORTRAIT mood=%s size=288x288 main_answer=%s\n",
                       mds_portrait_mood_name(mood),
                       action == MISSING_DATA_ACTION_NO ? "no" : "yes");
                if (stage == MISSING_DATA_STAGE_INSTALL_HELP) helpPage = 1;
                if (!mds_reload_overlay(&assets, stage, helpPage, selection)) {
                    mds_voice_shutdown(&voice); mds_unload_assets(&assets); return -1;
                }
                stageStart = now;
                typewriterTick = now;
                missing_data_typewriter_reset(&typewriter);
            }
        } else if (stageActive && mds_is_dialog_stage(stage) && (pressed & SCE_CTRL_CROSS) != 0) {
            if (typewriter.visibleChars < textLength) {
                missing_data_typewriter_force(&typewriter, textLength);
            } else if (stage == MISSING_DATA_STAGE_INSTALL_HELP) {
                int nextHelpPage = missing_data_help_page_after_confirm(helpPage);
                if (nextHelpPage >= 0) {
                    helpPage = nextHelpPage;
                    mood = MISSING_DATA_PORTRAIT_NEUTRAL;
                    if (helpPage == 2 && !mds_load_help_extras(&assets)) {
                        mds_voice_shutdown(&voice); mds_unload_assets(&assets); return -1;
                    }
                    if (helpPage == 3) {
                        mds_delete_texture(&assets.qr);
                        mds_delete_texture(&assets.url);
                    }
                    stageStart = now;
                    typewriterTick = now;
                    missing_data_typewriter_reset(&typewriter);
                } else {
                    const uint64_t fadeDurationUs = 1000000ULL;
                    const uint64_t fadeStartedUs = sceKernelGetProcessTimeWide();
                    for (;;) {
                        uint64_t fadeNow = sceKernelGetProcessTimeWide();
                        uint64_t fadeElapsed = fadeNow - fadeStartedUs;
                        DialogInfo finalDialog = mds_dialog_for_stage(stage, helpPage, mood);
                        char finalWrapped[512];
                        int finalLength = mds_prepare_dialog(finalDialog, finalWrapped);
                        mds_draw_scene(&assets, stage, helpPage, selection,
                                       fadeNow - sceneStart, finalLength, mood);
                        mds_draw_full(assets.background,
                                      missing_data_fade_alpha(fadeElapsed, fadeDurationUs));
                        vglSwapBuffers(GL_TRUE);
                        if (fadeElapsed >= fadeDurationUs) break;
                        sceKernelDelayThread(16667);
                    }
                    mds_voice_shutdown(&voice);
                    mds_unload_assets(&assets);
                    return 0;
                }
            } else {
                MissingDataStage next = missing_data_next_stage(stage, MISSING_DATA_ACTION_CONFIRM);
                if (next == MISSING_DATA_STAGE_RECHECK) {
                    uint32_t mask = MissingDataScene_requiredMask();
                    next = missing_data_next_stage(next, mask == 0u ?
                                                   MISSING_DATA_ACTION_RECHECK_OK :
                                                   MISSING_DATA_ACTION_RECHECK_FAILED);
                    if (next == MISSING_DATA_STAGE_DONE) {
                        mds_voice_shutdown(&voice);
                        mds_unload_assets(&assets);
                        return 1;
                    }
                    if (next == MISSING_DATA_STAGE_MISSING) helpPage = 0;
                }
                stage = next;
                mood = MISSING_DATA_PORTRAIT_NEUTRAL;
                if (!mds_reload_overlay(&assets, stage, helpPage, selection)) {
                    mds_voice_shutdown(&voice); mds_unload_assets(&assets); return -1;
                }
                stageStart = now;
                typewriterTick = now;
                missing_data_typewriter_reset(&typewriter);
            }
        }

        mds_draw_scene(&assets, stage, helpPage, selection, sceneElapsed,
                       typewriter.visibleChars, mood);
        vglSwapBuffers(GL_TRUE);
        sceKernelDelayThread(1000);
    }
}


static bool mds_load_named_overlay(SceneAssets* assets, const char* relativePath) {
    if (assets == NULL || relativePath == NULL) return false;
    mds_delete_texture(&assets->overlay);
    char path[192];
    snprintf(path, sizeof(path), "%s%s", MDS_ASSET, relativePath);
    assets->overlay = mds_load_texture(path);
    return assets->overlay.id != 0;
}

typedef enum MdsOrganizationMessage {
    MDS_ORG_MESSAGE_NONE = 0,
    MDS_ORG_MESSAGE_DECLINE,
    MDS_ORG_MESSAGE_MANUAL,
    MDS_ORG_MESSAGE_SUCCESS
} MdsOrganizationMessage;

static const char* mds_organization_text(MdsOrganizationMessage message) {
    switch (message) {
        case MDS_ORG_MESSAGE_DECLINE:
            return "Okay. You can review them yourself. See you later.";
        case MDS_ORG_MESSAGE_MANUAL:
            return "I found more than one copy of a required file. Please review them yourself. See you later.";
        case MDS_ORG_MESSAGE_SUCCESS:
            return "There. Everything is where it should be. Have fun!";
        default:
            return NULL;
    }
}

static const char* mds_portrait_mood_name(MissingDataPortraitMood mood) {
    switch (mood) {
        case MISSING_DATA_PORTRAIT_HAPPY: return "happy";
        case MISSING_DATA_PORTRAIT_SIGH: return "sigh";
        default: return "neutral";
    }
}

static MissingDataPortraitMood mds_organization_mood(MdsOrganizationMessage message) {
    if (message == MDS_ORG_MESSAGE_SUCCESS)
        return MISSING_DATA_PORTRAIT_HAPPY;
    if (message == MDS_ORG_MESSAGE_DECLINE || message == MDS_ORG_MESSAGE_MANUAL)
        return MISSING_DATA_PORTRAIT_SIGH;
    return MISSING_DATA_PORTRAIT_NEUTRAL;
}

static int mds_prepare_organization_text(MdsOrganizationMessage message, char wrapped[512]) {
    const char* text = mds_organization_text(message);
    if (text == NULL) { wrapped[0] = '\0'; return 0; }
    return mds_wrap_dialog_text(text, wrapped, 512);
}

static void mds_draw_organization_scene(SceneAssets* assets, bool question,
                                        MdsOrganizationMessage message,
                                        uint64_t sceneElapsedUs, int visibleChars) {
    float sceneSeconds = (float)sceneElapsedUs / 1000000.0f;
    float effectAlpha = 0.10f + 0.05f * sinf((sceneSeconds / 3.5f) * 2.0f * MDS_PI);
    int seraFrame = ((int)(sceneSeconds * 12.0f)) & 3;
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    mds_draw_full(assets->background, 1.0f);
    mds_draw_full(assets->effect, effectAlpha);

    float seraCenterY = 210.0f;
    if (sceneElapsedUs < 3200000ULL) {
        float p = (float)sceneElapsedUs / 3200000.0f;
        seraCenterY = -32.0f + 242.0f * p;
    }
    if (question) seraCenterY = 228.0f;
    mds_draw_scaled(assets->sera[seraFrame], 448.0f, seraCenterY - 32.0f, 64, 64, 1.0f);
    if (sceneElapsedUs < 3200000ULL) return;

    if (question) {
        mds_draw_scaled(assets->overlay, 114, 87, 728, 342, 1.0f);
        return;
    }

    char wrapped[512];
    int length = mds_prepare_organization_text(message, wrapped);
    if (visibleChars > length) visibleChars = length;
    MissingDataPortraitMood mood = mds_organization_mood(message);
    mds_draw_scaled(assets->textbox, 32, 344, 896, 184, 1.0f);
    mds_draw_scaled(assets->portraits[(int)mood], 336, 40, 288, 288, 1.0f);
    mds_draw_dialog_chars(assets, wrapped, visibleChars);
}

static void mds_organization_fade(SceneAssets* assets, MdsOrganizationMessage message,
                                  uint64_t sceneStart) {
    const uint64_t durationUs = 1000000ULL;
    char wrapped[512];
    int length = mds_prepare_organization_text(message, wrapped);
    uint64_t fadeStart = sceKernelGetProcessTimeWide();
    for (;;) {
        uint64_t now = sceKernelGetProcessTimeWide();
        uint64_t elapsed = now - fadeStart;
        mds_draw_organization_scene(assets, false, message, now - sceneStart, length);
        mds_draw_full(assets->background, missing_data_fade_alpha(elapsed, durationUs));
        vglSwapBuffers(GL_TRUE);
        if (elapsed >= durationUs) break;
        sceKernelDelayThread(16667);
    }
}

int MissingDataScene_runOrganization(const MissingDataInspection* inspection) {
    if (inspection == NULL) return -1;
    MissingDataLayoutState layout = missing_data_layout_state(inspection->rootMissingMask,
                                                              inspection->misplacedMask,
                                                              inspection->duplicateMask);
    if (layout == MISSING_DATA_LAYOUT_READY) return 1;
    if (layout == MISSING_DATA_LAYOUT_MISSING) return -1;

    SceneAssets assets;
    if (!mds_load_assets(&assets)) {
        mds_unload_assets(&assets);
        return -1;
    }
    VoicePlayer voice;
    (void)mds_voice_init(&voice);

    bool question = layout == MISSING_DATA_LAYOUT_ORGANIZABLE;
    MdsOrganizationMessage message = layout == MISSING_DATA_LAYOUT_AMBIGUOUS ?
                                     MDS_ORG_MESSAGE_MANUAL : MDS_ORG_MESSAGE_NONE;
    int selection = 0;
    if (question && !mds_load_named_overlay(&assets, "generated/organize_question_yes.png")) {
        mds_voice_shutdown(&voice);
        mds_unload_assets(&assets);
        return -1;
    }

    uint64_t sceneStart = sceKernelGetProcessTimeWide();
    uint64_t textStart = sceneStart + 3200000ULL;
    uint64_t typewriterTick = textStart;
    MissingDataTypewriter typewriter;
    missing_data_typewriter_reset(&typewriter);
    SceCtrlData pad;
    memset(&pad, 0, sizeof(pad));
    sceCtrlPeekBufferPositive(0, &pad, 1);
    uint32_t previousButtons = pad.buttons;

    for (;;) {
        uint64_t now = sceKernelGetProcessTimeWide();
        uint64_t sceneElapsed = now - sceneStart;
        memset(&pad, 0, sizeof(pad));
        sceCtrlPeekBufferPositive(0, &pad, 1);
        uint32_t pressed = pad.buttons & ~previousButtons;
        previousButtons = pad.buttons;

        char wrapped[512];
        int textLength = question ? 0 : mds_prepare_organization_text(message, wrapped);
        if (!question && now >= textStart && textLength > 0) {
            while (typewriter.visibleChars < textLength &&
                   now - typewriterTick >= MDS_TYPEWRITER_TICK_US) {
                typewriterTick += MDS_TYPEWRITER_TICK_US;
                if (missing_data_typewriter_step(&typewriter, wrapped, textLength))
                    mds_voice_blip(&voice);
            }
            if ((pressed & SCE_CTRL_CROSS) != 0) {
                if (typewriter.visibleChars < textLength) {
                    missing_data_typewriter_force(&typewriter, textLength);
                } else {
                    mds_organization_fade(&assets, message, sceneStart);
                    bool success = message == MDS_ORG_MESSAGE_SUCCESS;
                    mds_voice_shutdown(&voice);
                    mds_unload_assets(&assets);
                    return success ? 1 : 0;
                }
            }
        } else if (question && sceneElapsed >= 3200000ULL) {
            if ((pressed & (SCE_CTRL_UP | SCE_CTRL_DOWN)) != 0) {
                selection ^= 1;
                if (!mds_load_named_overlay(&assets, selection == 0 ?
                        "generated/organize_question_yes.png" : "generated/organize_question_no.png")) {
                    mds_voice_shutdown(&voice);
                    mds_unload_assets(&assets);
                    return -1;
                }
            }
            if ((pressed & SCE_CTRL_CROSS) != 0) {
                bool fixed = selection == 0 && MissingDataScene_fixOrganization(inspection);
                question = false;
                message = fixed ? MDS_ORG_MESSAGE_SUCCESS :
                          (selection == 0 ? MDS_ORG_MESSAGE_MANUAL : MDS_ORG_MESSAGE_DECLINE);
                printf("MDS_PORTRAIT mood=%s size=288x288 organization_message=%d\n",
                       mds_portrait_mood_name(mds_organization_mood(message)), (int)message);
                mds_delete_texture(&assets.overlay);
                textStart = now;
                typewriterTick = now;
                missing_data_typewriter_reset(&typewriter);
            }
        }

        if ((pressed & SCE_CTRL_CIRCLE) != 0) {
            mds_voice_shutdown(&voice);
            mds_unload_assets(&assets);
            return 0;
        }

        mds_draw_organization_scene(&assets, question, message,
                                    sceneElapsed, typewriter.visibleChars);
        vglSwapBuffers(GL_TRUE);
        sceKernelDelayThread(1000);
    }
}
