#include <assert.h>
#include <stdio.h>
#include "../source/missing_data_scene_policy.h"

int main(void) {
    assert(missing_data_layout_state(0u, 0u, 0u) == MISSING_DATA_LAYOUT_READY);
    assert(missing_data_layout_state(MISSING_DATA_REQ_DATA_WIN, MISSING_DATA_REQ_DATA_WIN, 0u) == MISSING_DATA_LAYOUT_ORGANIZABLE);
    assert(missing_data_layout_state(MISSING_DATA_REQ_DATA_WIN | MISSING_DATA_REQ_AUDIOGROUP1,
                                     MISSING_DATA_REQ_DATA_WIN, 0u) == MISSING_DATA_LAYOUT_MISSING);
    assert(missing_data_layout_state(MISSING_DATA_REQ_DATA_WIN, MISSING_DATA_REQ_DATA_WIN,
                                     MISSING_DATA_REQ_DATA_WIN) == MISSING_DATA_LAYOUT_AMBIGUOUS);

    assert(missing_data_required_mask(true, true, true, true, true) == 0u);
    assert((missing_data_required_mask(false, true, true, true, true) & MISSING_DATA_REQ_DATA_WIN) != 0u);
    assert((missing_data_required_mask(true, false, true, true, true) & MISSING_DATA_REQ_DATA_WIN) != 0u);
    assert((missing_data_required_mask(true, true, false, true, true) & MISSING_DATA_REQ_AUDIOGROUP1) != 0u);
    assert((missing_data_required_mask(true, true, true, false, true) & MISSING_DATA_REQ_AUDIOGROUP2) != 0u);
    assert((missing_data_required_mask(true, true, true, true, false) & MISSING_DATA_REQ_CSV) != 0u);

    assert(missing_data_next_stage(MISSING_DATA_STAGE_HELLO, MISSING_DATA_ACTION_CONFIRM) == MISSING_DATA_STAGE_NEED_FILES);
    assert(missing_data_next_stage(MISSING_DATA_STAGE_NEED_FILES, MISSING_DATA_ACTION_CONFIRM) == MISSING_DATA_STAGE_QUESTION);
    assert(missing_data_next_stage(MISSING_DATA_STAGE_QUESTION, MISSING_DATA_ACTION_YES) == MISSING_DATA_STAGE_YES_PLACE);
    assert(missing_data_next_stage(MISSING_DATA_STAGE_YES_PLACE, MISSING_DATA_ACTION_CONFIRM) == MISSING_DATA_STAGE_YES_CHECK);
    assert(missing_data_next_stage(MISSING_DATA_STAGE_YES_CHECK, MISSING_DATA_ACTION_CONFIRM) == MISSING_DATA_STAGE_RECHECK);
    assert(missing_data_next_stage(MISSING_DATA_STAGE_QUESTION, MISSING_DATA_ACTION_NO) == MISSING_DATA_STAGE_INSTALL_HELP);
    assert(missing_data_next_stage(MISSING_DATA_STAGE_RECHECK, MISSING_DATA_ACTION_RECHECK_FAILED) == MISSING_DATA_STAGE_MISSING);
    assert(missing_data_next_stage(MISSING_DATA_STAGE_RECHECK, MISSING_DATA_ACTION_RECHECK_OK) == MISSING_DATA_STAGE_DONE);
    assert(missing_data_help_page_after_confirm(1) == 2);
    assert(missing_data_help_page_after_confirm(2) == 3);
    assert(missing_data_help_page_after_confirm(3) == -1);
    assert(missing_data_fade_alpha(0ULL, 1000000ULL) == 0.0f);
    assert(missing_data_fade_alpha(500000ULL, 1000000ULL) > 0.49f);
    assert(missing_data_fade_alpha(500000ULL, 1000000ULL) < 0.51f);
    assert(missing_data_fade_alpha(1000000ULL, 1000000ULL) == 1.0f);

    /* Original-like typewriter: text_speed=0.5 at 60 Hz -> one character per 2 ticks. */
    MissingDataTypewriter tw;
    missing_data_typewriter_reset(&tw);
    assert(missing_data_typewriter_step(&tw, "ABC", 3) == false);
    assert(tw.visibleChars == 0);
    assert(missing_data_typewriter_step(&tw, "ABC", 3) == false);
    assert(tw.visibleChars == 1);
    for (int i = 0; i < 3; ++i) (void)missing_data_typewriter_step(&tw, "ABC", 3);
    assert(tw.visibleChars == 2);
    assert(missing_data_typewriter_step(&tw, "ABC", 3) == true);
    assert(tw.visibleChars == 3);

    missing_data_typewriter_reset(&tw);
    for (int i = 0; i < 4; ++i) (void)missing_data_typewriter_step(&tw, "A,B", 3);
    assert(tw.visibleChars == 2);
    assert(tw.pauseTicks == 16);
    for (int i = 0; i < 16; ++i) (void)missing_data_typewriter_step(&tw, "A,B", 3);
    assert(tw.visibleChars == 2);
    (void)missing_data_typewriter_step(&tw, "A,B", 3);
    assert(tw.visibleChars == 2);
    assert(missing_data_typewriter_step(&tw, "A,B", 3) == true);
    assert(tw.visibleChars == 3);
    missing_data_typewriter_force(&tw, 12);
    assert(tw.visibleChars == 12);

    assert(missing_data_portrait_mood(false, false) == MISSING_DATA_PORTRAIT_NEUTRAL);
    assert(missing_data_portrait_mood(true, false) == MISSING_DATA_PORTRAIT_HAPPY);
    assert(missing_data_portrait_mood(false, true) == MISSING_DATA_PORTRAIT_SIGH);

    assert(missing_data_candidate_name_is_win("data.win"));
    assert(missing_data_candidate_name_is_win("voidstranger-vita.win"));
    assert(!missing_data_candidate_name_is_win("data.winx"));
    assert(missing_data_sha256_hex_is_current("841211AE9B699589461F27F0B4AFD6F95551F1D51D94AA4447D20B887EF9C50A"));
    assert(missing_data_sha256_hex_is_current("841211ae9b699589461f27f0b4afd6f95551f1d51d94aa4447d20b887ef9c50a"));
    assert(!missing_data_sha256_hex_is_current("0CE2BAB66D6EDB3FB6506BEDEFC634BCB354AD034D828F8BA3DC679777C6E00A"));
    puts("missing_data_scene_policy_test: ok");
    return 0;
}
