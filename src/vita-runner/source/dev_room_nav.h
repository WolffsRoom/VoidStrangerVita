#pragma once

#include <stdint.h>

static inline int32_t vita_dev_room_order_target(int32_t currentOrderPosition,
                                                  uint32_t roomOrderCount,
                                                  int direction) {
    if (roomOrderCount == 0 || currentOrderPosition < 0 ||
        (uint32_t)currentOrderPosition >= roomOrderCount || direction == 0)
        return -1;
    int32_t target = currentOrderPosition + (direction < 0 ? -1 : 1);
    if (target < 0 || (uint32_t)target >= roomOrderCount) return -1;
    return target;
}
