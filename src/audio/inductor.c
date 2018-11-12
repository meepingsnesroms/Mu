#include <stdint.h>

#include "../portability.h"
#include "../emulator.h"
#include "blip_buf.h"


#define INDUCTOR_CLOCK_POWER 0.0001//the amount 1 clock of true or false will change the inductors total value
#define INDUCTOR_SPEAKER_RANGE 0x6000//prevent hitting the top or bottom of the speaker when switching direction rapidly


void inductorPwmDutyCycle(int32_t now, int32_t clocks, float dutyCycle){
   int32_t onClocks = clocks * dutyCycle;
   int32_t offClocks = clocks * (1.00 - dutyCycle);
   float inductorCurrentCharge;
   float inductorChargeAtLastSample;

#if !defined(EMU_NO_SAFETY)
   if(now + clocks >= AUDIO_CLOCK_RATE)
      return;
#endif

   inductorCurrentCharge = fMin(onClocks * INDUCTOR_CLOCK_POWER, 1.0);
   blip_add_delta(palmAudioResampler, now, inductorCurrentCharge * INDUCTOR_SPEAKER_RANGE);
   inductorChargeAtLastSample = inductorCurrentCharge;

   inductorCurrentCharge = fMax(-1.0, inductorCurrentCharge - offClocks * INDUCTOR_CLOCK_POWER);
   blip_add_delta(palmAudioResampler, now + onClocks, (inductorCurrentCharge - inductorChargeAtLastSample) * INDUCTOR_SPEAKER_RANGE);
}
