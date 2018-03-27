#include <stdint.h>
#include <stdio.h>

#include "m68k/m68kops.h"


#define OPCODE_BGND 0x4AFA
#define OPCODE_CPU32_START 0xF800
#define OPCODE_CPU32_END 0xF83F


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
   
}

void cpu32OpTbls(void){
   
}

void cpu32OpTblsn(void){
   
}

void cpu32OpTblu(void){
   
}

void cpu32OpTblun(void){
   
}


void m68k_op_bgnd(void){
   printf("Opcode BGND not implemented!\n");
}

void m68k_op_cpu32_dispatch(void){
   printf("cpu32 opcodes not implemented!\n");
}


void patchMusashiOpcodeHandlerCpu32(){
   patchOpcode(OPCODE_BGND, m68k_op_bgnd, 1/*dont know how many cycles*/);
   
   for(uint16_t currentOpcode = OPCODE_CPU32_START; currentOpcode <= OPCODE_CPU32_END; currentOpcode++)
      patchOpcode(currentOpcode, m68k_op_cpu32_dispatch, 1/*dont know how many cycles*/);
}
