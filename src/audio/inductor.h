#pragma once

#include <stdint.h>
#include <stdbool.h>

extern double inductorCurrentCharge;
extern double inductorChargeAtLastSample;

void inductorReset();

void inductorPwmOff(int32_t clocks);
void inductorAddClocks(int32_t clocks, bool charge);
void inductorSampleAudio(int32_t now);
