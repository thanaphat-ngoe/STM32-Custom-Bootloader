#ifndef INC_TIMER_H
#define INC_TIMER_H

#include "common-defines.h"

typedef struct timer_t {
    uint64_t wait_time;
    uint64_t target_time;
    bool auto_reset;
    bool has_elapsed;
} timer_t;

void timer_setup(timer_t* timer, uint64_t wait_time, bool auto_reset);
bool timer_has_elapsed(timer_t* timer);
void timer_reset(timer_t* timer);

#endif
