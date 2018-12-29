#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "emulator.h"
#include "portability.h"
#include "hardwareRegisters.h"
#include "memoryAccess.h"

#if defined(EMU_FAST_CPU)
#if defined(EMU_OPTIMIZE_FOR_ARM32)
#include <stdlib.h>//for exit(1)
#include "m68k/cyclone/Cyclone.h"
#else
#include "m68k/fame/fame.h"
#endif
#else
#include "m68k/musashi/m68kcpu.h"
#endif

#if defined(EMU_FAST_CPU)
#if defined(EMU_OPTIMIZE_FOR_ARM32)
static struct Cyclone cycloneCpu;


extern unsigned int m68k_read_memory_8(unsigned int address);
extern unsigned int m68k_read_memory_16(unsigned int address);
extern unsigned int m68k_read_memory_32(unsigned int address);
extern void m68k_write_memory_8(unsigned int address, unsigned char value);
extern void m68k_write_memory_16(unsigned int address, unsigned short value);
extern void m68k_write_memory_32(unsigned int address, unsigned int value);

unsigned int flx68000CheckPc(unsigned int pc){
   unsigned int dataBufferHost;
   unsigned int dataBufferGuest;
   unsigned int windowSize;

   pc -= cycloneCpu.membase;//get the real program counter

   if(chips[CHIP_A0_ROM].inBootMode || pc >= chips[CHIP_A0_ROM].start && pc < chips[CHIP_A0_ROM].start + chips[CHIP_A0_ROM].lineSize){
      dataBufferHost = (unsigned int)palmRom;
      dataBufferGuest = chips[CHIP_A0_ROM].start;
      windowSize = chips[CHIP_A0_ROM].mask + 1;
   }
   else if(pc >= chips[CHIP_DX_RAM].start && pc < chips[CHIP_DX_RAM].start + chips[CHIP_DX_RAM].lineSize){
      dataBufferHost = (unsigned int)palmRam;
      dataBufferGuest = chips[CHIP_DX_RAM].start;
      windowSize = chips[CHIP_DX_RAM].mask + 1;
   }
   else{
      //executing from anywhere else is not supported, just crash to prevent corrupting the user RAM file with invalid accesses
      exit(1);
   }

   cycloneCpu.membase = dataBufferHost - dataBufferGuest - windowSize * ((pc - dataBufferGuest) / windowSize);

   return cycloneCpu.membase + pc;//new program counter
}
#else
static M68K_CONTEXT fameCpu;


extern unsigned int m68k_read_memory_8(unsigned int address);
extern unsigned int m68k_read_memory_16(unsigned int address);
extern unsigned int m68k_read_memory_32(unsigned int address);
extern void m68k_write_memory_8(unsigned int address, unsigned char value);
extern void m68k_write_memory_16(unsigned int address, unsigned short value);
extern void m68k_write_memory_32(unsigned int address, unsigned int value);

void flx68000InterruptAcknowledge(unsigned level){
   int vector = interruptAcknowledge(level);

   fameCpu.interrupts[0] = level;
}

#endif
#endif

void flx68000Init(void){
   static bool inited = false;

   if(!inited){
#if defined(EMU_FAST_CPU)
#if defined(EMU_OPTIMIZE_FOR_ARM32)
      CycloneInit();
      cycloneCpu.read8 = m68k_read_memory_8;
      cycloneCpu.read16 = m68k_read_memory_16;
      cycloneCpu.read32 = m68k_read_memory_32;
      cycloneCpu.fetch8 = m68k_read_memory_8;
      cycloneCpu.fetch16 = m68k_read_memory_16;
      cycloneCpu.fetch32 = m68k_read_memory_32;
      cycloneCpu.write8 = m68k_write_memory_8;
      cycloneCpu.write16 = m68k_write_memory_16;
      cycloneCpu.write32 = m68k_write_memory_32;
      cycloneCpu.checkpc = flx68000CheckPc;
      cycloneCpu.IrqCallback = interruptAcknowledge;
      cycloneCpu.ResetCallback = emulatorSoftReset;
#else
      fm68k_init();
      fameCpu.read_byte = m68k_read_memory_8;
      fameCpu.read_word = m68k_read_memory_16;
      fameCpu.read_long = m68k_read_memory_32;
      fameCpu.write_byte = m68k_write_memory_8;
      fameCpu.write_word = m68k_write_memory_16;
      fameCpu.write_long = m68k_write_memory_32;
      fameCpu.iack_handler = flx68000InterruptAcknowledge;
      fameCpu.reset_handler = emulatorSoftReset;
#endif
#else
      m68k_init();
      m68k_set_cpu_type(M68K_CPU_TYPE_68000);

      CPU_ADDRESS_MASK = 0xFFFFFFFF;

      m68k_set_reset_instr_callback(emulatorSoftReset);
      m68k_set_int_ack_callback(interruptAcknowledge);
#endif
      inited = true;
   }
}

void flx68000Reset(void){
   resetHwRegisters();
   resetAddressSpace();//address space must be reset after hardware registers because it is dependent on them
#if defined(EMU_FAST_CPU)
#if defined(EMU_OPTIMIZE_FOR_ARM32)
   CycloneReset(&cycloneCpu);
#else
   fm68k_reset(&fameCpu);
#endif
#else
   m68k_pulse_reset();
#endif
}

uint64_t flx68000StateSize(void){
   uint64_t size = 0;

#if defined(EMU_FAST_CPU)
#if defined(EMU_OPTIMIZE_FOR_ARM32)
   size += 0x80;//specified in Cyclone.h, line 82
#else
   size += sizeof(fameCpu);
#endif
#else
   size += sizeof(uint32_t) * 50;//m68ki_cpu
#endif

   return size;
}

void flx68000SaveState(uint8_t* data){
   uint64_t offset = 0;
#if defined(EMU_FAST_CPU)
#if defined(EMU_OPTIMIZE_FOR_ARM32)
   CyclonePack(&cycloneCpu, data + offset);
   offset += 0x80;//specified in Cyclone.h, line 82
#else
   memcpy(data + offset, &fameCpu, sizeof(fameCpu));
   offset += sizeof(fameCpu);
#endif
#else
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
#endif
}

void flx68000LoadState(uint8_t* data){
   uint64_t offset = 0;

#if defined(EMU_FAST_CPU)
#if defined(EMU_OPTIMIZE_FOR_ARM32)
   CycloneUnpack(&cycloneCpu, data + offset);
   offset += 0x80;//specified in Cyclone.h, line 82
#else
   memcpy(&fameCpu, data + offset, sizeof(fameCpu));
   offset += sizeof(fameCpu);
#endif
#else
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
#endif
}

void flx68000Execute(void){
   double cyclesRemaining = palmSysclksPerClk32;

   beginClk32();

   while(cyclesRemaining >= 1.0){
      double sysclks = dMin(cyclesRemaining, EMU_SYSCLK_PRECISION);
      int32_t cpuCycles = sysclks * pctlrCpuClockDivider * palmClockMultiplier;

      if(cpuCycles > 0){
#if defined(EMU_FAST_CPU)
#if defined(EMU_OPTIMIZE_FOR_ARM32)
         cycloneCpu.cycles = cpuCycles;
         CycloneRun(&cycloneCpu);
#else
         fm68k_emulate(&fameCpu, cpuCycles, fm68k_reason_emulate);
#endif
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
#if defined(EMU_FAST_CPU)
#if defined(EMU_OPTIMIZE_FOR_ARM32)
   cycloneCpu.irq = irqLevel;
   CycloneFlushIrq(&cycloneCpu);
#else
   //fameCpu.interrupts[0] = irqLevel;
   //fm68k_would_interrupt(&fameCpu);
#endif
#else
   m68k_set_irq(irqLevel);
#endif
}

void flx68000RefreshAddressing(void){
#if defined(EMU_FAST_CPU) && !defined(EMU_OPTIMIZE_FOR_ARM32)
   uint32_t bank;

   MULTITHREAD_LOOP(bank) for(bank = 0; bank < TOTAL_MEMORY_BANKS; bank++){
      if(bankType[bank] == CHIP_A0_ROM)
         fameCpu.Fetch[bank] = &palmRom + BANK_ADDRESS(bank) - chips[CHIP_A0_ROM].start;
      else if(bankType[bank] == CHIP_DX_RAM)
         fameCpu.Fetch[bank] = &palmRam + BANK_ADDRESS(bank) - chips[CHIP_DX_RAM].start;
   }
#endif
}

bool flx68000IsSupervisor(void){
#if defined(EMU_FAST_CPU)
#if defined(EMU_OPTIMIZE_FOR_ARM32)
   return !!(CycloneGetSr(&cycloneCpu) & 0x2000);
#else
   return !!(fameCpu.sr & 0x2000);
#endif
#else
   return !!(m68k_get_reg(NULL, M68K_REG_SR) & 0x2000);
#endif
}

void flx68000BusError(uint32_t address, bool isWrite){
#if defined(EMU_FAST_CPU) || defined(EMU_NO_SAFETY)
   //no bus error callback
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
#if defined(EMU_FAST_CPU)
#if defined(EMU_OPTIMIZE_FOR_ARM32)
   if(reg < 8)
      return cycloneCpu.d[reg];
   else if(reg < 16)
      return cycloneCpu.a[reg - 8];
   else if(reg == 16)
      return cycloneCpu.pc;
   else if(reg == 17)
      return CycloneGetSr(&cycloneCpu);

   return 0x00000000;
#else
   if(reg < 8)
      return fameCpu.dreg[reg].D;
   else if(reg < 16)
      return fameCpu.areg[reg - 8].D;
   else if(reg == 16)
      return fm68k_get_pc(&fameCpu);
   else if(reg == 17)
      return fameCpu.sr;

   return 0x00000000;
#endif
#else
   return m68k_get_reg(NULL, reg);
#endif
}

uint32_t flx68000GetPc(void){
#if defined(EMU_FAST_CPU)
#if defined(EMU_OPTIMIZE_FOR_ARM32)
   return cycloneCpu.pc;
#else
   return fm68k_get_pc(&fameCpu);
#endif
#else
   return m68k_get_reg(NULL, M68K_REG_PC);
#endif
}

uint64_t flx68000ReadArbitraryMemory(uint32_t address, uint8_t size){
   uint64_t data = UINT64_MAX;//invalid access
#if defined(EMU_FAST_CPU)
#if defined(EMU_OPTIMIZE_FOR_ARM32)
   //reading from a hardware register FIFO will corrupt it!
   if(bankType[START_BANK(address)] != CHIP_NONE){
      uint16_t m68kSr = CycloneGetSr(&cycloneCpu);
      CycloneSetSr(&cycloneCpu, m68kSr | 0x2000);//prevent privilege violations
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
      CycloneSetSr(&cycloneCpu, m68kSr);
   }
#else
   //FAME here

#endif
#else
   //reading from a hardware register FIFO will corrupt it!
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
