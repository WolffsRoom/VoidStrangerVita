#ifndef BS_EMBEDDED_MUSIC_POLICY_H
#define BS_EMBEDDED_MUSIC_POLICY_H

#include <stddef.h>
#include <string.h>

static inline int audio_should_stream_embedded_music(int music,
                                                     const unsigned char* data,
                                                     size_t size) {
    return music && data != NULL && size >= 4 && memcmp(data, "OggS", 4) == 0;
}

#endif
