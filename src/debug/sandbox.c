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
#include "../armv5.h"
#include "sandbox.h"
#include "trapNames.h"


#define SANDBOX_MAX_WATCH_REGIONS 1000


typedef struct{
   void*    hostPointer;
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

typedef struct{
   uint32_t address;
   uint32_t size;
   uint8_t  type;
}mem_region_t;


static bool              sandboxActive;//used to "log out" of the emulator once a test has finished
static bool              sandboxControlHandoff;//used for functions that depend on timing, hands full control to the 68k
static uint8_t           sandboxCurrentCpuArch;
static local_cpu_state_t sandboxOldFunctionCpuState;
static mem_region_t      sandboxWatchRegions[SANDBOX_MAX_WATCH_REGIONS];//code locations in m68k address space to be sandboxed
static uint16_t          sandboxWatchRegionsActive;//number of used sandboxWatchRegions entrys


static uint32_t sandboxCallGuestFunction(bool fallthrough, uint32_t address, uint16_t trap, const char* prototype, ...);

#include "sandboxTrapNumToName.c.h"

static uint32_t getCurrentCpuPc(void){
   //returns the current program counter for the current CPU and architecture
   return sandboxCurrentCpuArch == SANDBOX_CPU_ARCH_M68K ? m68k_get_reg(NULL, M68K_REG_PC) : armv5GetRegister(15) & 0xFFFFFFFE;
}

static uint32_t getCurrentCpuPreviousPc(void){
   //returns the program counter of the current running opcode for the current CPU and architecture
   return sandboxCurrentCpuArch == SANDBOX_CPU_ARCH_M68K ? m68k_get_reg(NULL, M68K_REG_PPC) : armv5GetRegister(15) & 0xFFFFFFFE;
}

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

static void patchOsRom(uint32_t address, char* patch){
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

static const char* getLowMemGlobalName(uint32_t address){
   switch(address){
      case 0x00000100:
         return "MemTotalCards";

      case 0x00000102:
         return "MemCard0Start";

      case 0x0000010A:
         return "MemDebugFlags";

      case 0x00000122:
         return "TrapTablePtr";

      case 0x00000164:
         return "ScrStatePtr";

      case 0x0000016C:
         return "PenStatePtr";

      case 0x00000170:
         return "EvtStatePtr";

      case 0x00000174:
         return "SndStatePtr";

      case 0x00000178:
         return "TimStatePtr";

      case 0x0000017C:
         return "AlmStatePtr";

      case 0x00000180:
         return "FtrStatePtr";

      case 0x00000184:
         return "GrfStatePtr";

      default:
         return "UNKNOWN";
   }
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

   if(instruction == 0x4E4F/*Trap F/API call opcode*/){
      uint16_t trap = m68k_read_memory_16(programCounter + 2);

      if(!ignoreTrap(trap))
         debugLog("Trap F API:%s, API number:0x%04X, PC:0x%08X\n", lookupTrap(trap), trap, programCounter);
   }
}

static bool isAlphanumeric(char chr){
   if((chr >= 'a' && chr <= 'z') || (chr >= 'A' && chr <= 'Z') || (chr >= '0' && chr <= '9'))
      return true;
   return false;
}

static bool readable6CharsBack(uint32_t address){
   uint8_t count;
   for(count = 0; count < 6; count++)
      if(!isAlphanumeric(m68k_read_memory_8(address - count)))
         return false;
   return true;
}

static uint32_t find68kString(const char* str, uint32_t rangeStart, uint32_t rangeEnd){
   uint32_t strLength = strlen(str) + 1;//include null terminator
   uint32_t scanAddress;

   for(scanAddress = rangeStart; scanAddress <= rangeEnd - (strLength - 1); scanAddress++){
      //since only the first char is range checked remove the rest from the range to prevent reading off the end

      //check every byte against the start character
      if(m68k_read_memory_8(scanAddress) == str[0]){
         bool wrongString = false;
         uint32_t strIndex;

          //character match found, check for string
         for(strIndex = 1; strIndex < strLength; strIndex++){
            if(m68k_read_memory_8(scanAddress + strIndex) != str[strIndex]){
               wrongString = true;
               break;
            }
         }

         if(wrongString == false)
            return scanAddress;
      }
   }

   return rangeEnd;
}

static uint32_t skip68kString(uint32_t address){
   while(m68k_read_memory_8(address) != '\0')
      address++;
   address++;//skip null terminator too
   return address;
}

//THIS FUNCTION DOES NOT WORK IF A WORD ALIGNED 0x0000 IS FOUND IN THE FUNCTION BEING SEARCHED FOR, THIS IS A BUG
static uint32_t scanForPrivateFunctionAddress(const char* name){
   //function name format [0x**(unknown), string(with null terminator), 0x00, 0x00(if last 0x00 was on an even address, protects opcode alignemnt)]
   //this is not 100% accurate, it scans memory for a function address based on a string
   //if a duplicate set of stings is found but not encasing a function a fatal error will occur on execution
   uint32_t rangeEnd = chips[CHIP_A0_ROM].start + chips[CHIP_A0_ROM].lineSize - 1;
   uint32_t address = find68kString(name, chips[CHIP_A0_ROM].start, rangeEnd);

   while(address < rangeEnd){
      uint32_t signatureBegining = address - 3;//last opcode of function being looked for if the string is correct

      //skip string to test the null terminators
      address = skip68kString(address);

      //after a function string there are 2 null terminators(the one all strings have and 1 extra) or 3(2 extra) if the previous one was on an even address
      if(m68k_read_memory_8(address) == '\0' && (address & 0x00000001 || m68k_read_memory_8(address + 1) == '\0')){
         //valid string match found, get prior functions signature(a function string has a minimum of 6 printable chars with 2 null terminators)
         uint32_t priorFunctionSignature = signatureBegining;//last opcode of the function thats being looked for
         bool extraNull = false;

         while(m68k_read_memory_16(priorFunctionSignature) != '\0\0')
            priorFunctionSignature -= 2;

         //remove extra null terminator if present
         if(m68k_read_memory_8(priorFunctionSignature - 1) == '\0'){
            priorFunctionSignature--;
            extraNull = true;
         }

         //check that there are 6 valid alphanumeric characters before the nulls, this indicates that its a valid function signature
         if(readable6CharsBack(priorFunctionSignature - 1)){
            //valid, get first opcode after string and return its address
            return priorFunctionSignature + (extraNull ? 3 : 2);
         }
      }

      //string matched but structure was invalid, get next string
      address = find68kString(name, address, rangeEnd);
   }

   return 0x00000000;
}

//call anywhere in a function to get its name, used to determine the location of a crash, must free the pointer after reading the string
static char* getFunctionName68k(uint32_t address){
   char* data;
   uint32_t offset;

   address &= 0xFFFFFFFE;

   for(offset = 0; offset < 0x10000; offset += 2){
      if(m68k_read_memory_16(address + offset) == 0x4E75/*RTS*/){
         offset += 3;
         break;
      }
   }

   if(offset < 0x10000){
      uint16_t size = 0;
      uint16_t offset2;

      for(offset2 = 0; offset2 < 0x100; offset2++)
         if(isAlphanumeric(m68k_read_memory_8(address + offset + offset2)))
            size++;

      data = malloc(size + 1);
      for(offset2 = 0; offset2 < size; offset2++)
         data[offset2] = m68k_read_memory_8(address + offset + offset2);
      data[offset2] = '\0';
   }
   else{
      data = NULL;
   }

   return data;
}

static char* getFunctionNameArmv5(uint32_t address){
   return NULL;
}

static char* getFunctionNameThumb(uint32_t address){
   return NULL;
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

   uint32_t palmSideResourceData = sandboxCallGuestFunction(false, 0x00000000, MemChunkNew, "p(wlw)", 1/*heapID, storage RAM*/, resourceBuffer.size, 0x1200/*attr, seems to work without memOwnerID*/);
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
   //nothing to init yet
}

void sandboxReset(void){
   sandboxActive = false;
   sandboxControlHandoff = false;

   memset(sandboxWatchRegions, 0x00, sizeof(sandboxWatchRegions));
   sandboxWatchRegionsActive = 0;

   //patch OS here if needed
   //debug patches
   //sandboxCommand(SANDBOX_PATCH_OS, NULL);

   //add memory region monitors
   /*
   if(pc >= 0x100A07DC && pc <= 0x100ADB3C){
      //the SD slot driver
      return true;
   }
   */

   //log all register accesses
   //sandboxCommand(SANDBOX_CMD_REGISTER_WATCH_ENABLE, NULL);
}

uint32_t sandboxStateSize(void){
   uint32_t size = 0;

   size += sizeof(uint8_t) * 3;//sandboxActive / sandboxControlHandoff / sandboxCurrentCpuArch
   size += sizeof(uint32_t) * 4;//sandboxOldFunctionCpuState.(sp/pc/a0/d0)
   size += sizeof(uint16_t);//sandboxOldFunctionCpuState.sr
   size += sizeof(uint32_t) * 2 * SANDBOX_MAX_WATCH_REGIONS;//sandboxWatchRegions.(address/size)
   size += sizeof(uint8_t) * SANDBOX_MAX_WATCH_REGIONS;//sandboxWatchRegions.type
   size += sizeof(uint16_t);//sandboxWatchRegionsActive

   return size;
}

void sandboxSaveState(uint8_t* data){
   uint32_t offset = 0;
   uint16_t index;

   writeStateValue8(data + offset, sandboxActive);
   offset += sizeof(uint8_t);
   writeStateValue8(data + offset, sandboxControlHandoff);
   offset += sizeof(uint8_t);
   writeStateValue8(data + offset, sandboxCurrentCpuArch);//currently cant be ARMv5 during a frame boundry but that may change
   offset += sizeof(uint8_t);
   writeStateValue32(data + offset, sandboxOldFunctionCpuState.sp);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, sandboxOldFunctionCpuState.pc);
   offset += sizeof(uint32_t);
   writeStateValue16(data + offset, sandboxOldFunctionCpuState.sr);
   offset += sizeof(uint16_t);
   writeStateValue32(data + offset, sandboxOldFunctionCpuState.a0);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, sandboxOldFunctionCpuState.d0);
   offset += sizeof(uint32_t);
   for(index = 0; index < SANDBOX_MAX_WATCH_REGIONS; index++){
      writeStateValue32(data + offset, sandboxWatchRegions[index].address);
      offset += sizeof(uint32_t);
      writeStateValue32(data + offset, sandboxWatchRegions[index].size);
      offset += sizeof(uint32_t);
      writeStateValue8(data + offset, sandboxWatchRegions[index].type);
      offset += sizeof(uint8_t);
   }
   writeStateValue16(data + offset, sandboxWatchRegionsActive);
   offset += sizeof(uint16_t);
}

void sandboxLoadState(uint8_t* data){
   uint32_t offset = 0;
   uint16_t index;

   sandboxActive = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   sandboxControlHandoff = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   sandboxCurrentCpuArch = readStateValue8(data + offset);//currently cant be ARMv5 during a frame boundry but that may change
   offset += sizeof(uint8_t);
   sandboxOldFunctionCpuState.sp = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   sandboxOldFunctionCpuState.pc = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   sandboxOldFunctionCpuState.sr = readStateValue16(data + offset);
   offset += sizeof(uint16_t);
   sandboxOldFunctionCpuState.a0 = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   sandboxOldFunctionCpuState.d0 = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   for(index = 0; index < SANDBOX_MAX_WATCH_REGIONS; index++){
      sandboxWatchRegions[index].address = readStateValue32(data + offset);
      offset += sizeof(uint32_t);
      sandboxWatchRegions[index].size = readStateValue32(data + offset);
      offset += sizeof(uint32_t);
      sandboxWatchRegions[index].type = readStateValue8(data + offset);
      offset += sizeof(uint8_t);
   }
   sandboxWatchRegionsActive = readStateValue16(data + offset);
   offset += sizeof(uint16_t);
}

uint32_t sandboxCommand(uint32_t command, void* data){
   uint32_t result = EMU_ERROR_NONE;

   //tests cant run properly(they hang forever) unless the debug return hook is enabled, it also completly destroys accuracy to execute hacked in asm buffers
   if(!(palmEmuFeatures.info & FEATURE_DEBUG))
      return EMU_ERROR_NOT_IMPLEMENTED;

   debugLog("Sandbox: Command %d started\n", command);

   switch(command){
      case SANDBOX_CMD_PATCH_OS:{
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
            //patchOsRom(0x5CC6, "203C001000004E75");//move.l 0x100000, d0; rts
            //patchOsRom(0x83B0A, "203C001000004E75");//move.l 0x100000, d0; rts

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

      case SANDBOX_CMD_DEBUG_INSTALL_APP:{
            buffer_t* app = (buffer_t*)data;
            bool success = installResourceToDevice(*app);

            if(!success)
               result = EMU_ERROR_OUT_OF_MEMORY;
         }
         break;


      case SANDBOX_CMD_REGISTER_WATCH_ENABLE:{
            sandboxSetWatchRegion(0xFFFFF000, 0xFFF, SANDBOX_WATCH_DATA);//0x1000 will cause an overflow to 0x00000000 making it never trigger
         }
         break;

      default:
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

void sandboxOnMemoryAccess(uint32_t address, uint8_t size, bool write, uint32_t value){
   static bool isRecursive = false;

   //this function can read or write Palm memory, which calls this function, so dont run this function when calling from this function to prevent an infinte loop
   if(!isRecursive){
      isRecursive = true;
      {//actual code goes below:vvv
         bool sandboxedMemory = false;
         uint16_t memRegion;

         for(memRegion = 0; memRegion < sandboxWatchRegionsActive; memRegion++){
            if(sandboxWatchRegions[memRegion].type == SANDBOX_WATCH_DATA && address >= sandboxWatchRegions[memRegion].address && address < sandboxWatchRegions[memRegion].address + sandboxWatchRegions[memRegion].size){
               sandboxedMemory = true;
               break;
            }
         }

         if(sandboxedMemory || sandboxRunning()){
            uint32_t pc = getCurrentCpuPreviousPc();
            char* function = sandboxCurrentCpuArch == SANDBOX_CPU_ARCH_M68K ? getFunctionName68k(pc) : sandboxCurrentCpuArch == SANDBOX_CPU_ARCH_ARMV5 ? getFunctionNameArmv5(pc) : getFunctionNameThumb(pc);
            bool functionValid = !!function;

            if(!functionValid)
               function = "NAME NOT FOUND";

            if(address < 0x00010000){
               //low mem globals
               const char* variableName = getLowMemGlobalName(address);//dont free this pointer, it just returns a string constant from a list of string constants

               if(write)
                  debugLog("Writing low mem global: name:%s/address:0x%08X, size:%d, value:0x%08X, function:%s/PC:0x%08X\n", variableName, address, size, value, function, pc);
               else
                  debugLog("Reading low mem global: name:%s/address:0x%08X, size:%d, function:%s/PC:0x%08X\n", variableName, address, size, function, pc);
            }
            else if(address >= 0xFFFFF000){
               //hardware registers
               if(address >= 0xFFFFFE00){
                  //bootloader area
                  if(write){
                     if(address >= 0xFFFFFFC0)
                        debugLog("Writing bootloader area(valid): address:0x%08X, size:%d, function:%s/PC:0x%08X\n", address, size, function, pc);
                     else
                        debugLog("Writing bootloader area(invalid): address:0x%08X, size:%d, function:%s/PC:0x%08X\n", address, size, function, pc);
                  }
                  else{
                     debugLog("Reading bootloader area: address:0x%08X, size:%d, function:%s/PC:0x%08X\n", address, size, function, pc);
                  }
               }
               else{
                  //normal regs
                  const char* registerName = "UNKNOWN";

                  //TODO: get register name list

                  if(write)
                     debugLog("Writing hardware register: name:%s/address:0x%08X, size:%d, value:0x%08X, function:%s/PC:0x%08X\n", registerName, address, size, value, function, pc);
                  else
                     debugLog("Reading hardware register: name:%s/address:0x%08X, size:%d, function:%s/PC:0x%08X\n", registerName, address, size, function, pc);
               }
            }
            else{
               //everywhere else
               if(write)
                  debugLog("Writing: address:0x%08X, size:%d, value:0x%08X, function:%s/PC:0x%08X\n", address, size, value, function, pc);
               else
                  debugLog("Reading: address:0x%08X, size:%d, function:%s/PC:0x%08X\n", address, size, function, pc);
            }

            if(functionValid)
               free(function);
         }
      }//actual code goes above:^^^
      isRecursive = false;
   }
}

bool sandboxRunning(void){
   uint32_t pc = getCurrentCpuPc();
   uint16_t memRegion;

   //this is used to capture full logs when running from specific locations
   for(memRegion = 0; memRegion < sandboxWatchRegionsActive; memRegion++){
      if(sandboxWatchRegions[memRegion].type == SANDBOX_WATCH_CODE && pc >= sandboxWatchRegions[memRegion].address && pc < sandboxWatchRegions[memRegion].address + sandboxWatchRegions[memRegion].size)
         return true;
   }

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

uint16_t sandboxSetWatchRegion(uint32_t address, uint32_t size, uint8_t type){
   uint16_t index;

   //invalid type
   if(type == SANDBOX_WATCH_NONE || type >= SANDBOX_WATCH_TOTAL_TYPES)
      return 0xFFFF;

   //cant watch 0 sized memory area
   if(size < 1)
      return 0xFFFF;

   //try to use old watch region if available
   for(index = 0; index < sandboxWatchRegionsActive; index++){
      if(sandboxWatchRegions[index].type == SANDBOX_WATCH_NONE){
         //found reusable region
         sandboxWatchRegions[index].address = address;
         sandboxWatchRegions[index].size = size;
         sandboxWatchRegions[index].type = type;

         //return watch region reference, this doesnt change until the region is deleted
         return index;
      }
   }

   //check if new region is available
   if(sandboxWatchRegionsActive < SANDBOX_MAX_WATCH_REGIONS){
      //use new region
      sandboxWatchRegions[sandboxWatchRegionsActive].address = address;
      sandboxWatchRegions[sandboxWatchRegionsActive].size = size;
      sandboxWatchRegions[sandboxWatchRegionsActive].type = type;
      sandboxWatchRegionsActive++;

      //return watch region reference, this doesnt change until the region is deleted
      return sandboxWatchRegionsActive - 1;
   }

   //no regions left, error
   return 0xFFFF;
}

void sandboxClearWatchRegion(uint16_t index){
   if(index < sandboxWatchRegionsActive){
      if(index == sandboxWatchRegionsActive - 1)
         sandboxWatchRegionsActive--;//remove from end
      else
         sandboxWatchRegions[index].type = SANDBOX_WATCH_NONE;//mark as empty
   }
}

void sandboxSetCpuArch(uint8_t arch){
   sandboxCurrentCpuArch = arch;
}

#else
void sandboxInit(void){}
void sandboxReset(void){}
uint32_t sandboxStateSize(void){return 0;}
void sandboxSaveState(uint8_t* data){}
void sandboxLoadState(uint8_t* data){}
uint32_t sandboxCommand(uint32_t command, void* data){return 0;}
void sandboxOnOpcodeRun(void){}
void sandboxOnMemoryAccess(uint32_t address, uint8_t size, bool write, uint32_t value){}
bool sandboxRunning(void){return false;}
void sandboxReturn(void){}
uint16_t sandboxSetWatchRegion(uint32_t address, uint32_t size, uint8_t type){}
void sandboxClearWatchRegion(uint16_t index){}
void sandboxSetCpuArch(uint8_t arch){}
#endif
