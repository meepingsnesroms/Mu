#include <stdint.h>
#include <stdbool.h>

#include "pxa260.h"
#include "pxa260_MMU.h"
#include "pxa260_cp15.h"
#include "pxa260_DMA.h"
#include "pxa260_DSP.h"
#include "pxa260_GPIO.h"
#include "pxa260_IC.h"
#include "pxa260_LCD.h"
#include "pxa260_PwrClk.h"
#include "pxa260_RTC.h"
#include "pxa260_TIMR.h"
#include "pxa260_UART.h"
#include "pxa260I2c.h"
#include "pxa260Memctrl.h"
#include "pxa260Ssp.h"
#include "pxa260Udc.h"
#include "pxa260Timing.h"
#include "../tungstenT3Bus.h"
#include "../tsc2101.h"
#include "../tps65010.h"
#include "../emulator.h"
#include "../portability.h"


#define PXA260_TIMER_TICKS_PER_FRAME (TUNGSTEN_T3_CPU_CRYSTAL_FREQUENCY / EMU_FPS)


ArmCpu       pxa260CpuState;
uint16_t*    pxa260Framebuffer;
Pxa260pwrClk pxa260PwrClk;
Pxa260ic     pxa260Ic;
Pxa260rtc    pxa260Rtc;
Pxa260gpio   pxa260Gpio;
Pxa260timr   pxa260Timer;

static Pxa260lcd  pxa260Lcd;
static ArmMmu uArmMmu;
static ArmCoprocessor uArmCp14;
static ArmCP15 uArmCp15;


#include "pxa260Accessors.c.h"

void pxa260Reset(void){
   //set up extra CPU hardware
   pxa260icInit(&pxa260Ic);
   pxa260pwrClkInit(&pxa260PwrClk);
   pxa260lcdInit(&pxa260Lcd, &pxa260Ic);
   pxa260timrInit(&pxa260Timer, &pxa260Ic);
   pxa260rtcInit(&pxa260Rtc, &pxa260Ic);
   pxa260gpioInit(&pxa260Gpio, &pxa260Ic);
   pxa260TimingInit();
   pxa260I2cReset();
   pxa260MemctrlReset();
   pxa260SspReset();
   pxa260UdcReset();
   pxa260TimingReset();

   //set first timer event
   pxa260TimingTriggerEvent(PXA260_TIMING_CALLBACK_TICK_CPU_TIMER, TUNGSTEN_T3_CPU_PLL_FREQUENCY / TUNGSTEN_T3_CPU_CRYSTAL_FREQUENCY);


   cpuInit(&pxa260CpuState, 0x00000000, uArmMemAccess, uArmEmulErr, uArmHypercall, uArmSetFaultAddr);
   //PC starts at 0x00000000, the first opcode for Palm OS 5 is a jump
   uArmCp14.regXfer = pxa260pwrClkPrvCoprocRegXferFunc;
   uArmCp14.dataProcessing = NULL;
   uArmCp14.memAccess = NULL;
   uArmCp14.twoRegF = NULL;
   uArmCp14.userData = &pxa260PwrClk;

   //pwrclk already inited in pxa260Reset
   cpuCoprocessorRegister(&pxa260CpuState, 14, &uArmCp14);

   mmuInit(&uArmMmu, mmuReadF, NULL);
   cp15Init(&uArmCp15, &pxa260CpuState, &uArmMmu);
}

void pxa260SetRtc(uint16_t days, uint8_t hours, uint8_t minutes, uint8_t seconds){
   //TODO: make this do something
}

uint32_t pxa260StateSize(void){
   uint32_t size = 0;

   return size;
}

void pxa260SaveState(uint8_t* data){
   uint32_t offset = 0;

}

void pxa260LoadState(uint8_t* data){
   uint32_t offset = 0;

}

void pxa260Execute(bool wantVideo){
   tsc2101UpdateInterrupt();
   tps65010UpdateInterrupt();
   pxa260gpioUpdateKeyMatrix(&pxa260Gpio);
   pxa260rtcUpdate(&pxa260Rtc);

   //start render
   if(likely(wantVideo))
      pxa260lcdFrame(&pxa260Lcd);

   pxa260TimingRun(TUNGSTEN_T3_CPU_PLL_FREQUENCY / EMU_FPS);

   //end render
   if(likely(wantVideo))
      pxa260lcdFrame(&pxa260Lcd);
}

uint32_t pxa260GetRegister(uint8_t reg){
   return cpuGetRegExternal(&pxa260CpuState, reg);
}

uint32_t pxa260GetCpsr(void){
   return cpuGetRegExternal(&pxa260CpuState, ARM_REG_NUM_CPSR);
}

uint32_t pxa260GetSpsr(void){
   return cpuGetRegExternal(&pxa260CpuState, ARM_REG_NUM_SPSR);
}

uint64_t pxa260ReadArbitraryMemory(uint32_t address, uint8_t size){
   uint64_t data = UINT64_MAX;//invalid access
   uint32_t value;
   uint8_t unused;

   if(uArmMemAccess(&pxa260CpuState, &value, address, size / 8, false, true, &unused)){
      switch(size){
         case 8:
            data = *(uint8_t*)&value;
            break;

         case 16:
            data = *(uint16_t*)&value;
            break;

         case 32:
            data = *(uint32_t*)&value;
            break;
      }
   }

   return data;
}
