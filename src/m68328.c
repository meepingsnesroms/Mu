#include <stdint.h>
#include <stdbool.h>

#include "emulator.h"
#include "m68k/m68kcpu.h"
#include "m68k/m68kops.h"
#include "portability.h"
#include "hardwareRegisters.h"
#include "memoryAccess.h"


#define OPCODE_BGND        0x4AFA
#define OPCODE_CPU32_START 0xF800
#define OPCODE_CPU32_END   0xF83F


bool m68328LowPowerStop;


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
      m68328LowPowerStop = true;
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


void m68328Init(){
   static bool inited = false;

   if(!inited){
      m68k_init();
      m68k_set_cpu_type(M68K_CPU_TYPE_68000);

      CPU_ADDRESS_MASK = 0xFFFFFFFF;

      patchOpcode(OPCODE_BGND, m68k_op_bgnd, 16/*dont know how many cycles, average opcode*/);

      for(uint16_t currentOpcode = OPCODE_CPU32_START; currentOpcode <= OPCODE_CPU32_END; currentOpcode++)
         patchOpcode(currentOpcode, m68k_op_cpu32_dispatch, 91/*dont know how many cycles, most expensive opcode*/);

      //68328 may actually use some 68020 opcodes, those will be patched in if and when Palm OS attempts to call one

      m68k_set_reset_instr_callback(emulatorReset);
      m68k_set_int_ack_callback(interruptAcknowledge);

      inited = true;
   }
}

void m68328Reset(){
   resetHwRegisters();
   resetAddressSpace();//address space must be reset after hardware registers because it is dependent on them
   m68328LowPowerStop = false;
   m68k_pulse_reset();
}

uint64_t m68328StateSize(){
   uint64_t size = 0;

   size += sizeof(uint32_t) * 50;//m68ki_cpu
   size += sizeof(uint8_t);//m68328LowPowerStop

   return size;
}

void m68328SaveState(uint8_t* data){
   uint64_t offset = 0;

   for(uint8_t index = 0; index < 16; index++){
      writeStateValueUint32(data + offset, m68ki_cpu.dar[index]);
      offset += sizeof(uint32_t);
   }
   writeStateValueUint32(data + offset, m68ki_cpu.ppc);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.pc);
   offset += sizeof(uint32_t);
   for(uint8_t index = 0; index < 7; index++){
      writeStateValueUint32(data + offset, m68ki_cpu.sp[index]);
      offset += sizeof(uint32_t);
   }
   writeStateValueUint32(data + offset, m68ki_cpu.vbr);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.sfc);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.dfc);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.cacr);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.caar);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.ir);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.t1_flag);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.t0_flag);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.s_flag);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.m_flag);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.x_flag);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.n_flag);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.not_z_flag);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.v_flag);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.c_flag);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.int_mask);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.int_level);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.int_cycles);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.stopped);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.pref_addr);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.pref_data);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.address_mask);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.sr_mask);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.instr_mode);
   offset += sizeof(uint32_t);
   writeStateValueUint32(data + offset, m68ki_cpu.run_mode);
   offset += sizeof(uint32_t);
   writeStateValueBool(data + offset, m68328LowPowerStop);
   offset += sizeof(uint8_t);
}

void m68328LoadState(uint8_t* data){
   uint64_t offset = 0;

   for(uint8_t index = 0; index < 16; index++){
      m68ki_cpu.dar[index] = readStateValueUint32(data + offset);
      offset += sizeof(uint32_t);
   }
   m68ki_cpu.ppc = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.pc = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   for(uint8_t index = 0; index < 7; index++){
      m68ki_cpu.sp[index] = readStateValueUint32(data + offset);
      offset += sizeof(uint32_t);
   }
   m68ki_cpu.vbr = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.sfc = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.dfc = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.cacr = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.caar = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.ir = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.t1_flag = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.t0_flag = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.s_flag = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.m_flag = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.x_flag = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.n_flag = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.not_z_flag = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.v_flag = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.c_flag = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.int_mask = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.int_level = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.int_cycles = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.stopped = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.pref_addr = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.pref_data = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.address_mask = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.sr_mask = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.instr_mode = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68ki_cpu.run_mode = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   m68328LowPowerStop = readStateValueBool(data + offset);
   offset += sizeof(uint8_t);
}

void m68328BusError(uint32_t address, bool isWrite){
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
