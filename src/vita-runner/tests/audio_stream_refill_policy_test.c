#include <assert.h>
#include <stdio.h>
#include "../../butterscotch/src/audio/openal/audio_stream_refill_policy.h"

int main(void) {
    assert(audio_stream_refill_budget(1.0f / 60.0f) == 1);
    assert(audio_stream_refill_budget(1.0f / 30.0f) == 1);
    assert(audio_stream_refill_budget(0.050f) == 2);
    assert(audio_stream_refill_budget(0.100f) == 2);
    puts("audio_stream_refill_policy_test: ok");
    return 0;
}
