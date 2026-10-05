#include <assert.h>
#include <stdio.h>
#include "../../butterscotch/src/audio/openal/audio_gain_policy.h"

int main(void) {
    assert(audio_combined_master_gain(1.0f, 1.0f) == 1.0f);
    assert(audio_combined_master_gain(0.5f, 0.8f) > 0.399f && audio_combined_master_gain(0.5f, 0.8f) < 0.401f);
    assert(audio_group_category(1) == AUDIO_GROUP_CATEGORY_MUSIC);
    assert(audio_group_category(2) == AUDIO_GROUP_CATEGORY_SFX);
    assert(audio_group_category(0) == AUDIO_GROUP_CATEGORY_OTHER);
    float music = audio_combined_category_gain(1, 0.75f, 0.25f, 0.8f, 0.5f);
    float sfx = audio_combined_category_gain(0, 0.75f, 0.25f, 0.8f, 0.5f);
    assert(music > 0.599f && music < 0.601f);
    assert(sfx > 0.124f && sfx < 0.126f);
    puts("audio_gain_policy_test: ok");
    return 0;
}
