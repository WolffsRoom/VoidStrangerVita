#ifndef MISSING_DATA_SCENE_POLICY_H
#define MISSING_DATA_SCENE_POLICY_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

enum {
    MISSING_DATA_REQ_DATA_WIN    = 1u << 0,
    MISSING_DATA_REQ_AUDIOGROUP1 = 1u << 1,
    MISSING_DATA_REQ_AUDIOGROUP2 = 1u << 2,
    MISSING_DATA_REQ_CSV         = 1u << 3
};

typedef enum MissingDataLayoutState {
    MISSING_DATA_LAYOUT_READY = 0,
    MISSING_DATA_LAYOUT_ORGANIZABLE,
    MISSING_DATA_LAYOUT_MISSING,
    MISSING_DATA_LAYOUT_AMBIGUOUS
} MissingDataLayoutState;

static inline MissingDataLayoutState missing_data_layout_state(uint32_t rootMissingMask,
                                                               uint32_t misplacedMask,
                                                               uint32_t duplicateMask) {
    if (rootMissingMask == 0u) return MISSING_DATA_LAYOUT_READY;
    if ((duplicateMask & rootMissingMask) != 0u) return MISSING_DATA_LAYOUT_AMBIGUOUS;
    if ((misplacedMask & rootMissingMask) == rootMissingMask)
        return MISSING_DATA_LAYOUT_ORGANIZABLE;
    return MISSING_DATA_LAYOUT_MISSING;
}
typedef enum MissingDataStage {
    MISSING_DATA_STAGE_HELLO = 0,
    MISSING_DATA_STAGE_NEED_FILES,
    MISSING_DATA_STAGE_QUESTION,
    MISSING_DATA_STAGE_YES_PLACE,
    MISSING_DATA_STAGE_YES_CHECK,
    MISSING_DATA_STAGE_RECHECK,
    MISSING_DATA_STAGE_MISSING,
    MISSING_DATA_STAGE_INSTALL_HELP,
    MISSING_DATA_STAGE_DONE
} MissingDataStage;

typedef enum MissingDataPortraitMood {
    MISSING_DATA_PORTRAIT_NEUTRAL = 0,
    MISSING_DATA_PORTRAIT_HAPPY,
    MISSING_DATA_PORTRAIT_SIGH
} MissingDataPortraitMood;

typedef struct MissingDataTypewriter {
    int halfCounter;
    int visibleChars;
    int pauseTicks;
} MissingDataTypewriter;

#define MISSING_DATA_CURRENT_DATA_WIN_SHA256 "9CE2BAB66D6EDB3FB6506BEDEFC634BCB354AD034D828F8BA3DC679777C6E00A"
#define MISSING_DATA_CURRENT_DATA_WIN_SIZE 46697708ULL

static inline void missing_data_typewriter_reset(MissingDataTypewriter* state) {
    if (state == NULL) return;
    state->halfCounter = 0;
    state->visibleChars = 0;
    state->pauseTicks = 0;
}

static inline bool missing_data_typewriter_step(MissingDataTypewriter* state,
                                                const char* text, int length) {
    if (state == NULL || text == NULL || length <= 0 || state->visibleChars >= length) return false;
    if (state->pauseTicks > 0) {
        state->pauseTicks--;
        return false;
    }
    state->halfCounter++;
    int nextVisible = state->halfCounter / 2;
    if (nextVisible <= state->visibleChars) return false;
    if (nextVisible > length) nextVisible = length;
    state->visibleChars = nextVisible;
    char revealed = text[state->visibleChars - 1];
    if (revealed == ',') state->pauseTicks = 16;
    return (state->visibleChars % 3) == 0;
}

static inline void missing_data_typewriter_force(MissingDataTypewriter* state, int length) {
    if (state == NULL) return;
    if (length < 0) length = 0;
    state->visibleChars = length;
    state->halfCounter = length * 2;
    state->pauseTicks = 0;
}

static inline MissingDataPortraitMood missing_data_portrait_mood(bool success, bool negative) {
    if (success) return MISSING_DATA_PORTRAIT_HAPPY;
    if (negative) return MISSING_DATA_PORTRAIT_SIGH;
    return MISSING_DATA_PORTRAIT_NEUTRAL;
}

static inline bool missing_data_candidate_name_is_win(const char* name) {
    if (name == NULL) return false;
    size_t length = strlen(name);
    if (length < 4) return false;
    const char* ext = name + length - 4;
    return ext[0] == '.' &&
           (ext[1] == 'w' || ext[1] == 'W') &&
           (ext[2] == 'i' || ext[2] == 'I') &&
           (ext[3] == 'n' || ext[3] == 'N');
}

static inline bool missing_data_sha256_hex_is_current(const char* hex) {
    if (hex == NULL || strlen(hex) != 64) return false;
    const char* expected = MISSING_DATA_CURRENT_DATA_WIN_SHA256;
    for (int i = 0; i < 64; ++i) {
        char a = hex[i];
        char b = expected[i];
        if (a >= 'a' && a <= 'f') a = (char)(a - 'a' + 'A');
        if (b >= 'a' && b <= 'f') b = (char)(b - 'a' + 'A');
        if (a != b) return false;
    }
    return true;
}
typedef enum MissingDataAction {
    MISSING_DATA_ACTION_NONE = 0,
    MISSING_DATA_ACTION_CONFIRM,
    MISSING_DATA_ACTION_YES,
    MISSING_DATA_ACTION_NO,
    MISSING_DATA_ACTION_RECHECK_OK,
    MISSING_DATA_ACTION_RECHECK_FAILED
} MissingDataAction;

static inline uint32_t missing_data_required_mask(bool dataWinExists,
                                                  bool dataWinSizeValid,
                                                  bool audio1Exists,
                                                  bool audio2Exists,
                                                  bool csvExists) {
    uint32_t mask = 0u;
    if (!dataWinExists || !dataWinSizeValid) mask |= MISSING_DATA_REQ_DATA_WIN;
    if (!audio1Exists) mask |= MISSING_DATA_REQ_AUDIOGROUP1;
    if (!audio2Exists) mask |= MISSING_DATA_REQ_AUDIOGROUP2;
    if (!csvExists) mask |= MISSING_DATA_REQ_CSV;
    return mask;
}

static inline int missing_data_help_page_after_confirm(int page) {
    if (page < 3) return page + 1;
    return -1;
}

static inline float missing_data_fade_alpha(uint64_t elapsedUs, uint64_t durationUs) {
    if (durationUs == 0ULL || elapsedUs >= durationUs) return 1.0f;
    return (float)elapsedUs / (float)durationUs;
}
static inline MissingDataStage missing_data_next_stage(MissingDataStage stage,
                                                       MissingDataAction action) {
    switch (stage) {
        case MISSING_DATA_STAGE_HELLO:
            return action == MISSING_DATA_ACTION_CONFIRM ? MISSING_DATA_STAGE_NEED_FILES : stage;
        case MISSING_DATA_STAGE_NEED_FILES:
            return action == MISSING_DATA_ACTION_CONFIRM ? MISSING_DATA_STAGE_QUESTION : stage;
        case MISSING_DATA_STAGE_QUESTION:
            if (action == MISSING_DATA_ACTION_YES) return MISSING_DATA_STAGE_YES_PLACE;
            if (action == MISSING_DATA_ACTION_NO) return MISSING_DATA_STAGE_INSTALL_HELP;
            return stage;
        case MISSING_DATA_STAGE_YES_PLACE:
            return action == MISSING_DATA_ACTION_CONFIRM ? MISSING_DATA_STAGE_YES_CHECK : stage;
        case MISSING_DATA_STAGE_YES_CHECK:
            return action == MISSING_DATA_ACTION_CONFIRM ? MISSING_DATA_STAGE_RECHECK : stage;
        case MISSING_DATA_STAGE_RECHECK:
            if (action == MISSING_DATA_ACTION_RECHECK_OK) return MISSING_DATA_STAGE_DONE;
            if (action == MISSING_DATA_ACTION_RECHECK_FAILED) return MISSING_DATA_STAGE_MISSING;
            return stage;
        case MISSING_DATA_STAGE_MISSING:
            return action == MISSING_DATA_ACTION_CONFIRM ? MISSING_DATA_STAGE_INSTALL_HELP : stage;
        case MISSING_DATA_STAGE_INSTALL_HELP:
        case MISSING_DATA_STAGE_DONE:
        default:
            return stage;
    }
}

#endif
