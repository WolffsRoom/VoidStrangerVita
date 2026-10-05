#include <assert.h>
#include <stdio.h>
#include "../source/dev_room_nav.h"

int main(void) {
    assert(vita_dev_room_order_target(10, 100, -1) == 9);
    assert(vita_dev_room_order_target(10, 100, 1) == 11);
    assert(vita_dev_room_order_target(0, 100, -1) == -1);
    assert(vita_dev_room_order_target(99, 100, 1) == -1);
    assert(vita_dev_room_order_target(-1, 100, 1) == -1);
    assert(vita_dev_room_order_target(10, 0, 1) == -1);
    puts("dev_room_nav_test: ok");
    return 0;
}
