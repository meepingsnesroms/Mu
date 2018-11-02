#include <stdint.h>
#include <stdbool.h>

#include "../portability.h"
#include "../emulator.h"
#include "blip_buf.h"


#define INDUCTOR_CLOCK_POWER 0.0001//the amount 1 clock of true or false will change the inductors total value
#define INDUCTOR_SPEAKER_RANGE 0x6666//prevent hitting the top or bottom of the speaker when switching direction rapidly

double inductorCurrentCharge;
double inductorChargeAtLastSample;


void inductorReset(){
   inductorCurrentCharge = 0.0;
   inductorChargeAtLastSample = 0.0;
}

void inductorPwmDutyCycle(int32_t now, int32_t clocks, double dutyCycle){
   int32_t onClocks = clocks * dutyCycle;
   int32_t offClocks = clocks * (1.00 - dutyCycle);
   double cutoffPoint = dutyCycle - 0.50;//cant go past the actual duty cycle percentage

#if !defined(EMU_NO_SAFETY)
   if(now + clocks >= AUDIO_CLOCK_RATE)
      return;
#endif

   inductorCurrentCharge = dMin(inductorCurrentCharge + onClocks * INDUCTOR_CLOCK_POWER, cutoffPoint);
   blip_add_delta(palmAudioResampler, now, (inductorCurrentCharge - inductorChargeAtLastSample) * INDUCTOR_SPEAKER_RANGE);
   inductorChargeAtLastSample = inductorCurrentCharge;

   inductorCurrentCharge = dMax(-1.0, inductorCurrentCharge - offClocks * INDUCTOR_CLOCK_POWER);
   blip_add_delta(palmAudioResampler, now + onClocks, (inductorCurrentCharge - inductorChargeAtLastSample) * INDUCTOR_SPEAKER_RANGE);
   inductorChargeAtLastSample = inductorCurrentCharge;
}

void inductorPwmOff(int32_t now, int32_t clocks){
   //drift towards 0
   if(inductorCurrentCharge > 0.0)
      inductorCurrentCharge = dMax(0.0, inductorCurrentCharge - clocks * INDUCTOR_CLOCK_POWER);
   else
      inductorCurrentCharge = dMin(inductorCurrentCharge + clocks * INDUCTOR_CLOCK_POWER, 0.0);

   blip_add_delta(palmAudioResampler, now, (inductorCurrentCharge - inductorChargeAtLastSample) * INDUCTOR_SPEAKER_RANGE);
   inductorChargeAtLastSample = inductorCurrentCharge;
}
