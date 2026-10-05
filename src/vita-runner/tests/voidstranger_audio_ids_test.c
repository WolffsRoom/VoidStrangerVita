#include <assert.h>
#include <stdio.h>
#include "../source/voidstranger_audio_ids.h"

int main(void) {
    assert(voidstranger_settings_sound_id(0) == 103);
    assert(voidstranger_settings_sound_id(1) == 105);
    assert(voidstranger_settings_sound_id(2) == 104);
    assert(voidstranger_settings_sound_id(-1) == -1);
    assert(voidstranger_settings_sound_id(3) == -1);
    assert(VOIDSTRANGER_LOADING_STAIRS_SOUND_ID == 110);
    assert(VOIDSTRANGER_LOADING_PLACE_SOUND_ID == 114);
    assert(VOIDSTRANGER_LOADING_FALL_SOUND_ID == 115);
    puts("voidstranger_audio_ids_test: ok");
    return 0;
}
