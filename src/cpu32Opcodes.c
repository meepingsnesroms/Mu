#include <stdint.h>
#include <stdio.h>

#include <boolean.h>

#include "m68k/m68kcpu.h"
#include "m68k/m68kops.h"


#define OPCODE_BGND 0x4AFA
#define OPCODE_CPU32_START 0xF800
#define OPCODE_CPU32_END 0xF83F


bool lowPowerStopActive;


static inline void patchOpcode(uint16_t opcode, void (*handler)(void), unsigned char cycles){
   if(m68ki_instruction_jump_table[opcode] == m68k_op_illegal){
      //opcode not taken, patch it
      m68ki_instruction_jump_table[opcode] = handler;
      m68ki_cycles[2][opcode] = cycles;//set how many cycles for 68020
   }
   else{
      printf("Opcode 0x%04X is already taken!\n", opcode);
   }
}


void cpu32OpLpstop(void){
   uint immData = OPER_I_16();
   if(FLAG_S && immData & 0x2000){
      //turn off the cpu and wait for interrupts
      m68ki_set_sr(immData);
      lowPowerStopActive = true;
   }
   else{
      //program lacks authority
      m68ki_exception_privilege_violation();
   }
}

void cpu32OpTbls(void){
   printf("TBLS opcode not implemented!\n");
}

void cpu32OpTblsn(void){
   printf("TBLSN opcode not implemented!\n");
}

void cpu32OpTblu(void){
   printf("TBLU opcode not implemented!\n");
}

void cpu32OpTblun(void){
   printf("TBLUN opcode not implemented!\n");
}


void m68k_op_bgnd(void){
   printf("Opcode BGND not implemented!\n");
}

void m68k_op_cpu32_dispatch(void){
   uint opcodePart1 = REG_IR;
   uint opcodePart2 = OPER_I_16();
   
   if(opcodePart1 == 0xF800 && opcodePart2 == 0x01C0){
      //Low Power Stop
      cpu32OpLpstop();
      return;
   }
   
   switch(opcodePart1 & 0x0C00){
      case 0x0800:
         //signed and rounded
         cpu32OpTbls();
         return;
      case 0x0C00:
         //signed, not rounded
         cpu32OpTblsn();
         return;
      case 0x0000:
         //unsigned and rounded
         cpu32OpTblu();
         return;
      case 0x0400:
         //unsigned, not rounded
         cpu32OpTblun();
         return;
   }
   
   m68ki_exception_illegal();
}


void patchMusashiOpcodeHandlerCpu32(){
   patchOpcode(OPCODE_BGND, m68k_op_bgnd, 16/*dont know how many cycles, average opcode*/);
   
   for(uint16_t currentOpcode = OPCODE_CPU32_START; currentOpcode <= OPCODE_CPU32_END; currentOpcode++)
      patchOpcode(currentOpcode, m68k_op_cpu32_dispatch, 91/*dont know how many cycles, most expenceive opcode*/);
}
