#ifndef ARM_CONTROL_H
#define ARM_CONTROL_H

#include <stdint.h>

uint32_t disableInts(void);
void enableInts(uint32_t ints);

#endif
