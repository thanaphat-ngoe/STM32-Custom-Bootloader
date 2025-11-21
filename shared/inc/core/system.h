#ifndef INC_SYSTEM_H
#define INC_SYSTEM_H

#include "common-defines.h"

#define CPU_FREQ (32000000)
#define SYSTICK_FREQ (1000)

void system_setup(void);
uint32_t system_get_ticks(void);
void system_delay(uint32_t millisecond);

#endif
