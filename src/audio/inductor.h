#pragma once

#include <stdint.h>

extern double inductorCurrentCharge;
extern double inductorChargeAtLastSample;

void inductorReset();

void inductorPwmDutyCycle(int32_t now, int32_t clocks, double dutyCycle);
void inductorPwmOff(int32_t now, int32_t clocks);
