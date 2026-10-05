#pragma once

static inline int audio_stream_refill_budget(float deltaTime) {
    return deltaTime >= 0.05f ? 2 : 1;
}
