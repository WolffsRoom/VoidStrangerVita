#ifndef BS_AUDIO_GAIN_POLICY_H
#define BS_AUDIO_GAIN_POLICY_H

typedef enum AudioGroupCategory {
    AUDIO_GROUP_CATEGORY_OTHER = 0,
    AUDIO_GROUP_CATEGORY_MUSIC,
    AUDIO_GROUP_CATEGORY_SFX
} AudioGroupCategory;

static inline float audio_combined_master_gain(float gameGain, float platformGain) {
    return gameGain * platformGain;
}

static inline AudioGroupCategory audio_group_category(int groupIndex) {
    if (groupIndex == 1) return AUDIO_GROUP_CATEGORY_MUSIC;
    if (groupIndex == 2) return AUDIO_GROUP_CATEGORY_SFX;
    return AUDIO_GROUP_CATEGORY_OTHER;
}

static inline float audio_combined_category_gain(int music,
                                                  float platformMusicGain,
                                                  float platformSfxGain,
                                                  float gameMusicGain,
                                                  float gameSfxGain) {
    return music ? platformMusicGain * gameMusicGain
                 : platformSfxGain * gameSfxGain;
}

#endif
