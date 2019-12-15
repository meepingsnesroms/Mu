#include <stdint.h>
#include <stdbool.h>

#include "pxa260.h"
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
#include "uArmGlue.h"
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
Pxa260gpio   pxa260Gpio;
Pxa260timr   pxa260Timer;

static Pxa260lcd  pxa260Lcd;


#include "pxa260Accessors.c.h"

uint8_t read_byte(uint32_t address){
   if(address >= PXA260_ROM_START_ADDRESS && address < PXA260_ROM_START_ADDRESS + TUNGSTEN_T3_ROM_SIZE)
      return palmRom[address - PXA260_ROM_START_ADDRESS];
   else if(address >= PXA260_RAM_START_ADDRESS && address < PXA260_RAM_START_ADDRESS + TUNGSTEN_T3_RAM_SIZE)
      return palmRam[address - PXA260_RAM_START_ADDRESS];
   else if(address >= PXA260_IO_BASE && address < PXA260_IO_BASE + PXA260_BANK_SIZE)
      return pxa260_io_read_byte(address);

   debugLog("Invalid byte read at address: 0x%08X\n", address);
   return 0x00;
}

uint16_t read_half(uint32_t address){
   if(address >= PXA260_ROM_START_ADDRESS && address < PXA260_ROM_START_ADDRESS + TUNGSTEN_T3_ROM_SIZE)
      return *(uint16_t*)(&palmRom[address - PXA260_ROM_START_ADDRESS]);
   else if(address >= PXA260_RAM_START_ADDRESS && address < PXA260_RAM_START_ADDRESS + TUNGSTEN_T3_RAM_SIZE)
      return *(uint16_t*)(&palmRam[address - PXA260_RAM_START_ADDRESS]);
   else if(address >= TUNGSTEN_T3_W86L488_START_ADDRESS && address < TUNGSTEN_T3_W86L488_START_ADDRESS + TUNGSTEN_T3_W86L488_SIZE)
      return w86l488Read16(address);
   else if(address >= PXA260_IO_BASE && address < PXA260_IO_BASE + PXA260_BANK_SIZE)
      return pxa260_io_read_half(address);

   debugLog("Invalid half read at address: 0x%08X\n", address);
   return 0x0000;
}

uint32_t read_word(uint32_t address){
   if(address >= PXA260_ROM_START_ADDRESS && address < PXA260_ROM_START_ADDRESS + TUNGSTEN_T3_ROM_SIZE)
      return *(uint32_t*)(&palmRom[address - PXA260_ROM_START_ADDRESS]);
   else if(address >= PXA260_RAM_START_ADDRESS && address < PXA260_RAM_START_ADDRESS + TUNGSTEN_T3_RAM_SIZE)
      return *(uint32_t*)(&palmRam[address - PXA260_RAM_START_ADDRESS]);
   else if(address >= PXA260_MEMCTRL_BASE && address < PXA260_MEMCTRL_BASE + PXA260_BANK_SIZE)
      return pxa260MemctrlReadWord(address);
   else if(address >= PXA260_LCD_BASE && address < PXA260_LCD_BASE + PXA260_BANK_SIZE)
      return pxa260_lcd_read_word(address);
   else if(address >= PXA260_IO_BASE && address < PXA260_IO_BASE + PXA260_BANK_SIZE)
      return pxa260_io_read_word(address);

   debugLog("Invalid word read at address: 0x%08X\n", address);
   return 0x00000000;
}

void write_byte(uint32_t address, uint8_t byte){
   if(address >= PXA260_RAM_START_ADDRESS && address < PXA260_RAM_START_ADDRESS + TUNGSTEN_T3_RAM_SIZE)
      palmRam[address - PXA260_RAM_START_ADDRESS] = byte;
   else if(address >= PXA260_IO_BASE && address < PXA260_IO_BASE + PXA260_BANK_SIZE)
      pxa260_io_write_byte(address, byte);
   else
      debugLog("Invalid byte write at address: 0x%08X, value:0x%02X\n", address, byte);
}

void write_half(uint32_t address, uint16_t half){
   if(address >= PXA260_RAM_START_ADDRESS && address < PXA260_RAM_START_ADDRESS + TUNGSTEN_T3_RAM_SIZE)
      *(uint16_t*)(&palmRam[address - PXA260_RAM_START_ADDRESS]) = half;
   else if(address >= TUNGSTEN_T3_W86L488_START_ADDRESS && address < TUNGSTEN_T3_W86L488_START_ADDRESS + TUNGSTEN_T3_W86L488_SIZE)
      w86l488Write16(address, half);
   else if(address >= PXA260_IO_BASE && address < PXA260_IO_BASE + PXA260_BANK_SIZE)
      pxa260_io_write_half(address, half);
   else
      debugLog("Invalid half write at address: 0x%08X, value:0x%04X\n", address, half);
}

void write_word(uint32_t address, uint32_t word){
   if(address >= PXA260_RAM_START_ADDRESS && address < PXA260_RAM_START_ADDRESS + TUNGSTEN_T3_RAM_SIZE)
      *(uint32_t*)(&palmRam[address - PXA260_RAM_START_ADDRESS]) = word;
   else if(address >= PXA260_MEMCTRL_BASE && address < PXA260_MEMCTRL_BASE + PXA260_BANK_SIZE)
      pxa260MemctrlWriteWord(address, word);
   else if(address >= PXA260_LCD_BASE && address < PXA260_LCD_BASE + PXA260_BANK_SIZE)
      pxa260_lcd_write_word(address, word);
   else if(address >= PXA260_IO_BASE && address < PXA260_IO_BASE + PXA260_BANK_SIZE)
      pxa260_io_write_word(address, word);
   else
      debugLog("Invalid word write at address: 0x%08X, value:0x%08X\n", address, word);
}

void pxa260Reset(void){
   //set up extra CPU hardware
   pxa260icInit(&pxa260Ic);
   pxa260pwrClkInit(&pxa260PwrClk);
   pxa260lcdInit(&pxa260Lcd, &pxa260Ic);
   pxa260timrInit(&pxa260Timer, &pxa260Ic);
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
   uArmInitCpXX(&pxa260CpuState);
   //PC starts at 0x00000000, the first opcode for Palm OS 5 is a jump
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

   pxa260TimingRun(TUNGSTEN_T3_CPU_PLL_FREQUENCY / EMU_FPS);

   //render
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
