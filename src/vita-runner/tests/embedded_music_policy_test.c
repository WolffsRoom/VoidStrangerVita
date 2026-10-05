#include <assert.h>
#include <stdio.h>
#include "../../butterscotch/src/audio/openal/embedded_music_policy.h"

int main(void) {
    const unsigned char ogg[4] = {'O','g','g','S'};
    const unsigned char wav[4] = {'R','I','F','F'};
    assert(audio_should_stream_embedded_music(1, ogg, 4));
    assert(!audio_should_stream_embedded_music(0, ogg, 4));
    assert(!audio_should_stream_embedded_music(1, wav, 4));
    assert(!audio_should_stream_embedded_music(1, ogg, 3));
    puts("embedded_music_policy_test: ok");
    return 0;
}
