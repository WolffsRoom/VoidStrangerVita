#include <assert.h>
#include <stdio.h>
#include "../source/loading_animation.h"

int main(void) {
    assert(loading_target_cell_for_percent(0, 11) == 0);
    assert(loading_target_cell_for_percent(7, 11) == 0);
    assert(loading_target_cell_for_percent(8, 11) == 1);
    assert(loading_target_cell_for_percent(15, 11) == 1);
    assert(loading_target_cell_for_percent(16, 11) == 2);
    assert(loading_target_cell_for_percent(87, 11) == 10);
    assert(loading_target_cell_for_percent(88, 11) == 11);
    assert(loading_target_cell_for_percent(100, 11) == 11);

    assert(loading_step_phase(0) == LOADING_STEP_PLACE);
    assert(loading_step_phase(LOADING_PLACE_DURATION_US - 1) == LOADING_STEP_PLACE);
    assert(loading_step_phase(LOADING_PLACE_DURATION_US) == LOADING_STEP_WALK);
    assert(loading_step_phase(LOADING_STEP_DURATION_US - 1) == LOADING_STEP_WALK);
    assert(loading_step_phase(LOADING_STEP_DURATION_US) == LOADING_STEP_DONE);

    assert(loading_walk_progress(LOADING_PLACE_DURATION_US) == 0.0f);
    assert(loading_walk_progress(LOADING_STEP_DURATION_US) == 1.0f);

    assert(loading_intro_fade_alpha(0) == 0.0f);
    float introHalf = loading_intro_fade_alpha(LOADING_INTRO_FADE_US / 2);
    assert(introHalf > 0.49f && introHalf < 0.51f);
    assert(loading_intro_fade_alpha(LOADING_INTRO_FADE_US) == 1.0f);
    assert(loading_intro_fade_alpha(LOADING_INTRO_FADE_US + 1) == 1.0f);

    float v0 = loading_visual_ratio(0, false, 0, 11);
    float vHalf = loading_visual_ratio(5, true, LOADING_STEP_DURATION_US / 2, 11);
    float vLast = loading_visual_ratio(10, true, LOADING_STEP_DURATION_US - 1, 11);
    float vDone = loading_visual_ratio(11, false, 0, 11);
    assert(v0 == 0.0f);
    assert(vHalf > 0.49f && vHalf < 0.51f);
    assert(vLast > 0.99f && vLast < 1.0f);
    assert(vDone == 1.0f);

    puts("loading_animation_test: ok");
    return 0;
}
