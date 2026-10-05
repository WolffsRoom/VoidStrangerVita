#ifndef VOIDSTRANGER_AUDIO_IDS_H
#define VOIDSTRANGER_AUDIO_IDS_H

#define VOIDSTRANGER_SND_MENU_1 103
#define VOIDSTRANGER_SND_MENU_2 104
#define VOIDSTRANGER_SND_MENU_3 105
#define VOIDSTRANGER_LOADING_STAIRS_SOUND_ID 110 /* snd_stairs */
#define VOIDSTRANGER_LOADING_PLACE_SOUND_ID 114
#define VOIDSTRANGER_LOADING_FALL_SOUND_ID 115 /* snd_player_fall */

static inline int voidstranger_settings_sound_id(int type) {
    switch (type) {
        case 0: return VOIDSTRANGER_SND_MENU_1;
        case 1: return VOIDSTRANGER_SND_MENU_3;
        case 2: return VOIDSTRANGER_SND_MENU_2;
        default: return -1;
    }
}

#endif
