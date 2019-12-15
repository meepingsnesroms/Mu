#include "uArmGlue.h"
#include "pxa260.h"
#include "pxa260_CPU.h"
#include "pxa260_MMU.h"
#include "pxa260_cp15.h"
#include "pxa260_PwrClk.h"
#include "pxa260_LCD.h"

#include "../emulator.h"
#include "../tungstenT3Bus.h"
#include "../w86l488.h"


static ArmMmu uArmMmu;
static ArmCoprocessor uArmCp14;
static ArmCP15 uArmCp15;


static Err mmuReadF(void* userData, UInt32* buf, UInt32 pa){
   *(uint32_t*)buf = read_word(pa);
   return 1;
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
   uArmCp14.regXfer = pxa260pwrClkPrvCoprocRegXferFunc;
   uArmCp14.dataProcessing = NULL;
   uArmCp14.memAccess = NULL;
   uArmCp14.twoRegF = NULL;
   uArmCp14.userData = &pxa260PwrClk;

   //pwrclk already inited in pxa260Reset
   cpuCoprocessorRegister(cpu, 14, &uArmCp14);

   mmuInit(&uArmMmu, mmuReadF, NULL);
   cp15Init(&uArmCp15, cpu, &uArmMmu);
}
