#ifndef VOIDSTRANGER_FRUIT_TOUCH_H
#define VOIDSTRANGER_FRUIT_TOUCH_H

#include <stdbool.h>

/* A touch is a confirm pulse only on a fresh contact while the actual
 * obj_orange eating state is active. Holding the panel is intentionally not
 * auto-repeat: this mirrors scr_input_check_pressed(4) / Cross. */
static inline bool fruit_touch_should_pulse(bool canEat, bool touching, bool wasTouching) {
    return canEat && touching && !wasTouching;
}

static inline bool fruit_touch_hint_visible(bool canEat) {
    return canEat;
}

#endif
