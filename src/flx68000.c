#include <stdint.h>
#include <stdbool.h>

#include "emulator.h"
#include "portability.h"
#include "hardwareRegisters.h"
#include "memoryAccess.h"

#if defined(EMU_OPTIMIZE_FOR_ARM)
#include "m68k/cyclone/Cyclone.h"
#else
#include "m68k/musashi/m68kcpu.h"
#endif


#if defined(EMU_OPTIMIZE_FOR_ARM)
static struct Cyclone cycloneCpu;
#endif


void flx68000Init(){
   static bool inited = false;

   if(!inited){
#if defined(EMU_OPTIMIZE_FOR_ARM)
      CycloneInit();
#else
      m68k_init();
      m68k_set_cpu_type(M68K_CPU_TYPE_68000);

      CPU_ADDRESS_MASK = 0xFFFFFFFF;

      m68k_set_reset_instr_callback(emulatorReset);
      m68k_set_int_ack_callback(interruptAcknowledge);
#endif
      inited = true;
   }
}

void flx68000Reset(){
   resetHwRegisters();
   resetAddressSpace();//address space must be reset after hardware registers because it is dependent on them
#if defined(EMU_OPTIMIZE_FOR_ARM)
   CycloneReset(&cycloneCpu);
#else
   m68k_pulse_reset();
#endif
}

uint64_t flx68000StateSize(){
   uint64_t size = 0;

#if defined(EMU_OPTIMIZE_FOR_ARM)
   size += 0x80;//specified in Cyclone.h, line 82
#else
   size += m68k_context_size();
#endif

   return size;
}

void flx68000SaveState(uint8_t* data){
   uint64_t offset = 0;
#if defined(EMU_OPTIMIZE_FOR_ARM)
   CyclonePack(&cycloneCpu, data + offset);
   offset += 0x80;//specified in Cyclone.h, line 82
#else
   m68k_get_context(data + offset);
   offset += m68k_context_size();
#endif
}

void flx68000LoadState(uint8_t* data){
   uint64_t offset = 0;

#if defined(EMU_OPTIMIZE_FOR_ARM)
   CycloneUnpack(&cycloneCpu, data + offset);
   offset += 0x80;//specified in Cyclone.h, line 82
#else
   m68k_set_context(data + offset);
   offset += m68k_context_size();
#endif
}

void flx68000Execute(){
   double cyclesRemaining = palmSysclksPerClk32;

   beginClk32();

   while(cyclesRemaining >= 1.0){
      double sysclks = dMin(cyclesRemaining, EMU_SYSCLK_PRECISION);
      int32_t cpuCycles = sysclks * pctlrCpuClockDivider * palmClockMultiplier;

      if(cpuCycles > 0){
#if defined(EMU_OPTIMIZE_FOR_ARM)
         cycloneCpu.cycles = cpuCycles;
         CycloneRun(&cycloneCpu);
#else
         m68k_execute(cpuCycles);
#endif
      }
      addSysclks(sysclks);

      cyclesRemaining -= sysclks;
   }

   endClk32();
}

void flx68000SetIrq(uint8_t irqLevel){
#if defined(EMU_OPTIMIZE_FOR_ARM)
   cycloneCpu.irq = irqLevel;
   CycloneFlushIrq(&cycloneCpu);
#else
   m68k_set_irq(irqLevel);
#endif
}

bool flx68000IsSupervisor(){
#if defined(EMU_OPTIMIZE_FOR_ARM)
   return CycloneGetSr(&cycloneCpu) & 0x2000;
#else
   return m68k_get_reg(NULL, M68K_REG_SR) & 0x2000;
#endif
}

void flx68000BusError(uint32_t address, bool isWrite){
#if defined(EMU_OPTIMIZE_FOR_ARM)
   //no bus error yet
#else
   //never call outsize of a 68k opcode, behavior is undefined due to longjmp
   m68ki_trigger_bus_error(address, isWrite ? MODE_WRITE : MODE_READ, FLAG_S | m68ki_get_address_space());
#endif
}

uint32_t flx68000GetRegister(uint8_t reg){
   //register standard, applys to all CPU cores
   /*
   M68K_REG_D0 = 0,
   M68K_REG_D1,
   M68K_REG_D2,
   M68K_REG_D3,
   M68K_REG_D4,
   M68K_REG_D5,
   M68K_REG_D6,
   M68K_REG_D7,
   M68K_REG_A0,
   M68K_REG_A1,
   M68K_REG_A2,
   M68K_REG_A3,
   M68K_REG_A4,
   M68K_REG_A5,
   M68K_REG_A6,
   M68K_REG_A7,
   M68K_REG_PC,
   M68K_REG_SR
   */

#if defined(EMU_OPTIMIZE_FOR_ARM)
   //debug not supported on embedded devices yet
#else
   return m68k_get_reg(NULL, reg);
#endif
}

uint32_t flx68000GetPc(){
#if defined(EMU_OPTIMIZE_FOR_ARM)
   //debug not supported on embedded devices yet
#else
   return m68k_get_reg(NULL, M68K_REG_PPC);
#endif
}

uint64_t flx68000ReadArbitraryMemory(uint32_t address, uint8_t size){
   uint64_t data = UINT64_MAX;//invalid access

#if defined(EMU_OPTIMIZE_FOR_ARM)
   //debug not supported on embedded devices yet
#else
   //until SPI and UART destructive reads are implemented all reads to mapped addresses are safe, SPI is now implemented, this needs to be fixed
   if(bankType[START_BANK(address)] != CHIP_NONE){
      uint16_t m68kSr = m68k_get_reg(NULL, M68K_REG_SR);
      m68k_set_reg(M68K_REG_SR, 0x2000);//prevent privilege violations
      switch(size){
         case 8:
            data = m68k_read_memory_8(address);
            break;

         case 16:
            data = m68k_read_memory_16(address);
            break;

         case 32:
            data = m68k_read_memory_32(address);
            break;
      }
      m68k_set_reg(M68K_REG_SR, m68kSr);
   }
#endif

   return data;
}
