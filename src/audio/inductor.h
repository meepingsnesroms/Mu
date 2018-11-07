#ifndef INDUCTOR_H
#define INDUCTOR_H

#include <stdint.h>

extern float inductorCurrentCharge;
extern float inductorChargeAtLastSample;

void inductorReset();

void inductorPwmDutyCycle(int32_t now, int32_t clocks, float dutyCycle);
void inductorPwmOff(int32_t now, int32_t clocks);

#endif
