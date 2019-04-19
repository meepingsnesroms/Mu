#include <stdint.h>
#include <stdbool.h>

#include "emulator.h"
#include "portability.h"
#include "dbvzRegisters.h"
#include "m515Bus.h"
#include "m68k/m68kcpu.h"


//memory speed hack, used by cyclone, cyclone always crashed so I decided to just port over one of its biggest speed ups and only use musashi
#if M68K_SEPARATE_READS
static uintptr_t memBase;


void flx68000PcLongJump(uint32_t newPc){
   uintptr_t dataBufferHost;
   uint32_t dataBufferGuest;
   uint32_t windowSize;

   switch(dbvzBankType[DBVZ_START_BANK(newPc)]){
      case DBVZ_CHIP_A0_ROM:
         dataBufferHost = (uintptr_t)palmRom;
         dataBufferGuest = chips[DBVZ_CHIP_A0_ROM].start;
         windowSize = chips[DBVZ_CHIP_A0_ROM].mask + 1;
         break;

      case DBVZ_CHIP_DX_RAM:
         dataBufferHost = (uintptr_t)palmRam;
         dataBufferGuest = chips[DBVZ_CHIP_DX_RAM].start;
         windowSize = chips[DBVZ_CHIP_DX_RAM].mask + 1;
         break;

      case DBVZ_CHIP_REGISTERS:
         //needed for when EMU_NO_SAFETY is set and a function is run in the sandbox
         dataBufferHost = (uintptr_t)palmReg;
         dataBufferGuest = DBVZ_BANK_ADDRESS(DBVZ_START_BANK(newPc));
         windowSize = DBVZ_REG_SIZE;
         break;
   }

   memBase = dataBufferHost - dataBufferGuest - windowSize * ((newPc - dataBufferGuest) / windowSize);
}

//everything must be 16 bit aligned(accept 8 bit accesses) due to 68k unaligned access rules,
//32 bit reads are 2 16 bit reads because on some platforms 32 bit reads that arnt on 32 bit boundrys will crash the program
#if defined(EMU_BIG_ENDIAN)
uint16_t m68k_read_immediate_16(uint32_t address){
   return *(uint16_t*)(memBase + address);
}
uint32_t m68k_read_immediate_32(uint32_t address){
   return *(uint16_t*)(memBase + address) << 16 | *(uint16_t*)(memBase + address + 2);
}
uint8_t  m68k_read_pcrelative_8(uint32_t address){
   return *(uint8_t*)(memBase + address);
}
uint16_t  m68k_read_pcrelative_16(uint32_t address){
   return *(uint16_t*)(memBase + address);
}
uint32_t  m68k_read_pcrelative_32(uint32_t address){
   return *(uint16_t*)(memBase + address) << 16 | *(uint16_t*)(memBase + address + 2);
}
#else
uint16_t m68k_read_immediate_16(uint32_t address){
   return *(uint16_t*)(memBase + address);
}
uint32_t m68k_read_immediate_32(uint32_t address){
   return *(uint16_t*)(memBase + address) << 16 | *(uint16_t*)(memBase + address + 2);
}
uint8_t  m68k_read_pcrelative_8(uint32_t address){
   return *(uint8_t*)(memBase + (address ^ 1));
}
uint16_t  m68k_read_pcrelative_16(uint32_t address){
   return *(uint16_t*)(memBase + address);
}
uint32_t  m68k_read_pcrelative_32(uint32_t address){
   return *(uint16_t*)(memBase + address) << 16 | *(uint16_t*)(memBase + address + 2);
}
#endif
#endif

void flx68000Init(void){
   static bool inited = false;

   if(!inited){
      m68k_init();
      m68k_set_cpu_type(M68K_CPU_TYPE_68000);

      CPU_ADDRESS_MASK = 0xFFFFFFFF;

      inited = true;
   }
}

void flx68000Reset(void){
   dbvzResetRegisters();
   dbvzResetAddressSpace();//address space must be reset after hardware registers because it is dependent on them
   m68k_pulse_reset();
}

uint32_t flx68000StateSize(void){
   uint32_t size = 0;

   size += sizeof(uint32_t) * 50;//m68ki_cpu

   return size;
}

void flx68000SaveState(uint8_t* data){
   uint32_t offset = 0;
   uint8_t index;

   for(index = 0; index < 16; index++){
      writeStateValue32(data + offset, m68ki_cpu.dar[index]);
      offset += sizeof(uint32_t);
   }
   writeStateValue32(data + offset, m68ki_cpu.ppc);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.pc);
   offset += sizeof(uint32_t);
   for(index = 0; index < 7; index++){
      writeStateValue32(data + offset, m68ki_cpu.sp[index]);
      offset += sizeof(uint32_t);
   }
   writeStateValue32(data + offset, m68ki_cpu.vbr);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.sfc);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.dfc);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.cacr);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.caar);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.ir);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.t1_flag);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.t0_flag);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.s_flag);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.m_flag);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.x_flag);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.n_flag);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.not_z_flag);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.v_flag);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.c_flag);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.int_mask);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.int_level);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.int_cycles);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.stopped);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.pref_addr);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.pref_data);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.address_mask);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.sr_mask);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.instr_mode);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, m68ki_cpu.run_mode);
   offset += sizeof(uint32_t);
}

void flx68000LoadState(uint8_t* data){
   uint32_t offset = 0;
   uint8_t index;

   for(index = 0; index < 16; index++){
      m68ki_cpu.dar[index] = readStateValue32(data + offset);
      offset += sizeof(uint32_t);
   }
   m68ki_cpu.ppc = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.pc = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   for(index = 0; index < 7; index++){
      m68ki_cpu.sp[index] = readStateValue32(data + offset);
      offset += sizeof(uint32_t);
   }
   m68ki_cpu.vbr = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.sfc = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.dfc = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.cacr = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.caar = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.ir = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.t1_flag = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.t0_flag = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.s_flag = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.m_flag = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.x_flag = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.n_flag = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.not_z_flag = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.v_flag = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.c_flag = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.int_mask = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.int_level = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.int_cycles = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.stopped = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.pref_addr = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.pref_data = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.address_mask = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.sr_mask = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.instr_mode = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.run_mode = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
}

void flx68000LoadStateFinished(void){
#if M68K_SEPARATE_READS
   //set PC accessor to the PC from the state
   flx68000PcLongJump(m68ki_cpu.pc);
#endif
}

void flx68000Execute(void){
   double cyclesRemaining = palmSysclksPerClk32;

   dbvzBeginClk32();

   while(cyclesRemaining >= 1.0){
      double sysclks = dMin(cyclesRemaining, EMU_SYSCLK_PRECISION);
      int32_t cpuCycles = sysclks * pctlrCpuClockDivider * palmClockMultiplier;

      if(cpuCycles > 0)
         m68k_execute(cpuCycles);
      dbvzAddSysclks(sysclks);

      cyclesRemaining -= sysclks;
   }

   dbvzEndClk32();
}

void flx68000SetIrq(uint8_t irqLevel){
   m68k_set_irq(irqLevel);
}

bool flx68000IsSupervisor(void){
   return !!(m68k_get_reg(NULL, M68K_REG_SR) & 0x2000);
}

void flx68000BusError(uint32_t address, bool isWrite){
#if !defined(EMU_NO_SAFETY)
   if(!(palmEmuFeatures.info & FEATURE_DURABLE)){
      //never call outsize of a 68k opcode, behavior is undefined due to longjmp
      m68ki_trigger_bus_error(address, isWrite ? MODE_WRITE : MODE_READ, FLAG_S | m68ki_get_address_space());
   }
#endif
}

uint32_t flx68000GetRegister(uint8_t reg){
   return m68k_get_reg(NULL, reg);
}

uint32_t flx68000GetPc(void){
   return m68k_get_reg(NULL, M68K_REG_PPC);
}

uint64_t flx68000ReadArbitraryMemory(uint32_t address, uint8_t size){
   uint64_t data = UINT64_MAX;//invalid access

   //reading from a hardware register FIFO will corrupt it!
   if(dbvzBankType[DBVZ_START_BANK(address)] != DBVZ_CHIP_NONE){
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

   return data;
}
