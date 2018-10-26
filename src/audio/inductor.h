#pragma once

#include <stdint.h>
#include <stdbool.h>

extern int32_t inductorCurrentCharge;
extern int32_t inductorLastAudioSample;

void inductorReset();

void inductorAddClocks(int32_t clocks, bool charge);
void inductorSampleAudio(int32_t now);
