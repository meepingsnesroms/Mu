#include <stdint.h>
#include <stdbool.h>


#if defined(EMU_DEBUG) && defined(EMU_SANDBOX)
//this wrappers 68k code and allows calling it for tests, this should be useful for determining if hardware accesses are correct
//Note: when running a test the emulator runs at native speed and no clocks are emulated
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "../m68k/m68k.h"
#include "../m68k/m68kcpu.h"
#include "../emulator.h"
#include "../ads7846.h"
#include "../hardwareRegisters.h"
#include "../portability.h"
#include "../specs/emuFeatureRegisterSpec.h"
#include "sandbox.h"
#include "trapNames.h"


typedef struct{
   void* hostPointer;
   uint32_t emuPointer;
   uint64_t bytes;//may be used for strings or structs in the future
}return_pointer_t;

typedef struct{
   uint32_t sp;
   uint32_t pc;
   uint16_t sr;
   uint32_t a0;
   uint32_t d0;
}local_cpu_state_t;


static bool              sandboxActive;//used to "log out" of the emulator once a test has finished
static bool              sandboxControlHandoff;//used for functions that depend on timing, hands full control to the 68k
static local_cpu_state_t sandboxOldFunctionCpuState;


static uint32_t sandboxCallGuestFunction(bool fallthrough, uint32_t address, uint16_t trap, const char* prototype, ...);

#include "sandboxTrapNumToName.c.h"

static char* takeStackDump(uint32_t bytes){
   char* textBytes = malloc(bytes * 2);
   uint32_t textBytesOffset = 0;
   uint32_t stackAddress = m68k_get_reg(NULL, M68K_REG_SP);
   uint32_t count;

   textBytes[0] = '\0';

   for(count = 0; count < bytes; count++){
      sprintf(textBytes + textBytesOffset, "%02X", m68k_read_memory_8(stackAddress + count));
      textBytesOffset = strlen(textBytes);
   }

   return textBytes;
}

void patchOsRom(uint32_t address, char* patch){
   uint32_t offset;
   uint32_t patchBytes = strlen(patch) / 2;//1 char per nibble
   uint32_t swapBegin = address & 0xFFFFFFFE;
   uint32_t swapSize = patchBytes / sizeof(uint16_t) + 1;
   char conv[5] = "0xXX";

   swap16BufferIfLittle(&palmRom[swapBegin], swapSize);
   for(offset = 0; offset < patchBytes; offset++){
      conv[2] = patch[offset * 2];
      conv[3] = patch[offset * 2 + 1];
      palmRom[address + offset] = strtol(conv, NULL, 0);
   }
   swap16BufferIfLittle(&palmRom[swapBegin], swapSize);
}

static bool ignoreTrap(uint16_t trap){
   switch(trap){
      case HwrDisableDataWrites:
      case HwrEnableDataWrites:
      case DmGetDatabase://this could be important later but is spammy on boot
      case MemStoreInfo://this could be important later but is spammy on boot
      case HwrDelay:
         return true;
   }

   return false;
}

static void printTrapInfo(uint16_t trap){
   debugLog("name:%s, API:0x%04X, location:0x%08X\n", lookupTrap(trap), trap, m68k_read_memory_32(0x000008CC + (trap & 0x0FFF) * 4));
}

static void logApiCalls(void){
   uint32_t programCounter = m68k_get_reg(NULL, M68K_REG_PPC);
   uint16_t instruction = m68k_get_reg(NULL, M68K_REG_IR);

   if(instruction == 0x4E4F){
      //Trap F/API call
      uint16_t trap = m68k_read_memory_16(programCounter + 2);
      if(!ignoreTrap(trap)){
         debugLog("Trap F API:%s, API number:0x%04X, PC:0x%08X\n", lookupTrap(trap), trap, programCounter);
      }
   }
}

static uint32_t makePalmString(const char* str){
   uint32_t strLength = strlen(str) + 1;
   uint32_t strData = sandboxCallGuestFunction(false, 0x00000000, MemPtrNew, "p(l)", strLength);

   if(strData){
      uint32_t count;

      for(count = 0; count < strLength; count++)
         m68k_write_memory_8(strData + count, str[count]);
   }

   return strData;
}

static char* makeNativeString(uint32_t address){
   if(address){
      int16_t strLength = sandboxCallGuestFunction(false, 0x00000000, StrLen, "w(p)", address) + 1;
      char* nativeStr = malloc(strLength);
      int16_t count;

      for(count = 0; count < strLength; count++)
         nativeStr[count] = m68k_read_memory_8(address + count);
      return nativeStr;
   }
   return NULL;
}

static void freePalmString(uint32_t address){
   sandboxCallGuestFunction(false, 0x00000000, MemChunkFree, "w(p)", address);
}

static bool installResourceToDevice(buffer_t resourceBuffer){
   /*
   #define memNewChunkFlagNonMovable    0x0200
   #define memNewChunkFlagAllowLarge    0x1000  // this is not in the sdk *g*

   void *MemPtrNewL ( UInt32 size ) {

     SysAppInfoPtr appInfoP;
     UInt16        ownerID;

     ownerID =
       ((SysAppInfoPtr)SysGetAppInfo(&appInfoP, &appInfoP))->memOwnerID;

     return MemChunkNew ( 0, size, ownerID |
                memNewChunkFlagNonMovable |
                memNewChunkFlagAllowLarge );

   }
   */

   //uint32_t palmSideResourceData = callFunction(false, 0x00000000, MemPtrNew, "p(l)", (uint32_t)resourceBuffer.size);
   uint32_t palmSideResourceData = sandboxCallGuestFunction(false, 0x00000000, MemChunkNew, "p(wlw)", 1/*heapID, storage RAM*/, (uint32_t)resourceBuffer.size, 0x1200/*attr, seems to work without memOwnerID*/);
   bool storageRamReadOnly = chips[CHIP_DX_RAM].readOnlyForProtectedMemory;
   uint16_t error;
   uint32_t count;

   //buffer not allocated
   if(!palmSideResourceData)
      return false;

   chips[CHIP_DX_RAM].readOnlyForProtectedMemory = false;//need to unprotect storage RAM
   for(count = 0; count < resourceBuffer.size; count++)
      m68k_write_memory_8(palmSideResourceData + count, resourceBuffer.data[count]);
   chips[CHIP_DX_RAM].readOnlyForProtectedMemory = storageRamReadOnly;//restore old protection state
   error = sandboxCallGuestFunction(false, 0x00000000, DmCreateDatabaseFromImage, "w(p)", palmSideResourceData);//Err DmCreateDatabaseFromImage(MemPtr bufferP);//this looks best
   sandboxCallGuestFunction(false, 0x00000000, MemChunkFree, "w(p)", palmSideResourceData);

   //didnt install
   if(error != 0)
      return false;

   return true;
}

static uint32_t sandboxGetStackFrameSize(const char* prototype){
   const char* params = prototype + 2;
   uint32_t size = 0;

   while(*params != ')'){
      switch(*params){
         case 'v':
         case 'V':
            //do nothing
            break;

         case 'b':
            //bytes are 16 bits long on the stack due to memory alignment restrictions
         case 'w':
            size += 2;
            break;

         case 'l':
         case 'p':
         case 'B':
         case 'W':
         case 'L':
         case 'P':
            size += 4;
            break;
      }

      params++;
   }

   return size;
}

static void sandboxBackupCpuState(void){
   sandboxOldFunctionCpuState.sp = m68k_get_reg(NULL, M68K_REG_SP);
   sandboxOldFunctionCpuState.pc = m68k_get_reg(NULL, M68K_REG_PC);
   sandboxOldFunctionCpuState.sr = m68k_get_reg(NULL, M68K_REG_SR);
   sandboxOldFunctionCpuState.a0 = m68k_get_reg(NULL, M68K_REG_A0);
   sandboxOldFunctionCpuState.d0 = m68k_get_reg(NULL, M68K_REG_D0);
}

static void sandboxRestoreCpuState(void){
   m68k_set_reg(M68K_REG_SP, sandboxOldFunctionCpuState.sp);
   m68k_set_reg(M68K_REG_PC, sandboxOldFunctionCpuState.pc);
   m68k_set_reg(M68K_REG_SR, sandboxOldFunctionCpuState.sr & 0xF0FF | m68k_get_reg(NULL, M68K_REG_SR) & 0x0700);//dont restore intMask
   m68k_set_reg(M68K_REG_A0, sandboxOldFunctionCpuState.a0);
   m68k_set_reg(M68K_REG_D0, sandboxOldFunctionCpuState.d0);
}

static uint32_t sandboxCallGuestFunction(bool fallthrough, uint32_t address, uint16_t trap, const char* prototype, ...){
   //prototype is a Java style function signature describing values passed and returned "v(wllp)"
   //is return void and pass a uint16_t(word), 2 uint32_t(long) and 1 pointer
   //valid types are b(yte), w(ord), l(ong), p(ointer) and v(oid), a capital letter means its a return pointer
   //EvtGetPen v(WWB) returns nothing but writes back to the calling function with 3 pointers,
   //these are allocated in the bootloader area and interpreted to host pointers on return
   va_list args;
   const char* params = prototype + 2;
   uint32_t stackFrameStart = m68k_get_reg(NULL, M68K_REG_SP);
   uint32_t newStackFrameSize = sandboxGetStackFrameSize(prototype);
   uint32_t stackWriteAddr = stackFrameStart - newStackFrameSize;
   uint32_t oldStopped = m68ki_cpu.stopped;
   uint32_t functionReturn = 0x00000000;
   return_pointer_t functionReturnPointers[10];
   uint8_t functionReturnPointerIndex = 0;
   uint32_t callWriteOut = 0xFFFFFFE0;
   uint32_t callStart;
   uint8_t count;

   sandboxBackupCpuState();

   va_start(args, prototype);
   while(*params != ')'){
      switch(*params){
         case 'v':
         case 'V':
            //do nothing, this is wrong for "V"
            break;

         case 'b':
            //bytes are 16 bits long on the stack due to memory alignment restrictions
            //bytes are written to the top byte of there word
            m68k_write_memory_8(stackWriteAddr, va_arg(args, uint32_t));
            stackWriteAddr += 2;
            break;
         case 'w':
            m68k_write_memory_16(stackWriteAddr, va_arg(args, uint32_t));
            stackWriteAddr += 2;
            break;

         case 'l':
         case 'p':
            m68k_write_memory_32(stackWriteAddr, va_arg(args, uint32_t));
            stackWriteAddr += 4;
            break;

         //return pointer values
         case 'B':
            functionReturnPointers[functionReturnPointerIndex].hostPointer = va_arg(args, void*);
            functionReturnPointers[functionReturnPointerIndex].emuPointer = callWriteOut;
            functionReturnPointers[functionReturnPointerIndex].bytes = 1;
            m68k_write_memory_32(stackWriteAddr, functionReturnPointers[functionReturnPointerIndex].emuPointer);
            stackWriteAddr += 4;
            callWriteOut += 4;
            functionReturnPointerIndex++;
            break;

         case 'W':
            functionReturnPointers[functionReturnPointerIndex].hostPointer = va_arg(args, void*);
            functionReturnPointers[functionReturnPointerIndex].emuPointer = callWriteOut;
            functionReturnPointers[functionReturnPointerIndex].bytes = 2;
            m68k_write_memory_32(stackWriteAddr, functionReturnPointers[functionReturnPointerIndex].emuPointer);
            stackWriteAddr += 4;
            callWriteOut += 4;
            functionReturnPointerIndex++;
            break;

         case 'L':
         case 'P':
            functionReturnPointers[functionReturnPointerIndex].hostPointer = va_arg(args, void*);
            functionReturnPointers[functionReturnPointerIndex].emuPointer = callWriteOut;
            functionReturnPointers[functionReturnPointerIndex].bytes = 4;
            m68k_write_memory_32(stackWriteAddr, functionReturnPointers[functionReturnPointerIndex].emuPointer);
            stackWriteAddr += 4;
            callWriteOut += 4;
            functionReturnPointerIndex++;
            break;
      }

      params++;
   }

   //write to the bootloader memory, its not important when debugging
   callStart = callWriteOut;

   if(address){
      //direct jump handler, used for private APIs
      m68k_write_memory_16(callWriteOut, 0x4EB9);//jump to subroutine opcode
      callWriteOut += 2;
      m68k_write_memory_32(callWriteOut, address);
      callWriteOut += 4;
   }
   else{
      //OS function handler
      m68k_write_memory_16(callWriteOut, 0x4E4F);//trap f opcode
      callWriteOut += 2;
      m68k_write_memory_16(callWriteOut, trap);
      callWriteOut += 2;
   }

   //end execution with CMD_EXECUTION_DONE
   m68k_write_memory_16(callWriteOut, 0x23FC);//move.l data imm to address at imm2 opcode
   callWriteOut += 2;
   m68k_write_memory_32(callWriteOut, CMD_DEBUG_EXEC_END);
   callWriteOut += 4;
   m68k_write_memory_32(callWriteOut, EMU_REG_ADDR(EMU_CMD));
   callWriteOut += 4;

   sandboxActive = true;
   if(fallthrough)
      sandboxControlHandoff = true;
   else
      m68ki_cpu.stopped = 0;
   m68k_set_reg(M68K_REG_SP, stackFrameStart - newStackFrameSize);
   m68k_set_reg(M68K_REG_PC, callStart);

   if(!fallthrough){
      while(sandboxActive)
         m68k_execute(1);//m68k_execute() always runs requested cycles + extra cycles of the final opcode, this executes 1 opcode
      if(prototype[0] == 'p')
         functionReturn = m68k_get_reg(NULL, M68K_REG_A0);
      else if(prototype[0] == 'b' || prototype[0] == 'w' || prototype[0] == 'l')
         functionReturn = m68k_get_reg(NULL, M68K_REG_D0);
      m68ki_cpu.stopped = oldStopped;
      sandboxRestoreCpuState();

      //remap all argument pointers
      for(count = 0; count < functionReturnPointerIndex; count++){
         switch(functionReturnPointers[count].bytes){
            case 1:
               *(uint8_t*)functionReturnPointers[count].hostPointer = m68k_read_memory_8(functionReturnPointers[count].emuPointer);
               break;

            case 2:
               *(uint16_t*)functionReturnPointers[count].hostPointer = m68k_read_memory_16(functionReturnPointers[count].emuPointer);
               break;

            case 4:
               *(uint32_t*)functionReturnPointers[count].hostPointer = m68k_read_memory_32(functionReturnPointers[count].emuPointer);
               break;
         }
      }
   }

   va_end(args);
   return functionReturn;
}


void sandboxInit(void){
   sandboxActive = false;
   sandboxControlHandoff = false;
}

uint32_t sandboxCommand(uint32_t command, void* data){
   uint32_t result = EMU_ERROR_NONE;

   //tests cant run properly(they hang forever) unless the debug return hook is enabled, it also completly destroys accuracy to execute hacked in asm buffers
   if(!(palmEmuFeatures.info & FEATURE_DEBUG))
      return EMU_ERROR_NOT_IMPLEMENTED;

   debugLog("Sandbox: Command %d started\n", command);

   switch(command){
      case SANDBOX_PATCH_OS:{
            //this will not be in the v1.0 release
            //remove parts of the OS that cause lockups, yeah its bad

            //remove ErrDisplayFileLineMsg from HwrIRQ2Handler, device locks on USB polling without this
            //this is fixed, leaving it here for reference
            //patchOsRom(0x83652, "4E714E71");//nop; nop

            //make SysSetTrapAddress/SysGetTrapAddress support traps > ScrDefaultPaletteState 0xA459
            //up to final OS 5.3 trap DmSyncDatabase 0xA476
            //SysSetTrapAddress_1001AE36:
            //SysGetTrapAddress_1001AE7C:
            //patchOsRom(0x1AE42, "0C410477");//cmpi.w 0x477, d1
            //patchOsRom(0x1AE8A, "0C42A477");//cmpi.w 0xA477, d2

            //double dynamic heap size, verified working
            //HwrCalcDynamicRAMSize_10005CC6:
            //HwrCalcDynamicRAMSize_10083B0A:
            //patchOsRom(0x5CC6, "203C000800004E75");//move.l 0x80000, d0; rts
            //patchOsRom(0x83B0A, "203C000800004E75");//move.l 0x80000, d0; rts

            //patch PrvChunkNew to only allocate in 4 byte intervals
            //PrvChunkNew_10020CBC:
            //TODO

            //set RAM to 32MB
            //patchOsRom(0x2C5E, "203C020000004E75");//move.l 0x2000000, d0; rts
            //patchOsRom(0x8442E, "203C020000004E75");//move.l 0x2000000, d0; rts

            //set RAM to 128MB
            //PrvGetRAMSize_10002C5E, small ROM
            //PrvGetRAMSize_1008442E, big ROM
            //patchOsRom(0x2C5E, "203C080000004E75");//move.l 0x8000000, d0; rts
            //patchOsRom(0x8442E, "203C080000004E75");//move.l 0x8000000, d0; rts
            //ROM:100219D0                 move.l  #unk_FFFFFF,d0
            //patchOsRom(0x219D0, "203C01FFFFFF4E75");//move.l 0x1FFFFFF, d0; rts
            //bus error at 0x1001DEDA when 128MB is present
            //0x55 memory filler, 32 bit, at PC:0x1001FFDC, and of course, its MemSet, need a stack trace now
            //PrvInitHeapPtr_10021908 sets up the 0x55 stuff in RAM
            /*
            ROM:100219C2                 move.l  d0,6(a4)        ; Move Data from Source to Destination
            ROM:100219C6                 tst.b   arg_A(a6)       ; Test an Operand
            ROM:100219CA                 beq.s   loc_100219EA    ; Branch if Equal
            ROM:100219CC                 move.b  #$55,-(sp) ; 'U' ; Move Data from Source to Destination
            ROM:100219D0                 move.l  #unk_FFFFFF,d0  ; Move Data from Source to Destination
            ROM:100219D6                 and.l   (a3),d0         ; AND Logical
            ROM:100219D8                 subq.l  #8,d0           ; Subtract Quick
            ROM:100219DA                 move.l  d0,-(sp)        ; Move Data from Source to Destination
            ROM:100219DC                 movea.l a3,a0           ; Move Address
            ROM:100219DE                 pea     8(a0)           ; Push Effective Address
            ROM:100219E2                 trap    #$F             ; Trap sysTrapMemSet
            ROM:100219E2                 dc.w    $A027
            ROM:100219E6                 lea     $A(sp),sp       ; Load Effective Address
            */
            //D0 is 0x3E1CC(254412) at PC:0x10021988

            // Add the heap, as long as it's not the dynamic heap.  During
            // bootup, the memory initialization sequence goes like:
            //
            //	if hard reset required:
            //		MemCardFormat
            //			lay out the card
            //			MemStoreInit
            //				for each heap
            //					MemHeapInit
            //	MemInit
            //		for each card:
            //			MemInitHeapTable
            //		for each dynamic heap:
            //			MemHeapInit
            //		for each RAM heap:
            //			Unlock all chunks
            //			Compact
            //
            // Which means that if there's no hard reset, MemHeapInit
            // has not been called on the dynamic heap at the time
            // MemInitHeapTable is called.  And since the dynamic heap
            // is currently in a corrupted state (because the boot stack
            // and initial LCD buffer have been whapped over it), we
            // can't perform the heap walk we'd normally do when adding
            // a heap object.

            //need to investigate what these vars are
            /*
            ROM:100148EE                 move.l  #$3BE,(dword_15C).w ; Move Data from Source to Destination
            ROM:100148F6                 move.l  #$422,(dword_112).w ; Move Data from Source to Destination
            ROM:100148FE                 move.l  #$890,(dword_11A).w ; Move Data from Source to Destination
            ROM:10014906                 move.l  #$8CC,(TrapTablePointer).w ; Move Data from Source to Destination
            ROM:1001490E                 move.w  #$45A,(word_13E).w ; Move Data from Source to Destination
            ROM:10014914                 move.w  #$1000,(word_28E).w ; Move Data from Source to Destination
            */

            //may be able to use these to set the border colors like in OS 5
            /*
            RAM:00001758                 dc.l UIColorInit_10074672
            RAM:0000175C                 dc.l UIColorGetTableEntryIndex_100746F6
            RAM:00001760                 dc.l UIColorGetTableEntryRGB_1007472A
            RAM:00001760                                         ; DATA XREF: ROM:101D66A1↓o
            RAM:00001764                 dc.l UIColorSetTableEntry_10074762
            RAM:00001768 off_1768:       dc.l UIColorPushTable_100747B0
            */

            //another road block surfaces
            /*
            When a Palm Powered handheld is presented with multiple dynamic heaps,
            the first heap (heap 0) on card 0 is the active dynamic heap.
            All other potential dynamic heaps are ignored. For example, it
            is possible that a future Palm Powered handheld supporting multiple
            cards might be presented with two cards, each having its own dynamic heap;
            if so, only the dynamic heap residing on card 0 would be active—the system
            would not treat any heaps on other cards as dynamic heaps, nor would heap
            IDs be assigned to these heaps. Subsequent storage heaps would be assigned
            IDs in sequential order, as always beginning with RAM heaps, followed by ROM heaps.
            */
         }
         break;

      case SANDBOX_INSTALL_APP:{
            buffer_t* app = (buffer_t*)data;
            bool success = installResourceToDevice(*app);

            if(!success)
               result = EMU_ERROR_OUT_OF_MEMORY;
         }
         break;
   }

   debugLog("Sandbox: Command %d finished\n", command);

   return result;
}

void sandboxOnOpcodeRun(void){
#if defined(EMU_SANDBOX_LOG_APIS)
   logApiCalls();
#endif
   switch(m68k_get_reg(NULL, M68K_REG_PC)){//switched this from PPC to PC
      //case 0x10083652://USB issue location //address based on PPC
      //case 0x100846C0://HwrDelay, before mysterious jump //address based on PPC
      //case 0x100846CC://HwrDelay, after mysterious jump //address based on PPC
      //case 0x100AD514://HwrSpiSdioInterrupts, SD card interrupt handler
      case 0x10021988://just before "andi.l #unk_FFFFFF, d0"
         //to add a emulator breakpoint add a new line above here|^^^
         {
            uint32_t m68kRegisters[M68K_REG_CPU_TYPE];
            uint8_t count;

            for(count = 0; count < M68K_REG_CPU_TYPE; count++)
               m68kRegisters[count] = m68k_get_reg(NULL, count);

            /*
            register order, read m68kRegisters with debugger
            M68K_REG_D0,
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
            M68K_REG_SR,
            M68K_REG_SP,
            M68K_REG_USP,
            M68K_REG_ISP,
            M68K_REG_MSP,
            M68K_REG_SFC,
            M68K_REG_DFC,
            M68K_REG_VBR,
            M68K_REG_CACR,
            M68K_REG_CAAR,
            M68K_REG_PREF_ADDR,
            M68K_REG_PREF_DATA,
            M68K_REG_PPC,
            M68K_REG_IR
            */

            //set host breakpoint here|vvv
            if(true){
               bool breakpoint = true;
            }
            //set host breakpoint here|^^^
         }
         break;
   }
}

bool sandboxRunning(void){
   uint32_t pc = m68k_get_reg(NULL, M68K_REG_PC);

   //this is used to capture full logs from certain sections of the ROM
   /*
   if(pc >= 0x100A07DC && pc <= 0x100ADB3C){
      //the SD slot driver
      return true;
   }
   */

   return sandboxActive;
}

void sandboxReturn(void){
   sandboxActive = false;
   if(sandboxControlHandoff){
      //control was just handed back to the host
      sandboxControlHandoff = false;
      sandboxRestoreCpuState();
      debugLog("Sandbox: Control returned to host\n");
   }
}

#else
void sandboxInit(void){}
uint32_t sandboxCommand(uint32_t command, void* data){return 0;}
void sandboxOnOpcodeRun(void){}
bool sandboxRunning(void){return false;}
void sandboxReturn(void){}
#endif
