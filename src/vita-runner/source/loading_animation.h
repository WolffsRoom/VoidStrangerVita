#ifndef VOIDSTRANGER_LOADING_ANIMATION_H
#define VOIDSTRANGER_LOADING_ANIMATION_H

#include <stdint.h>
#include <stdbool.h>

#define LOADING_PROGRESS_STEP_PERCENT 8
#define LOADING_PLACE_DURATION_US 180000ULL
#define LOADING_WALK_DURATION_US 220000ULL
#define LOADING_STEP_DURATION_US (LOADING_PLACE_DURATION_US + LOADING_WALK_DURATION_US)
#define LOADING_INTRO_FADE_US 400000ULL

typedef enum LoadingStepPhase {
    LOADING_STEP_PLACE = 0,
    LOADING_STEP_WALK,
    LOADING_STEP_DONE
} LoadingStepPhase;

static inline int loading_target_cell_for_percent(int percent, int floorCount) {
    if (floorCount <= 0 || percent <= 0) return 0;
    if (percent > 100) percent = 100;
    int cell = percent / LOADING_PROGRESS_STEP_PERCENT;
    if (cell > floorCount) cell = floorCount;
    return cell;
}

static inline LoadingStepPhase loading_step_phase(uint64_t elapsedUs) {
    if (elapsedUs < LOADING_PLACE_DURATION_US) return LOADING_STEP_PLACE;
    if (elapsedUs < LOADING_STEP_DURATION_US) return LOADING_STEP_WALK;
    return LOADING_STEP_DONE;
}

static inline float loading_walk_progress(uint64_t elapsedUs) {
    if (elapsedUs <= LOADING_PLACE_DURATION_US) return 0.0f;
    if (elapsedUs >= LOADING_STEP_DURATION_US) return 1.0f;
    return (float)(elapsedUs - LOADING_PLACE_DURATION_US) / (float)LOADING_WALK_DURATION_US;
}

static inline float loading_intro_fade_alpha(uint64_t elapsedUs) {
    if (elapsedUs == 0) return 0.0f;
    if (elapsedUs >= LOADING_INTRO_FADE_US) return 1.0f;
    return (float)elapsedUs / (float)LOADING_INTRO_FADE_US;
}

/* Cached-texture validation can complete before the first visible step.  In
 * that mode the loader's visual clock is authoritative: the percentage is the
 * exact fractional position through Gray's place+walk sequence. */
static inline float loading_visual_ratio(int currentCell, bool stepActive,
                                         uint64_t stepElapsedUs, int floorCount) {
    if (floorCount <= 0) return 1.0f;
    if (currentCell <= 0 && !stepActive) return 0.0f;
    if (currentCell >= floorCount) return 1.0f;
    float segment = (float)(currentCell < 0 ? 0 : currentCell);
    if (stepActive) {
        float within = (float)stepElapsedUs / (float)LOADING_STEP_DURATION_US;
        if (within < 0.0f) within = 0.0f;
        if (within > 1.0f) within = 1.0f;
        segment += within;
    }
    float ratio = segment / (float)floorCount;
    if (ratio < 0.0f) return 0.0f;
    if (ratio > 1.0f) return 1.0f;
    return ratio;
}

#endif
