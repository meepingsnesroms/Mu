#include <stdint.h>
#include <stdbool.h>

#include "emulator.h"
#include "m68k/m68kcpu.h"
#include "m68k/m68kops.h"


#define OPCODE_BGND        0x4AFA
#define OPCODE_CPU32_START 0xF800
#define OPCODE_CPU32_END   0xF83F


bool lowPowerStopActive;


static inline void patchOpcode(uint16_t opcode, void (*handler)(void), uint8_t cycles){
      m68ki_instruction_jump_table[opcode] = handler;
      m68ki_cycles[0][opcode] = cycles;
      m68ki_cycles[1][opcode] = cycles;
      m68ki_cycles[2][opcode] = cycles;
}


void cpu32OpLpstop(void){
   uint immData = OPER_I_16();

   if(FLAG_S && immData & 0x2000){
      //turn off the CPU and wait for interrupts
      m68ki_set_sr(immData);
      lowPowerStopActive = true;
      debugLog("LowPowerStop set, CPU is off, PC:0x%08X!\n", m68k_get_reg(NULL, M68K_REG_PPC));
   }
   else{
      //program lacks authority
      m68ki_exception_privilege_violation();
   }
}

void cpu32OpTbls(void){
   debugLog("TBLS opcode not implemented, PC:0x%08X!\n", m68k_get_reg(NULL, M68K_REG_PPC));
}

void cpu32OpTblsn(void){
   debugLog("TBLSN opcode not implemented, PC:0x%08X!\n", m68k_get_reg(NULL, M68K_REG_PPC));
}

void cpu32OpTblu(void){
   debugLog("TBLU opcode not implemented, PC:0x%08X!\n", m68k_get_reg(NULL, M68K_REG_PPC));
}

void cpu32OpTblun(void){
   debugLog("TBLUN opcode not implemented, PC:0x%08X!\n", m68k_get_reg(NULL, M68K_REG_PPC));
}


void m68k_op_bgnd(void){
   debugLog("Opcode BGND not implemented, PC:0x%08X!\n", m68k_get_reg(NULL, M68K_REG_PPC));
}

void m68k_op_cpu32_dispatch(void){
   uint opcodePart1 = REG_IR;
   uint opcodePart2 = OPER_I_16();
   
   if(opcodePart1 == 0xF800 && opcodePart2 == 0x01C0){
      //low power stop
      cpu32OpLpstop();
   }
   else{
      //other opcodes
      switch(opcodePart1 & 0x0C00){
         case 0x0800:
            //signed and rounded
            cpu32OpTbls();
            break;

         case 0x0C00:
            //signed, not rounded
            cpu32OpTblsn();
            break;

         case 0x0000:
            //unsigned and rounded
            cpu32OpTblu();
            break;

         case 0x0400:
            //unsigned, not rounded
            cpu32OpTblun();
            break;

         default:
            m68ki_exception_1111();//illegal coprocessor instruction
            break;
      }
   }
}


void patchTo68328(){
   CPU_ADDRESS_MASK = 0xFFFFFFFF;

   patchOpcode(OPCODE_BGND, m68k_op_bgnd, 16/*dont know how many cycles, average opcode*/);
   
   for(uint32_t currentOpcode = OPCODE_CPU32_START; currentOpcode <= OPCODE_CPU32_END; currentOpcode++)
      patchOpcode(currentOpcode, m68k_op_cpu32_dispatch, 91/*dont know how many cycles, most expensive opcode*/);

   //68328 may actually use some 68020 opcodes, those will be patched in if and when Palm OS attempts to call one
}

void triggerBusError(uint32_t address, bool isWrite){
   uint sr = m68ki_init_exception();

   m68ki_push_32(REG_PC);
   m68ki_push_16(sr);
   m68ki_push_16(REG_IR);
   m68ki_push_32(address);	/* access address */
   /* 0 0 0 0 0 0 0 0 0 0 0 R/W I/N FC
    * R/W  0 = write, 1 = read
    * I/N  0 = instruction, 1 = not
    * FC   3-bit function code
    */
   m68ki_push_16((isWrite ? MODE_WRITE : MODE_READ) | CPU_INSTR_MODE | FLAG_S | m68ki_get_address_space());

   m68ki_jump_vector(EXCEPTION_BUS_ERROR);
}
