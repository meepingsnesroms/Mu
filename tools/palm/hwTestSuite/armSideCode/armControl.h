#ifndef ARM_CONTROL_H
#define ARM_CONTROL_H

#include <stdint.h>
#include <stdbool.h>

uint32_t disableInts(void);
void enableInts(uint32_t ints);
bool getGpio(uint32_t pin);
void setGpio(uint32_t pin, bool value);

#endif
