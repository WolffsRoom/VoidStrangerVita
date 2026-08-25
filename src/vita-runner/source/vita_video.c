// vita_video.c - SceAvPlayer-based MP4 video playback for Deltarune Vita
// Ported from vitaGL samples/video_playback/main.c
#include "vita_video.h"
#include <vitasdk.h>
#include <vitaGL.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define VIDEO_BUFFERS 5
#define VIDEO_MEM_ALIGNMENT 0x40000
#define ALIGN_MEM(x, align) (((x) + ((align) - 1)) & ~((align) - 1))

#define VIDEO_TRACKED_ALLOCS 64
typedef struct VideoTrackedAllocation {
    void* address;
    uint32_t size;
} VideoTrackedAllocation;

static VideoTrackedAllocation g_cpu_allocations[VIDEO_TRACKED_ALLOCS];
static VideoTrackedAllocation g_gpu_allocations[VIDEO_TRACKED_ALLOCS];
static volatile int g_memory_lock = 0;
static uint64_t g_video_cpu_bytes = 0;
static uint64_t g_video_cpu_peak = 0;
static uint64_t g_video_gpu_bytes = 0;
static uint64_t g_video_gpu_peak = 0;

static void video_memory_lock(void) {
    while (__sync_lock_test_and_set(&g_memory_lock, 1)) sceKernelDelayThread(50);
}

static void video_memory_unlock(void) {
    __sync_lock_release(&g_memory_lock);
}

static void video_track_allocation(VideoTrackedAllocation* entries,
                                   void* address, uint32_t size,
                                   uint64_t* current, uint64_t* peak) {
    if (address == NULL) return;
    video_memory_lock();
    for (int i = 0; i < VIDEO_TRACKED_ALLOCS; ++i) {
        if (entries[i].address != NULL) continue;
        entries[i].address = address;
        entries[i].size = size;
        *current += size;
        if (*current > *peak) *peak = *current;
        break;
    }
    video_memory_unlock();
}

static void video_untrack_allocation(VideoTrackedAllocation* entries,
                                     void* address, uint64_t* current) {
    if (address == NULL) return;
    video_memory_lock();
    for (int i = 0; i < VIDEO_TRACKED_ALLOCS; ++i) {
        if (entries[i].address != address) continue;
        uint32_t size = entries[i].size;
        entries[i].address = NULL;
        entries[i].size = 0;
        *current = *current > size ? *current - size : 0;
        break;
    }
    video_memory_unlock();
}

static void video_release_failed_init_allocations(void) {
    // sceAvPlayerInit may return an error without calling the replacement
    // deallocators. Release those orphaned blocks before GML/fallback retries
    // the open, otherwise every attempt permanently consumes several MiB.
    for (int i = 0; i < VIDEO_TRACKED_ALLOCS; ++i) {
        void* address = g_gpu_allocations[i].address;
        if (address != NULL) {
            video_untrack_allocation(g_gpu_allocations, address, &g_video_gpu_bytes);
            vglFree(address);
        }
    }
    for (int i = 0; i < VIDEO_TRACKED_ALLOCS; ++i) {
        void* address = g_cpu_allocations[i].address;
        if (address != NULL) {
            video_untrack_allocation(g_cpu_allocations, address, &g_video_cpu_bytes);
            free(address);
        }
    }
}

// ===[ Memory callbacks for sceAvPlayer ]===
static void* avp_alloc_cpu(void* p, uint32_t align, uint32_t size) {
    (void)p;
    void* result = memalign(align, size);
    video_track_allocation(g_cpu_allocations, result, size,
                           &g_video_cpu_bytes, &g_video_cpu_peak);
    return result;
}
static void avp_free_cpu(void* p, void* ptr) {
    (void)p;
    video_untrack_allocation(g_cpu_allocations, ptr, &g_video_cpu_bytes);
    free(ptr);
}
static void* avp_alloc_gpu(void* p, uint32_t align, uint32_t size) {
    (void)p;
    if (align < VIDEO_MEM_ALIGNMENT) align = VIDEO_MEM_ALIGNMENT;
    size = ALIGN_MEM(size, align);
    // Match YoYo Loader's proven SceAvPlayer path. PHYCONT only has a few MiB
    // left after VitaGL and made sceAvPlayerInit fail before producing a frame.
    // VGL_MEM_SLOW can spill into the much larger user-RAM pool and still
    // returns memory suitable for the decoder/GXM texture.
    void* res = vglAlloc(size, VGL_MEM_SLOW);
    if (res == NULL) return NULL;
    video_track_allocation(g_gpu_allocations, res, size,
                           &g_video_gpu_bytes, &g_video_gpu_peak);
    return res;
}
static void avp_free_gpu(void* p, void* addr) {
    (void)p;
    glFinish();
    video_untrack_allocation(g_gpu_allocations, addr, &g_video_gpu_bytes);
    vglFree(addr);
}

// ===[ Video state ]===
typedef enum {
    VS_INACTIVE = 0,
    VS_PLAYING  = 1,
    VS_PAUSED   = 2,
} VideoState;

static volatile VideoState g_state = VS_INACTIVE;
static SceAvPlayerHandle   g_player;
static SceUID              g_audio_thread_id = -1;
static int                 g_audio_port      = -1;
static int                 g_audio_port_new  = 0;
static int                 g_audio_old_len   = 0;
static int                 g_audio_old_freq  = 0;
static int                 g_audio_old_mode  = 0;

static GLuint           g_frame_tex[VIDEO_BUFFERS];
static SceGxmTexture*   g_frame_gxm[VIDEO_BUFFERS];
static int              g_frame_idx            = 0;
static int              g_first_frame_decoded  = 0;
static int              g_initialized          = 0;
static uint64_t         g_started_us           = 0;
static volatile int     g_pending_event        = VITA_VIDEO_EVENT_NONE;
static int              g_looping              = 0;
static float            g_volume               = 1.0f;
static float            g_duration_ms          = 40874.167f;
static float            g_video_width          = 640.0f;
static float            g_video_height         = 480.0f;
static uint64_t         g_paused_started_us    = 0;
static uint64_t         g_paused_total_us      = 0;
static int              g_first_frame_logged   = 0;
static int              g_first_draw_logged    = 0;
static int              g_decoder_wait_logged  = 0;
static SceUID           g_video_file           = -1;
static uint64_t         g_video_file_size      = 0;
static int              g_video_file_reads     = 0;
static volatile int     g_video_file_lock      = 0;

static void avp_file_lock(void) {
    while (__sync_lock_test_and_set(&g_video_file_lock, 1)) sceKernelDelayThread(50);
}

static void avp_file_unlock(void) {
    __sync_lock_release(&g_video_file_lock);
}

// Retail sceAvPlayer returns its controller object as an address in user RAM.
// SceAvPlayerHandle is an int, so valid 0x81xxxxxx..0x9xxxxxxx handles appear
// negative in a signed comparison. Treating them as errors prevented every
// video from reaching sceAvPlayerAddSource.
static bool video_player_handle_valid(SceAvPlayerHandle handle) {
    uint32_t value = (uint32_t)handle;
    return value >= 0x81000000u && value < 0xA0000000u && (value & 3u) == 0;
}

static void video_log(const char* state, const char* path, int result) {
    extern int g_vitaProbeLoggingEnabled;
    if (!g_vitaProbeLoggingEnabled) return;
    FILE* f = fopen("ux0:data/voidstranger/butterscotch-probe.log", "a");
    if (f != NULL) {
        fprintf(f, "VIDEO=%s result=0x%08X path=%s\n", state,
                (unsigned)result, path != NULL ? path : "<null>");
        fclose(f);
    }
}

// Use explicit file callbacks for ux0 sources. The stock sceAvPlayer path can
// accept AddSource yet never start decoding external files on some firmware;
// YoYo Loader's proven implementation supplies the file operations directly.
static int avp_file_open(void* object, const char* path) {
    (void)object;
    if (g_video_file >= 0) sceIoClose(g_video_file);
    g_video_file = sceIoOpen(path, SCE_O_RDONLY, 0);
    g_video_file_size = 0;
    g_video_file_reads = 0;
    if (g_video_file < 0) {
        video_log("file_callback_open_failed", path, g_video_file);
        return g_video_file;
    }
    SceIoStat st;
    memset(&st, 0, sizeof(st));
    if (sceIoGetstat(path, &st) >= 0) g_video_file_size = (uint64_t)st.st_size;
    video_log("file_callback_open", path, (int)g_video_file_size);
    return 0;
}

static int avp_file_close(void* object) {
    (void)object;
    int result = 0;
    if (g_video_file >= 0) result = sceIoClose(g_video_file);
    g_video_file = -1;
    return result;
}

static int avp_file_read(void* object, uint8_t* buffer, uint64_t offset, uint32_t length) {
    (void)object;
    if (g_video_file < 0) return -1;
    avp_file_lock();
    SceOff seek = sceIoLseek(g_video_file, (SceOff)offset, SCE_SEEK_SET);
    if (seek < 0) {
        avp_file_unlock();
        return (int)seek;
    }
    int result = sceIoRead(g_video_file, buffer, length);
    avp_file_unlock();
    if (result > 0) ++g_video_file_reads;
    return result;
}

static uint64_t avp_file_size(void* object) {
    (void)object;
    return g_video_file_size;
}

// ===[ Audio thread ]===
static int audio_thread_func(SceSize args, void* argp) {
    (void)args; (void)argp;
    // Match YoYo Loader's proven SceAvPlayer path. OpenAL already owns an
    // output port, so opening another MAIN port can fail silently and starve
    // the movie audio queue. Reuse an existing port when available; otherwise
    // open a BGM port dedicated to the video.
    g_audio_port = -1;
    g_audio_port_new = 0;
    for (int i = 0; i < 8; ++i) {
        if (sceAudioOutGetConfig(i, SCE_AUDIO_OUT_CONFIG_TYPE_LEN) < 0) continue;
        g_audio_port = i;
        g_audio_old_len = sceAudioOutGetConfig(i, SCE_AUDIO_OUT_CONFIG_TYPE_LEN);
        g_audio_old_freq = sceAudioOutGetConfig(i, SCE_AUDIO_OUT_CONFIG_TYPE_FREQ);
        g_audio_old_mode = sceAudioOutGetConfig(i, SCE_AUDIO_OUT_CONFIG_TYPE_MODE);
        break;
    }
    if (g_audio_port < 0) {
        g_audio_port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM, 1024,
                                           48000, SCE_AUDIO_OUT_MODE_STEREO);
        g_audio_port_new = g_audio_port >= 0 ? 1 : 0;
    }
    while (g_state != VS_INACTIVE) {
        if (g_state == VS_PLAYING && sceAvPlayerIsActive(g_player)) {
            SceAvPlayerFrameInfo frame;
            if (sceAvPlayerGetAudioData(g_player, &frame)) {
                if (g_audio_port < 0) continue;
                sceAudioOutSetConfig(g_audio_port, 1024, frame.details.audio.sampleRate,
                    frame.details.audio.channelCount == 1 ? SCE_AUDIO_OUT_MODE_MONO : SCE_AUDIO_OUT_MODE_STEREO);
                int volume = (int)(g_volume * SCE_AUDIO_VOLUME_0DB);
                int volumes[2] = { volume, volume };
                sceAudioOutSetVolume(g_audio_port,
                    SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, volumes);
                sceAudioOutOutput(g_audio_port, frame.pData);
            } else {
                sceKernelDelayThread(1000);
            }
        } else {
            sceKernelDelayThread(5000);
        }
    }
    if (g_audio_port >= 0) {
        if (g_audio_port_new) sceAudioOutReleasePort(g_audio_port);
        else sceAudioOutSetConfig(g_audio_port, g_audio_old_len, g_audio_old_freq,
                                  (SceAudioOutMode)g_audio_old_mode);
        g_audio_port = -1;
    }
    return sceKernelExitDeleteThread(0);
}

// ===[ Public API ]===
int VitaVideo_init(void) {
    if (g_initialized) return 0;
    int r = sceSysmoduleLoadModule(SCE_SYSMODULE_AVPLAYER);
    if (r < 0 && r != (int)0x80540101 /* already loaded */) return r;
    glGenTextures(VIDEO_BUFFERS, g_frame_tex);
    for (int i = 0; i < VIDEO_BUFFERS; i++) {
        glBindTexture(GL_TEXTURE_2D, g_frame_tex[i]);
        // Allocate a tiny placeholder; the actual data pointer will come from sceAvPlayer
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        g_frame_gxm[i] = vglGetGxmTexture(GL_TEXTURE_2D);
        vglFree(vglGetTexDataPointer(GL_TEXTURE_2D));
    }
    g_initialized = 1;
    return 0;
}

void VitaVideo_shutdown(void) {
    VitaVideo_close();
    if (g_initialized) {
        glDeleteTextures(VIDEO_BUFFERS, g_frame_tex);
        g_initialized = 0;
    }
}

int VitaVideo_open(const char* path) {
    if (!g_initialized) {
        int initResult = VitaVideo_init();
        if (initResult < 0) {
            video_log("init_failed", path, initResult);
            return initResult;
        }
    }
    VitaVideo_close(); // stop any previous video

    // GameMaker resolves vid/... through the game virtual filesystem. The
    // prepared Vita data keeps videos in the shared deltarunevita/vid folder,
    // outside chapter3. Prefer that canonical path and retain the per-chapter
    // layout only as a compatibility fallback for older data packs.
    char resolvedPath[384];
    char chapterPath[384];
    const char* sourcePath = path;
    extern int g_vitaActiveChapter;
    // Translation data.win files may pass an already-expanded path inside
    // mods/Lang/<language>/chapter3/vid. Videos are shared Vita assets, so
    // resolve every MP4 by basename before considering a chapter-local path.
    const char* videoName = NULL;
    if (path != NULL) {
        const char* slash = strrchr(path, '/');
        const char* backslash = strrchr(path, '\\');
        const char* separator = slash;
        if (backslash != NULL && (separator == NULL || backslash > separator))
            separator = backslash;
        videoName = separator != NULL ? separator + 1 : path;
    }
    if (videoName != NULL && strstr(videoName, ".mp4") != NULL) {
        snprintf(resolvedPath, sizeof(resolvedPath),
                 "ux0:data/voidstranger/vid/%s", videoName);
        struct stat fileInfo;
        if (stat(resolvedPath, &fileInfo) == 0) sourcePath = resolvedPath;
    } else if (path != NULL && strchr(path, ':') == NULL && path[0] != '/') {
        snprintf(resolvedPath, sizeof(resolvedPath),
                 "ux0:data/voidstranger/%s", path);
        struct stat fileInfo;
        if (stat(resolvedPath, &fileInfo) == 0) {
            sourcePath = resolvedPath;
        } else {
            snprintf(chapterPath, sizeof(chapterPath),
                     "ux0:data/voidstranger/chapter%d/%s",
                     g_vitaActiveChapter, path);
            sourcePath = chapterPath;
        }
    }

    SceAvPlayerInitData init;
    memset(&init, 0, sizeof(init));
    init.memoryReplacement.allocate          = avp_alloc_cpu;
    init.memoryReplacement.deallocate        = avp_free_cpu;
    init.memoryReplacement.allocateTexture   = avp_alloc_gpu;
    init.memoryReplacement.deallocateTexture = avp_free_gpu;
    init.fileReplacement.objectPointer = NULL;
    init.fileReplacement.open = avp_file_open;
    init.fileReplacement.close = avp_file_close;
    init.fileReplacement.readOffset = avp_file_read;
    init.fileReplacement.size = avp_file_size;
    init.basePriority                = 0xA0;
    init.numOutputVideoFrameBuffers  = VIDEO_BUFFERS;
    init.autoStart                   = GL_TRUE;

    g_player = sceAvPlayerInit(&init);
    if (!video_player_handle_valid(g_player)) {
        video_log("player_init_failed", sourcePath, (int)g_player);
        video_release_failed_init_allocations();
        return (int)g_player;
    }

    g_state = VS_PLAYING;
    g_frame_idx = 0;
    g_first_frame_decoded = 0;
    g_first_frame_logged = 0;
    g_first_draw_logged = 0;
    g_decoder_wait_logged = 0;
    g_started_us = sceKernelGetProcessTimeWide();
    g_paused_started_us = 0;
    g_paused_total_us = 0;
    g_video_width = 640.0f;
    g_video_height = 480.0f;
    g_duration_ms = strstr(sourcePath, "ch5_intro_") != NULL ? 30333.333f : 40874.167f;

    int r = sceAvPlayerAddSource(g_player, sourcePath);
    if (r < 0) {
        video_log("open_failed", sourcePath, r);
        VitaVideo_close();
        return r;
    }
    video_log("open_ok", sourcePath, r);
    sceAvPlayerSetLooping(g_player, g_looping ? GL_TRUE : GL_FALSE);
    g_pending_event = VITA_VIDEO_EVENT_START;

    // Start audio thread
    g_audio_thread_id = sceKernelCreateThread("vita_video_audio", audio_thread_func,
                                               0x10000100 - 10, 0x4000, 0, 0, NULL);
    if (g_audio_thread_id >= 0)
        sceKernelStartThread(g_audio_thread_id, 0, NULL);

    return 0;
}

bool VitaVideo_updateFrame(void) {
    if (g_state == VS_INACTIVE || !g_initialized) return false;

    if (g_state == VS_PLAYING) {
        int active = sceAvPlayerIsActive(g_player);
        if (active) {
            SceAvPlayerFrameInfo frame;
            if (sceAvPlayerGetVideoData(g_player, &frame)) {
                g_frame_idx = (g_frame_idx + 1) % VIDEO_BUFFERS;
                sceGxmTextureInitLinear(g_frame_gxm[g_frame_idx],
                    frame.pData,
                    SCE_GXM_TEXTURE_FORMAT_YVU420P2_CSC1,
                    frame.details.video.width,
                    frame.details.video.height, 0);
                sceGxmTextureSetMinFilter(g_frame_gxm[g_frame_idx], SCE_GXM_TEXTURE_FILTER_LINEAR);
                sceGxmTextureSetMagFilter(g_frame_gxm[g_frame_idx], SCE_GXM_TEXTURE_FILTER_LINEAR);
                g_video_width = (float)frame.details.video.width;
                g_video_height = (float)frame.details.video.height;
                g_first_frame_decoded = 1;
                if (!g_first_frame_logged) {
                    char dimensions[64];
                    snprintf(dimensions, sizeof(dimensions), "%ux%u",
                             frame.details.video.width, frame.details.video.height);
                    video_log("first_frame_decoded", dimensions, 0);
                    g_first_frame_logged = 1;
                }
            }
            if (!g_first_frame_decoded && !g_decoder_wait_logged &&
                sceKernelGetProcessTimeWide() - g_started_us > 2000000ULL) {
                char detail[96];
                snprintf(detail, sizeof(detail), "active=1 reads=%d size=%llu",
                         g_video_file_reads, (unsigned long long)g_video_file_size);
                video_log("waiting_first_frame", detail, 0);
                g_decoder_wait_logged = 1;
            }
        } else if (g_first_frame_decoded) {
            // Video finished
            sceAvPlayerStop(g_player);
            sceAvPlayerClose(g_player);
            g_state = VS_INACTIVE;
            g_pending_event = VITA_VIDEO_EVENT_END;
            return false;
        } else if (!g_decoder_wait_logged &&
                   sceKernelGetProcessTimeWide() - g_started_us > 2000000ULL) {
            char detail[96];
            snprintf(detail, sizeof(detail), "active=0 reads=%d size=%llu",
                     g_video_file_reads, (unsigned long long)g_video_file_size);
            video_log("waiting_first_frame", detail, 0);
            g_decoder_wait_logged = 1;
        }
    }

    return g_state != VS_INACTIVE;
}

bool VitaVideo_hasDecodedFrame(void) {
    return g_state != VS_INACTIVE && g_first_frame_decoded != 0;
}

void VitaVideo_draw(float x, float y, float w, float h) {
    if (g_state == VS_INACTIVE || !g_initialized || !g_first_frame_decoded) return;

    // Draw the last decoded frame as a full-screen quad inside the given rect
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, g_frame_tex[g_frame_idx]);
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(x,     y    );
        glTexCoord2f(1.0f, 0.0f); glVertex2f(x + w, y    );
        glTexCoord2f(0.0f, 1.0f); glVertex2f(x,     y + h);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(x + w, y + h);
    glEnd();
    glEnable(GL_BLEND);
}

void VitaVideo_drawHost(float x, float y, float w, float h) {
    if (!VitaVideo_hasDecodedFrame()) return;

    glViewport(0, 0, 960, 544);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrthof(0.0f, 960.0f, 544.0f, 0.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    VitaVideo_draw(x, y, w, h);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    if (!g_first_draw_logged) {
        video_log("first_frame_presented", "host_overlay", 0);
        g_first_draw_logged = 1;
    }
}

void VitaVideo_close(void) {
    if (g_state == VS_INACTIVE) return;
    g_state = VS_INACTIVE; // signals audio thread to stop
    if (g_audio_thread_id >= 0) {
        SceUInt timeout = 3000000;
        sceKernelWaitThreadEnd(g_audio_thread_id, NULL, &timeout);
        g_audio_thread_id = -1;
    }
    if (video_player_handle_valid(g_player)) {
        sceAvPlayerStop(g_player);
        sceAvPlayerClose(g_player);
    }
    g_first_frame_decoded = 0;
}

void VitaVideo_pause(void) {
    if (g_state == VS_PLAYING) {
        sceAvPlayerPause(g_player);
        g_state = VS_PAUSED;
        g_paused_started_us = sceKernelGetProcessTimeWide();
    }
}

void VitaVideo_resume(void) {
    if (g_state == VS_PAUSED) {
        sceAvPlayerResume(g_player);
        g_state = VS_PLAYING;
        if (g_paused_started_us != 0) {
            g_paused_total_us += sceKernelGetProcessTimeWide() - g_paused_started_us;
            g_paused_started_us = 0;
        }
    }
}

int VitaVideo_getStatus(void) {
    if (g_state == VS_PLAYING) return VITA_VIDEO_STATUS_PLAYING;
    if (g_state == VS_PAUSED) return VITA_VIDEO_STATUS_PAUSED;
    return VITA_VIDEO_STATUS_NONE;
}

float VitaVideo_getDuration(void) {
    return g_state == VS_INACTIVE ? 0.0f : g_duration_ms;
}

float VitaVideo_getPosition(void) {
    if (g_state == VS_INACTIVE || g_started_us == 0) return 0.0f;
    uint64_t now = sceKernelGetProcessTimeWide();
    uint64_t paused = g_paused_total_us;
    if (g_state == VS_PAUSED && g_paused_started_us != 0) paused += now - g_paused_started_us;
    uint64_t elapsed = now > g_started_us + paused ? now - g_started_us - paused : 0;
    return (float)elapsed / 1000.0f;
}

float VitaVideo_getWidth(void) { return g_video_width; }
float VitaVideo_getHeight(void) { return g_video_height; }

int VitaVideo_consumeEvent(void) {
    int event = g_pending_event;
    g_pending_event = VITA_VIDEO_EVENT_NONE;
    return event;
}

void VitaVideo_setLooping(bool enabled) {
    g_looping = enabled ? 1 : 0;
    if (g_state != VS_INACTIVE) sceAvPlayerSetLooping(g_player, enabled ? GL_TRUE : GL_FALSE);
}

void VitaVideo_setVolume(float volume) {
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    g_volume = volume;
}

void VitaVideo_getMemoryStats(VitaVideoMemoryStats* stats) {
    if (stats == NULL) return;
    video_memory_lock();
    stats->cpuBytes = g_video_cpu_bytes;
    stats->cpuPeakBytes = g_video_cpu_peak;
    stats->gpuBytes = g_video_gpu_bytes;
    stats->gpuPeakBytes = g_video_gpu_peak;
    video_memory_unlock();
}
