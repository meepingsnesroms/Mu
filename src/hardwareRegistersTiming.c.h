//both timer functions can call eachother define them here
static void timer1(uint8_t reason, double sysclk);
static void timer2(uint8_t reason, double sysclk);

static void timer1(uint8_t reason, double sysclks){
   uint16_t timer1Control = registerArrayRead16(TCTL1);
   uint16_t timer1Compare = registerArrayRead16(TCMP1);
   double timer1OldCount = timerCycleCounter[0];
   double timer1Prescaler = (registerArrayRead16(TPRER1) & 0x00FF) + 1;
   bool timer1Enabled = timer1Control & 0x0001;

   if(timer1Enabled){
      switch((timer1Control & 0x000E) >> 1){
         case 0x0000://stop counter
            //do nothing
            return;

         case 0x0001://SYSCLK / timer prescaler
            if(reason != TIMER_REASON_SYSCLK)
               return;
            timerCycleCounter[0] += sysclks / timer1Prescaler;
            break;

         case 0x0002://SYSCLK / 16 / timer prescaler
            if(reason != TIMER_REASON_SYSCLK)
               return;
            timerCycleCounter[0] += sysclks / 16.0 / timer1Prescaler;
            break;

         case 0x0003://TIN/TOUT pin / timer prescaler, the other timer can be attached to TIN/TOUT
            if(reason != TIMER_REASON_TIN)
               return;
            timerCycleCounter[0] += 1.0 / timer1Prescaler;
            break;

         default://CLK32 / timer prescaler
            if(reason != TIMER_REASON_CLK32)
               return;
            timerCycleCounter[0] += 1.0 / timer1Prescaler;
            break;
      }

      if(timer1OldCount < timer1Compare && timerCycleCounter[0] >= timer1Compare){
         //the comparison against the old value is to prevent an interrupt on every increment in free running mode
         //the timer is not cycle accurate and may not hit the value in the compare register perfectly so check if it would have during in the emulated time
         uint8_t pcrTinToutConfig = registerArrayRead8(PCR) & 0x03;//TIN/TOUT seems not to be physicaly connected but cascaded timers still need to be supported

         //interrupt enabled
         if(timer1Control & 0x0010)
            setIprIsrBit(INT_TMR1);
         //checkInterrupts() is run when the clock that called this function is finished

         //set timer triggered bit
         registerArrayWrite16(TSTAT1, registerArrayRead16(TSTAT1) | 0x0001);
         timerStatusReadAcknowledge[0] &= 0xFFFE;//lock bit until next read

         //increment other timer if enabled
         if(pcrTinToutConfig == 0x03)
            timer2(TIMER_REASON_TIN, 0);

         //not free running, reset to 0, to prevent loss of ticks after compare event just subtract timerXCompare
         if(!(timer1Control & 0x0100))
            timerCycleCounter[0] -= timer1Compare;
      }

      if(timerCycleCounter[0] > 0xFFFF)
         timerCycleCounter[0] -= 0xFFFF;
      registerArrayWrite16(TCN1, (uint16_t)timerCycleCounter[0]);
   }
}

static void timer2(uint8_t reason, double sysclks){
   uint16_t timer2Control = registerArrayRead16(TCTL2);
   uint16_t timer2Compare = registerArrayRead16(TCMP2);
   double timer2OldCount = timerCycleCounter[1];
   double timer2Prescaler = (registerArrayRead16(TPRER2) & 0x00FF) + 1;
   bool timer2Enabled = timer2Control & 0x0001;

   if(timer2Enabled){
      switch((timer2Control & 0x000E) >> 1){
         case 0x0000://stop counter
            //do nothing
            return;

         case 0x0001://SYSCLK / timer prescaler
            if(reason != TIMER_REASON_SYSCLK)
               return;
            timerCycleCounter[1] += sysclks / timer2Prescaler;
            break;

         case 0x0002://SYSCLK / 16 / timer prescaler
            if(reason != TIMER_REASON_SYSCLK)
               return;
            timerCycleCounter[1] += sysclks / 16.0 / timer2Prescaler;
            break;

         case 0x0003://TIN/TOUT pin / timer prescaler, the other timer can be attached to TIN/TOUT
            if(reason != TIMER_REASON_TIN)
               return;
            timerCycleCounter[1] += 1.0 / timer2Prescaler;
            break;

         default://CLK32 / timer prescaler
            if(reason != TIMER_REASON_CLK32)
               return;
            timerCycleCounter[1] += 1.0 / timer2Prescaler;
            break;
      }

      if(timer2OldCount < timer2Compare && timerCycleCounter[1] >= timer2Compare){
         //the comparison against the old value is to prevent an interrupt on every increment in free running mode
         //the timer is not cycle accurate and may not hit the value in the compare register perfectly so check if it would have during in the emulated time
         uint8_t pcrTinToutConfig = registerArrayRead8(PCR) & 0x03;//TIN/TOUT seems not to be physicaly connected but cascaded timers still need to be supported

         //interrupt enabled
         if(timer2Control & 0x0010)
            setIprIsrBit(INT_TMR2);
         //checkInterrupts() is run when the clock that called this function is finished

         //set timer triggered bit
         registerArrayWrite16(TSTAT2, registerArrayRead16(TSTAT2) | 0x0001);
         timerStatusReadAcknowledge[1] &= 0xFFFE;//lock bit until next read

         //increment other timer if enabled
         if(pcrTinToutConfig == 0x02)
            timer1(TIMER_REASON_TIN, 0);

         //not free running, reset to 0, to prevent loss of ticks after compare event just subtract timerXCompare
         if(!(timer2Control & 0x0100))
            timerCycleCounter[1] -= timer2Compare;
      }

      if(timerCycleCounter[1] > 0xFFFF)
         timerCycleCounter[1] -= 0xFFFF;
      registerArrayWrite16(TCN2, (uint16_t)timerCycleCounter[1]);
   }
}

static double dmaclksPerClk32(){
   uint16_t pllcr = registerArrayRead16(PLLCR);
   uint16_t pllfsr = registerArrayRead16(PLLFSR);
   uint8_t p = pllfsr & 0x00FF;
   uint8_t q = pllfsr >> 8 & 0x000F;
   double dmaclks = 2.0 * (14.0 * (p + 1.0) + q + 1.0);

   //prescaler 1 enabled, divide by 2
   if(pllcr & 0x0080)
      dmaclks /= 2.0;

   //prescaler 2 enabled, divides value from prescaler 1 by 2
   if(pllcr & 0x0020)
      dmaclks /= 2.0;

   return dmaclks;
}

static double sysclksPerClk32(){
   uint8_t sysclkSelect = registerArrayRead16(PLLCR) >> 8 & 0x0007;

   //>= 4 means run at full speed, no divider
   if(sysclkSelect >= 4)
      return dmaclksPerClk32();

   //divide DMACLK by 2 to the power of PLLCR SYSCLKSEL
   return dmaclksPerClk32() / (2 << sysclkSelect);
}

static void rtiInterruptClk32(){
   //this function is part of endClk32();
   uint16_t triggeredRtiInterrupts = 0x0000;

   if(clk32Counter % (CRYSTAL_FREQUENCY / 512) == 0){
      //RIS7 - 512HZ
      triggeredRtiInterrupts |= 0x8000;
   }
   if(clk32Counter % (CRYSTAL_FREQUENCY / 256) == 0){
      //RIS6 - 256HZ
      triggeredRtiInterrupts |= 0x4000;
   }
   if(clk32Counter % (CRYSTAL_FREQUENCY / 128) == 0){
      //RIS5 - 128HZ
      triggeredRtiInterrupts |= 0x2000;
   }
   if(clk32Counter % (CRYSTAL_FREQUENCY / 64) == 0){
      //RIS4 - 64HZ
      triggeredRtiInterrupts |= 0x1000;
   }
   if(clk32Counter % (CRYSTAL_FREQUENCY / 32) == 0){
      //RIS3 - 32HZ
      triggeredRtiInterrupts |= 0x0800;
   }
   if(clk32Counter % (CRYSTAL_FREQUENCY / 16) == 0){
      //RIS2 - 16HZ
      triggeredRtiInterrupts |= 0x0400;
   }
   if(clk32Counter % (CRYSTAL_FREQUENCY / 8) == 0){
      //RIS1 - 8HZ
      triggeredRtiInterrupts |= 0x0200;
   }
   if(clk32Counter % (CRYSTAL_FREQUENCY / 4) == 0){
      //RIS0 - 4HZ
      triggeredRtiInterrupts |= 0x0100;
   }

   triggeredRtiInterrupts &= registerArrayRead16(RTCIENR);
   if(triggeredRtiInterrupts){
      registerArrayWrite16(RTCISR, registerArrayRead16(RTCISR) | triggeredRtiInterrupts);
      setIprIsrBit(INT_RTI);
   }
}

static void watchdogSecondTickClk32(){
   //this function is part of endClk32();
   uint16_t watchdogState = registerArrayRead16(WATCHDOG);

   if(watchdogState & 0x0001){
      //watchdog enabled
      watchdogState += 0x0100;//add second to watchdog timer
      watchdogState &= 0x0383;//cap overflow
      if((watchdogState & 0x0200) == 0x0200){
         //time expired
         if(watchdogState & 0x0002){
            //interrupt
            watchdogState |= 0x0080;
            setIprIsrBit(INT_WDT);
         }
         else{
            //reset
            emulatorReset();
            return;
         }
      }
      registerArrayWrite16(WATCHDOG, watchdogState);
   }
}

static void rtcAddSecondClk32(){
   //this function is part of endClk32();
   if(registerArrayRead16(RTCCTL) & 0x0080){
      //RTC enable bit set
      uint16_t rtcInterruptEvents = 0x0000;
      uint32_t newRtcTime;
      uint32_t oldRtcTime = registerArrayRead32(RTCTIME);
      uint32_t rtcAlrm = registerArrayRead32(RTCALRM);
      uint16_t dayAlrm = registerArrayRead16(DAYALRM);
      uint16_t days = registerArrayRead16(DAYR);
      uint8_t hours = oldRtcTime >> 24;
      uint8_t minutes = oldRtcTime >> 16 & 0x0000003F;
      uint8_t seconds = oldRtcTime & 0x0000003F;

      seconds++;
      rtcInterruptEvents |= 0x0010;
      if(seconds >= 60){
         uint16_t stopwatch = registerArrayRead16(STPWCH);

         if(stopwatch != 0x003F){
            if(stopwatch == 0x0000)
               stopwatch = 0x003F;
            else
               stopwatch--;
            registerArrayWrite16(STPWCH, stopwatch);
         }

         //if stopwatch ran out above or was enabled with 0x003F in the register trigger interrupt
         if(stopwatch == 0x003F)
            rtcInterruptEvents |= 0x0001;

         minutes++;
         seconds = 0;
         rtcInterruptEvents |= 0x0002;
         if(minutes >= 60){
            hours++;
            minutes = 0;
            rtcInterruptEvents |= 0x0020;
            if(hours >= 24){
               hours = 0;
               days++;
               rtcInterruptEvents |= 0x0008;
            }
         }
      }

      newRtcTime = seconds;
      newRtcTime |= minutes << 16;
      newRtcTime |= hours << 24;

      if(newRtcTime == rtcAlrm && days == dayAlrm)
         rtcInterruptEvents |= 0x0040;

      rtcInterruptEvents &= registerArrayRead16(RTCIENR);
      if(rtcInterruptEvents){
         registerArrayWrite16(RTCISR, registerArrayRead16(RTCISR) | rtcInterruptEvents);
         setIprIsrBit(INT_RTC);
      }

      registerArrayWrite32(RTCTIME, newRtcTime);
      registerArrayWrite16(DAYR, days & 0x01FF);
   }

   watchdogSecondTickClk32();
}

void beginClk32(){
   palmClk32Sysclks = 0.0;
}

void endClk32(){
   registerArrayWrite16(PLLFSR, registerArrayRead16(PLLFSR) ^ 0x8000);

   //second position counter
   if(clk32Counter >= CRYSTAL_FREQUENCY - 1){
      clk32Counter = 0;
      rtcAddSecondClk32();
   }
   else{
      clk32Counter++;
   }

   //disabled if both the watchdog timer AND the RTC timer are disabled
   if(registerArrayRead16(RTCCTL) & 0x0080 || registerArrayRead16(WATCHDOG) & 0x01)
      rtiInterruptClk32();

   timer1(TIMER_REASON_CLK32, 0);
   timer2(TIMER_REASON_CLK32, 0);
   samplePwm1(true/*forClk32*/, 0.0);

   //PLLCR sleep wait
   if(pllSleepWait != -1){
      if(pllSleepWait == 0){
         //disable PLL and CPU
         palmSysclksPerClk32 = 0.0;
         debugLog("PLL disabled, CPU is off!\n");
      }
      pllSleepWait--;
   }

   //PLLCR wake select wait
   if(pllWakeWait != -1){
      if(pllWakeWait == 0){
         //reenable PLL and CPU
         registerArrayWrite16(PLLCR, registerArrayRead16(PLLCR) & 0xFFF7);
         palmSysclksPerClk32 = sysclksPerClk32();
         debugLog("PLL reenabled, CPU is on!\n");
      }
      pllWakeWait--;
   }

   checkInterrupts();
}

void addSysclks(double count){
   timer1(TIMER_REASON_SYSCLK, count);
   timer2(TIMER_REASON_SYSCLK, count);
   samplePwm1(false/*forClk32*/, count);

   checkInterrupts();
   palmClk32Sysclks += count;
}

static int32_t audioGetFramePercentIncrementFromClk32s(int32_t count){
   return (double)count / ((double)CRYSTAL_FREQUENCY / EMU_FPS) * AUDIO_END_OF_FRAME;
}

static int32_t audioGetFramePercentIncrementFromSysclks(double count){
   return count / (palmSysclksPerClk32 * ((double)CRYSTAL_FREQUENCY / EMU_FPS)) * AUDIO_END_OF_FRAME;
}

static int32_t audioGetFramePercentage(){
   //returns how much of the frame has executed
   //0% = 0, 100% = AUDIO_END_OF_FRAME
   return audioGetFramePercentIncrementFromClk32s(palmFrameClk32s) + audioGetFramePercentIncrementFromSysclks(palmClk32Sysclks);
}
