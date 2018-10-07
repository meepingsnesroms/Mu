#include <stdint.h>
#include <stdbool.h>

#include "emulator.h"
#include "m68k/m68kcpu.h"
#include "portability.h"
#include "hardwareRegisters.h"
#include "memoryAccess.h"


void m68328Init(){
   static bool inited = false;

   if(!inited){
      m68k_init();
      m68k_set_cpu_type(M68K_CPU_TYPE_68000);

      CPU_ADDRESS_MASK = 0xFFFFFFFF;

      m68k_set_reset_instr_callback(emulatorReset);
      m68k_set_int_ack_callback(interruptAcknowledge);

      inited = true;
   }
}

void m68328Reset(){
   resetHwRegisters();
   resetAddressSpace();//address space must be reset after hardware registers because it is dependent on them
   m68k_pulse_reset();
}

uint64_t m68328StateSize(){
   uint64_t size = 0;

   size += sizeof(uint32_t) * 50;//m68ki_cpu

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
}

void m68328BusError(uint32_t address, bool isWrite){
   //never call outsize of a 68k opcode, behavior is undefined due to longjmp
   m68ki_trigger_bus_error(address, isWrite ? MODE_WRITE : MODE_READ, FLAG_S | m68ki_get_address_space());
}
