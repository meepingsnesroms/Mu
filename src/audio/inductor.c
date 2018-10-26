#include <stdint.h>
#include <stdbool.h>

#include "../portability.h"
#include "../emulator.h"
#include "blip_buf.h"


#define INDUCTOR_RANGE 10000
#define INDUCTOR_CLOCK_POWER 0.80


int32_t inductorCurrentCharge;
int32_t inductorLastAudioSample;


void inductorReset(){
   inductorCurrentCharge = 0;
   inductorLastAudioSample = 0;
}

void inductorAddClocks(int32_t clocks, bool charge){
   inductorCurrentCharge = sClamp(-INDUCTOR_RANGE, inductorCurrentCharge + (charge ? +clocks : -clocks) * INDUCTOR_CLOCK_POWER, INDUCTOR_RANGE);
}

void inductorSampleAudio(int32_t now){
   blip_add_delta(palmAudioResampler, now, inductorCurrentCharge - inductorLastAudioSample);
   inductorLastAudioSample = inductorCurrentCharge;
}
