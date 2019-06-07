#include <stdint.h>
#include <stdbool.h>
#include <setjmp.h>

#include "pxa255_DMA.h"
#include "pxa255_DSP.h"
#include "pxa255_GPIO.h"
#include "pxa255_IC.h"
#include "pxa255_LCD.h"
#include "pxa255_PwrClk.h"
#include "pxa255_RTC.h"
#include "pxa255_TIMR.h"
#include "pxa255_UART.h"
#include "../armv5te/cpu.h"
#include "../armv5te/emu.h"
#include "../armv5te/mem.h"
#include "../armv5te/os/os.h"
#include "../armv5te/translate.h"
#include "../tungstenT3Bus.h"
#include "../emulator.h"


#define PXA255_IO_BASE 0x40000000
#define PXA255_MEMCTRL_BASE 0x48000000

#define PXA255_TIMER_TICKS_PER_FRAME (TUNGSTEN_T3_CPU_CRYSTAL_FREQUENCY / EMU_FPS)


uint16_t*         pxa255Framebuffer;
Pxa255pwrClk      pxa255PwrClk;
static Pxa255ic   pxa255Ic;
static Pxa255lcd  pxa255Lcd;
static Pxa255timr pxa255Timer;
static Pxa255gpio pxa255Gpio;


#include "pxa255Accessors.c.h"

bool pxa255Init(uint8_t** returnRom, uint8_t** returnRam){
   uint32_t mem_offset = 0;
   uint8_t i;

   //enable dynarec if available
   do_translate = true;

   mem_and_flags = os_reserve(MEM_MAXSIZE * 2);
   if(!mem_and_flags)
      return false;

   addr_cache_init();
   memset(mem_areas, 0x00, sizeof(mem_areas));

   //regions
   //ROM
   mem_areas[0].base = PXA255_ROM_START_ADDRESS;
   mem_areas[0].size = TUNGSTEN_T3_ROM_SIZE;
   mem_areas[0].ptr = mem_and_flags + mem_offset;
   mem_offset += TUNGSTEN_T3_ROM_SIZE;

   //RAM
   mem_areas[1].base = PXA255_RAM_START_ADDRESS;
   mem_areas[1].size = TUNGSTEN_T3_RAM_SIZE;
   mem_areas[1].ptr = mem_and_flags + mem_offset;
   mem_offset += TUNGSTEN_T3_RAM_SIZE;

   //memory regions that are not directly mapped to a buffer are not added to mem_areas
   //adding them will cause SIGSEGVs

   //accessors
   //default
   for(i = 0; i < PXA255_TOTAL_MEMORY_BANKS; i++){
       // will fallback to bad_* on non-memory addresses
       read_byte_map[i] = memory_read_byte;
       read_half_map[i] = memory_read_half;
       read_word_map[i] = memory_read_word;
       write_byte_map[i] = memory_write_byte;
       write_half_map[i] = memory_write_half;
       write_word_map[i] = memory_write_word;
   }

   //PCMCIA0
   for(i = PXA255_START_BANK(PXA255_PCMCIA0_START_ADDRESS); i <= PXA255_END_BANK(PXA255_PCMCIA0_START_ADDRESS, PXA255_PCMCIA0_SIZE); i++){
       read_byte_map[i] = pxa255_pcmcia0_read_byte;
       read_half_map[i] = pxa255_pcmcia0_read_half;
       read_word_map[i] = pxa255_pcmcia0_read_word;
       write_byte_map[i] = pxa255_pcmcia0_write_byte;
       write_half_map[i] = pxa255_pcmcia0_write_half;
       write_word_map[i] = pxa255_pcmcia0_write_word;
   }

   //PCMCIA1
   for(i = PXA255_START_BANK(PXA255_PCMCIA1_START_ADDRESS); i <= PXA255_END_BANK(PXA255_PCMCIA1_START_ADDRESS, PXA255_PCMCIA1_SIZE); i++){
       read_byte_map[i] = pxa255_pcmcia1_read_byte;
       read_half_map[i] = pxa255_pcmcia1_read_half;
       read_word_map[i] = pxa255_pcmcia1_read_word;
       write_byte_map[i] = pxa255_pcmcia1_write_byte;
       write_half_map[i] = pxa255_pcmcia1_write_half;
       write_word_map[i] = pxa255_pcmcia1_write_word;
   }

   //IO
   read_byte_map[PXA255_START_BANK(PXA255_IO_BASE)] = pxa255_io_read_byte;
   read_half_map[PXA255_START_BANK(PXA255_IO_BASE)] = bad_read_half;
   read_word_map[PXA255_START_BANK(PXA255_IO_BASE)] = pxa255_io_read_word;
   write_byte_map[PXA255_START_BANK(PXA255_IO_BASE)] = pxa255_io_write_byte;
   write_half_map[PXA255_START_BANK(PXA255_IO_BASE)] = bad_write_half;
   write_word_map[PXA255_START_BANK(PXA255_IO_BASE)] = pxa255_io_write_word;

   //LCD
   read_byte_map[PXA255_START_BANK(PXA255_LCD_BASE)] = bad_read_byte;
   read_half_map[PXA255_START_BANK(PXA255_LCD_BASE)] = bad_read_half;
   read_word_map[PXA255_START_BANK(PXA255_LCD_BASE)] = pxa255_lcd_read_word;
   write_byte_map[PXA255_START_BANK(PXA255_LCD_BASE)] = bad_write_byte;
   write_half_map[PXA255_START_BANK(PXA255_LCD_BASE)] = bad_write_half;
   write_word_map[PXA255_START_BANK(PXA255_LCD_BASE)] = pxa255_lcd_write_word;

   //MEMCTRL
   read_byte_map[PXA255_START_BANK(PXA255_MEMCTRL_BASE)] = bad_read_byte;
   read_half_map[PXA255_START_BANK(PXA255_MEMCTRL_BASE)] = bad_read_half;
   read_word_map[PXA255_START_BANK(PXA255_MEMCTRL_BASE)] = pxa255_memctrl_read_word;
   write_byte_map[PXA255_START_BANK(PXA255_MEMCTRL_BASE)] = bad_write_byte;
   write_half_map[PXA255_START_BANK(PXA255_MEMCTRL_BASE)] = bad_write_half;
   write_word_map[PXA255_START_BANK(PXA255_MEMCTRL_BASE)] = pxa255_memctrl_write_word;

   *returnRom = mem_areas[0].ptr;
   *returnRam = mem_areas[1].ptr;

   return true;
}

void pxa255Deinit(void){
   if(mem_and_flags){
       // translation_table uses absolute addresses
       flush_translations();
       memset(mem_areas, 0, sizeof(mem_areas));
       os_free(mem_and_flags, MEM_MAXSIZE * 2);
       mem_and_flags = NULL;
   }

   addr_cache_deinit();
}

void pxa255Reset(void){
   /*
   static void emu_reset()
   {
       memset(mem_areas[1].ptr, 0, mem_areas[1].size);

       memset(&arm, 0, sizeof arm);
       arm.control = 0x00050078;
       arm.cpsr_low28 = MODE_SVC | 0xC0;
       cpu_events &= EVENT_DEBUG_STEP;

       sched_reset();
       sched.items[SCHED_THROTTLE].clock = CLOCK_27M;
       sched.items[SCHED_THROTTLE].proc = throttle_interval_event;

       memory_reset();
   }
   */

   //set up extra CPU hardware
   pxa255icInit(&pxa255Ic);
   pxa255pwrClkInit(&pxa255PwrClk);
   pxa255lcdInit(&pxa255Lcd, &pxa255Ic);
   pxa255timrInit(&pxa255Timer, &pxa255Ic);
   pxa255gpioInit(&pxa255Gpio, &pxa255Ic);

   memset(&arm, 0, sizeof arm);
   arm.control = 0x00050078;
   arm.cpsr_low28 = MODE_SVC | 0xC0;
   cycle_count_delta = 0;
   cpu_events = 0;
   //cpu_events &= EVENT_DEBUG_STEP;

   //PC starts at 0x00000000, the first opcode for Palm OS 5 is a jump
}

void pxa255SetRtc(uint16_t days, uint8_t hours, uint8_t minutes, uint8_t seconds){
   //TODO: make this do something
}

uint32_t pxa255StateSize(void){
   uint32_t size = 0;

   return size;
}

void pxa255SaveState(uint8_t* data){
   uint32_t offset = 0;

}

void pxa255LoadState(uint8_t* data){
   uint32_t offset = 0;

}

void pxa255Execute(bool wantVideo){
   uint32_t index;
#if OS_HAS_PAGEFAULT_HANDLER
    os_exception_frame_t seh_frame = { NULL, NULL };

    os_faulthandler_arm(&seh_frame);
#endif

   //TODO: need to take the PLL into account still
   cycle_count_delta = -1 * 60 * TUNGSTEN_T3_CPU_CRYSTAL_FREQUENCY / EMU_FPS;

   while(setjmp(restart_after_exception)){};

   exiting = false;//exiting is never set to true, maybe I should remove it?
    while (!exiting && cycle_count_delta < 0) {
         if (cpu_events & (EVENT_FIQ | EVENT_IRQ)) {
             // Align PC in case the interrupt occurred immediately after a jump
             if (arm.cpsr_low28 & 0x20)
                 arm.reg[15] &= ~1;
             else
                 arm.reg[15] &= ~3;

             if (cpu_events & EVENT_WAITING)
                 arm.reg[15] += 4; // Skip over wait instruction

             arm.reg[15] += 4;
             cpu_exception((cpu_events & EVENT_FIQ) ? EX_FIQ : EX_IRQ);
         }
         cpu_events &= ~EVENT_WAITING;//this might need to be move above?

         if (arm.cpsr_low28 & 0x20)
             cpu_thumb_loop();
         else
             cpu_arm_loop();
    }

#if OS_HAS_PAGEFAULT_HANDLER
    os_faulthandler_unarm(&seh_frame);
#endif
    //this needs to run at 3.6864 MHz
    for(index = 0; index < TUNGSTEN_T3_CPU_CRYSTAL_FREQUENCY / EMU_FPS; index++)
      pxa255timrTick(&pxa255Timer);

    //render
    if(likely(wantVideo))
      pxa255lcdFrame(&pxa255Lcd);
}

uint32_t pxa255GetRegister(uint8_t reg){
   return reg_pc(reg);
}
