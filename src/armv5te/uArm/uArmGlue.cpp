#include "uArmGlue.h"
#include "CPU_2.h"
#include "MMU_2.h"
#include "cp15.h"

#include "../../emulator.h"
#include "../../armv5te/cpu.h"
#include "../../armv5te/asmcode.h"
#include "../../armv5te/cpudefs.h"
#include "../../pxa260/pxa260.h"


static ArmMmu uArmMmu;
static ArmCoprocessor uArmCp14;
static ArmCP15 uArmCp15;


static Err mmuReadF(void* userData, UInt32* buf, UInt32 pa){
   *(uint32_t*)buf = read_word(pa);
   return 1;
}

Boolean	uArmCp14RegXferF	(struct ArmCpu* cpu, void* userData, Boolean two/* MCR2/MRC2 ? */, Boolean MRC, UInt8 op1, UInt8 Rx, UInt8 CRn, UInt8 CRm, UInt8 op2){
   //if(!cpu->coproc[vb8].regXfer(cpu, cpu->coproc[vb8].userData, specialInstr, (instr & 0x00100000UL) != 0, (instr >> 21) & 0x07, (instr >> 12) & 0x0F, (instr >> 16) & 0x0F, instr & 0x0F, (instr >> 5) & 0x07)) goto invalid_instr;
   Instruction inst;

   inst.raw = 0xE000E10 | op1 << 21 | Rx << 12 | CRn << 16 | CRm | op2 << 5;

   if(MRC)
      inst.raw |= 0x00100000;

   do_cp14_instruction(inst);
   return true;
}

Boolean	uArmCp14DatProcF	(struct ArmCpu* cpu, void* userData, Boolean two/* CDP2 ? */, UInt8 op1, UInt8 CRd, UInt8 CRn, UInt8 CRm, UInt8 op2){
   debugLog("uARM CP14 dat proc unimplemented\n");
   return false;
}

Boolean	uArmCp14MemAccsF	(struct ArmCpu* cpu, void* userData, Boolean two /* LDC2/STC2 ? */, Boolean N, Boolean store, UInt8 CRd, UInt32 addr, UInt8* option /* NULL if none */){
   debugLog("uARM CP14 mem access unimplemented\n");
   return false;
}

Boolean uArmCp14TwoRegF	(struct ArmCpu* cpu, void* userData, Boolean MRRC, UInt8 op, UInt8 Rd, UInt8 Rn, UInt8 CRm){
   debugLog("uARM CP14 2 reg access unimplemented\n");
   return false;
}

Boolean	uArmMemAccess(struct ArmCpu* cpu, void* buf, UInt32 vaddr, UInt8 size, Boolean write, Boolean priviledged, UInt8* fsr){
   if(!mmuTranslate(&uArmMmu, vaddr, priviledged, write, &vaddr, fsr))
      return false;

   if(write){
      switch(size){
         case 1:
            write_byte(vaddr, *(uint8_t*)buf);
            return true;

         case 2:
            write_half(vaddr, *(uint16_t*)buf);
            return true;

         case 4:
            write_word(vaddr, *(uint32_t*)buf);
            return true;

         default:
            debugLog("uARM wrote memory with invalid byte count:%d\n", size);
            return false;
      }
   }
   else{
      switch(size){
         case 1:
            *(uint8_t*)buf = read_byte(vaddr);
            return true;

         case 2:
            *(uint16_t*)buf = read_half(vaddr);
            return true;

         case 4:
            *(uint32_t*)buf = read_word(vaddr);
            return true;

         case 32:
            ((uint32_t*)buf)[0] = read_word(vaddr);
            ((uint32_t*)buf)[1] = read_word(vaddr + 4);
            ((uint32_t*)buf)[2] = read_word(vaddr + 8);
            ((uint32_t*)buf)[3] = read_word(vaddr + 12);
            ((uint32_t*)buf)[4] = read_word(vaddr + 16);
            ((uint32_t*)buf)[5] = read_word(vaddr + 20);
            ((uint32_t*)buf)[6] = read_word(vaddr + 24);
            ((uint32_t*)buf)[7] = read_word(vaddr + 28);
            return true;

         default:
            debugLog("uARM read memory with invalid byte count:%d\n", size);
            return false;
      }
   }
}

Boolean	uArmHypercall(struct ArmCpu* cpu){
   //no hypercalls
   return true;
}

void	uArmEmulErr	(struct ArmCpu* cpu, const char* err_str){
   debugLog("uARM error:%s\n", err_str);
}

void	uArmSetFaultAddr(struct ArmCpu* cpu, UInt32 adr, UInt8 faultStatus){
   debugLog("uARM set fault addr:0x%08X, status:0x%02X\n", adr, faultStatus);
}

void uArmInitCpXX(ArmCpu* cpu){
   uArmCp14.regXfer = uArmCp14RegXferF;
   uArmCp14.dataProcessing = uArmCp14DatProcF;
   uArmCp14.memAccess = uArmCp14MemAccsF;
   uArmCp14.twoRegF = uArmCp14TwoRegF;

   cpuCoprocessorRegister(cpu, 14, &uArmCp14);

   mmuInit(&uArmMmu, mmuReadF, NULL);
   cp15Init(&uArmCp15, cpu, &uArmMmu);
}
