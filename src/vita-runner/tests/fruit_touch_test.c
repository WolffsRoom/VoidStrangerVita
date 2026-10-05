#include <assert.h>
#include <stdio.h>
#include "../source/fruit_touch.h"

int main(void) {
    assert(!fruit_touch_should_pulse(false, true, false));
    assert(!fruit_touch_should_pulse(false, true, true));
    assert(fruit_touch_should_pulse(true, true, false));
    assert(!fruit_touch_should_pulse(true, true, true));
    assert(!fruit_touch_should_pulse(true, false, true));
    assert(fruit_touch_hint_visible(true));
    assert(!fruit_touch_hint_visible(false));
    puts("fruit_touch_test: ok");
    return 0;
}
