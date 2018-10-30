#include <stdint.h>
#include <stdbool.h>

#include "../portability.h"
#include "../emulator.h"
#include "blip_buf.h"


#define INDUCTOR_CLOCK_POWER 0.000001//the amount 1 clock of true or false will change the inductors total value


double inductorCurrentCharge;
double inductorChargeAtLastSample;


void inductorReset(){
   inductorCurrentCharge = 0.0;
   inductorChargeAtLastSample = 0.0;
}

void inductorAddClocks(int32_t clocks, bool charge){
   inductorCurrentCharge = dClamp(0.0, inductorCurrentCharge + (charge ? +clocks : -clocks) * INDUCTOR_CLOCK_POWER, 1.0);
}

void inductorSampleAudio(int32_t now){
   blip_add_delta_fast(palmAudioResampler, now, (inductorCurrentCharge - inductorChargeAtLastSample) * AUDIO_VOLUME);
   inductorChargeAtLastSample = inductorCurrentCharge;
}
