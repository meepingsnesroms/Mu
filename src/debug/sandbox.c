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
#include "../specs/emuFeatureRegistersSpec.h"
#include "sandbox.h"
#include "trapNames.h"


typedef struct{
   void* hostPointer;
   uint32_t emuPointer;
   uint64_t bytes;//may be used for strings or structs in the future
}return_pointer_t;


//used to "log out" of the emulator once a test has finished
static bool inSandbox;


#include "sandboxTrapNumToName.c.h"

static char* takeStackDump(uint32_t bytes){
   char* textBytes = malloc(bytes * 2);
   uint32_t textBytesOffset = 0;
   uint32_t stackAddress = m68k_get_reg(NULL, M68K_REG_SP);

   textBytes[0] = '\0';

   for(uint32_t count = 0; count < bytes; count++){
      sprintf(textBytes + textBytesOffset, "%02X", m68k_read_memory_8(stackAddress + count));
      textBytesOffset = strlen(textBytes);
   }

   return textBytes;
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

static void logApiCalls(){
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

static int64_t randomRange(int64_t start, int64_t end){
   static bool seedRng = true;
   int64_t result;

   if(seedRng){
      srand(time(NULL));
      seedRng = false;
   }

   result = rand();
   result <<= 32;
   result |= rand();
   result %= llabs(end - start) + 1;
   result += start;

   return result;
}


static bool isAlphanumeric(char chr){
   if((chr >= 'a' && chr <= 'z') || (chr >= 'A' && chr <= 'Z') || (chr >= '0' && chr <= '9'))
      return true;
   return false;
}

static bool readable6CharsBack(uint32_t address){
   for(uint8_t count = 0; count < 6; count++)
      if(!isAlphanumeric(m68k_read_memory_8(address - count)))
         return false;
   return true;
}

static uint32_t find68kString(const char* str, uint32_t rangeStart, uint32_t rangeEnd){
   uint32_t strLength = strlen(str) + 1;//include null terminator

   for(uint32_t scanAddress = rangeStart; scanAddress <= rangeEnd - (strLength - 1); scanAddress++){
      //since only the first char is range checked remove the rest from the range to prevent reading off the end

      //check every byte against the start character
      if(m68k_read_memory_8(scanAddress) == str[0]){
         bool wrongString = false;

          //character match found, check for string
         for(uint32_t strIndex = 1; strIndex < strLength; strIndex++){
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

static uint32_t getNewStackFrameSize(const char* prototype){
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

static uint32_t callFunction(bool fallthrough, uint32_t address, uint16_t trap, const char* prototype, ...){
   //prototype is a java style function signature describing values passed and returned "v(wllp)"
   //is return void and pass a uint16_t(word), 2 uint32_t(long) and 1 pointer
   //valid types are b(yte), w(ord), l(ong), p(ointer) and v(oid), a capital letter means its a return pointer
   //EvtGetPen v(WWB) returns nothing but writes back to the calling function with 3 pointers,
   //these are allocated in the bootloader area and interpreted to host pointers on return
   va_list args;
   const char* params = prototype + 2;
   uint32_t stackFrameStart = m68k_get_reg(NULL, M68K_REG_SP);
   uint32_t newStackFrameSize = getNewStackFrameSize(prototype);
   uint32_t stackWriteAddr = stackFrameStart - newStackFrameSize;
   uint32_t oldStopped = m68ki_cpu.stopped;
   uint32_t oldPc = m68k_get_reg(NULL, M68K_REG_PC);
   uint32_t oldA0 = m68k_get_reg(NULL, M68K_REG_A0);
   uint32_t oldD0 = m68k_get_reg(NULL, M68K_REG_D0);
   uint32_t functionReturn = 0x00000000;
   return_pointer_t functionReturnPointers[10];
   uint8_t functionReturnPointerIndex = 0;
   uint32_t callWriteOut = 0xFFFFFFE0;
   uint32_t callStart;

   va_start(args, prototype);
   while(*params != ')'){
      switch(*params){
         case 'v':
         case 'V':
            //do nothing
            break;

         case 'b':
            //bytes are 16 bits long on the stack due to memory alignment restrictions
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
   m68k_write_memory_32(callWriteOut, MAKE_EMU_CMD(CMD_EXECUTION_DONE));
   callWriteOut += 4;
   m68k_write_memory_32(callWriteOut, EMU_REG_ADDR(EMU_CMD));
   callWriteOut += 4;

   inSandbox = true;
   m68ki_cpu.stopped = 0;
   m68k_set_reg(M68K_REG_SP, stackFrameStart - newStackFrameSize);
   m68k_set_reg(M68K_REG_PC, callStart);

   //only setup the trap then fallthrough to normal execution, may be needed on app switch since the trap may not return
   if(!fallthrough){
      while(inSandbox)
         m68k_execute(1);//m68k_execute() always runs requested cycles + extra cycles of the final opcode, this executes 1 opcode
      if(prototype[0] == 'p')
         functionReturn = m68k_get_reg(NULL, M68K_REG_A0);
      else if(prototype[0] == 'b' || prototype[0] == 'w' || prototype[0] == 'l')
         functionReturn = m68k_get_reg(NULL, M68K_REG_D0);
      m68ki_cpu.stopped = oldStopped;
      m68k_set_reg(M68K_REG_PC, oldPc);
      m68k_set_reg(M68K_REG_SP, stackFrameStart);
      m68k_set_reg(M68K_REG_A0, oldA0);
      m68k_set_reg(M68K_REG_D0, oldD0);

      //remap all argument pointers
      for(uint8_t count = 0; count < functionReturnPointerIndex; count++){
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

static uint32_t makePalmString(const char* str){
   uint32_t strLength = strlen(str) + 1;
   uint32_t strData = callFunction(false, 0x00000000, MemPtrNew, "p(l)", strLength);

   if(strData != 0)
      for(uint32_t count = 0; count < strLength; count++)
         m68k_write_memory_8(strData + count, str[count]);
   return strData;
}

static char* makeNativeString(uint32_t address){
   if(address != 0){
      int16_t strLength = callFunction(false, 0x00000000, StrLen, "w(p)", address) + 1;
      char* nativeStr = malloc(strLength);

      for(int16_t count = 0; count < strLength; count++)
         nativeStr[count] = m68k_read_memory_8(address + count);
      return nativeStr;
   }
   return NULL;
}

static void freePalmString(uint32_t address){
   callFunction(false, 0x00000000, MemChunkFree, "w(p)", address);
}

static bool installResourceToDevice(buffer_t resourceBuffer){
   uint32_t palmSideResourceData = callFunction(false, 0x00000000, MemPtrNew, "p(l)", (uint32_t)resourceBuffer.size);
   uint16_t error;

   //buffer not allocated
   if(!palmSideResourceData)
      return false;

   for(uint32_t count = 0; count < resourceBuffer.size; count++)
      m68k_write_memory_8(palmSideResourceData + count, resourceBuffer.data[count]);
   error = callFunction(false, 0x00000000, DmCreateDatabaseFromImage, "w(p)", palmSideResourceData);//Err DmCreateDatabaseFromImage(MemPtr bufferP);//this looks best
   callFunction(false, 0x00000000, MemChunkFree, "w(p)", palmSideResourceData);

   //dident install
   if(error != 0)
      return false;

   return true;
}

void sandboxInit(){
   inSandbox = false;
#if defined(EMU_SANDBOX_OPCODE_LEVEL_DEBUG)
   m68k_set_instr_hook_callback(sandboxOnOpcodeRun);
#endif
}

uint32_t sandboxCommand(uint32_t test, void* data){
   uint32_t result = EMU_ERROR_NONE;

   //tests cant run properly(hang forever) unless the debug return hook is enabled, it also completly destroys accuracy to execute hacked in asm buffers
   if(!(palmSpecialFeatures & FEATURE_DEBUG))
      return EMU_ERROR_NOT_IMPLEMENTED;

   debugLog("Sandbox: Test %d started\n", test);

   switch(test){
      case SANDBOX_TEST_OS_VER:{
            uint32_t verStrAddr = callFunction(false, 0x00000000, SysGetOSVersionString, "p()");
            char* nativeStr = makeNativeString(verStrAddr);

            debugLog("Sandbox: OS version is:\"%s\"\n", nativeStr);
            free(nativeStr);
         }
         break;

      case SANDBOX_SEND_OS_TOUCH:{
            //this will not be in the v1.0 release
            if(!m68k_read_memory_8(0x00000253)){
               //0x00000253 seems to be a mutex for accessing the event queue
               //the hack only seems to work when in an interrupt, a hack for a hack :(
               m68k_set_irq(5);

               if(palmInput.touchscreenTouched){
                  //press
                  m68k_write_memory_16(0xFFFFFFE0 - 4, palmInput.touchscreenX);
                  m68k_write_memory_16(0xFFFFFFE0 - 2, palmInput.touchscreenY);
                  callFunction(false, 0x00000000, PenScreenToRaw, "w(p)", 0xFFFFFFE0 - 4);
                  callFunction(false, 0x00000000, EvtEnqueuePenPoint, "w(p)", 0xFFFFFFE0 - 4);
               }
               else{
                  //release
                  m68k_write_memory_16(0xFFFFFFE0 - 4, (uint16_t)-1);
                  m68k_write_memory_16(0xFFFFFFE0 - 2, (uint16_t)-1);
                  callFunction(false, 0x00000000, PenScreenToRaw, "w(p)", 0xFFFFFFE0 - 4);
                  callFunction(false, 0x00000000, EvtEnqueuePenPoint, "w(p)", 0xFFFFFFE0 - 4);
               }
            }
         }
         break;

      case SANDBOX_PATCH_OS:{
            //this will not be in the v1.0 release
            //remove parts of the OS that cause lockups, yeah its bad

            //remove ErrDisplayFileLineMsg from HwrIRQ2Handler, device locks on USB polling without this
            palmRom[0x83652] = 0x4E;
            palmRom[0x83653] = 0x71;
            palmRom[0x83654] = 0x4E;
            palmRom[0x83655] = 0x71;
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

   debugLog("Sandbox: Test %d finished\n", test);

   return result;
}

void sandboxOnOpcodeRun(){
#if defined(EMU_SANDBOX_LOG_APIS)
   logApiCalls();
#endif
   switch(m68k_get_reg(NULL, M68K_REG_PPC)){
      //case 0x10083652://USB issue location
      case 0x100846C0://HwrDelay, before mysterious jump
      case 0x100846CC://HwrDelay, after mysterious jump
         //to add a emulator breakpoint add a new line above here|^^^
         {
            uint32_t m68kRegisters[M68K_REG_CPU_TYPE];

            for(uint8_t count = 0; count < M68K_REG_CPU_TYPE; count++)
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
            bool breakHere = true;
            //set host breakpoint here|^^^
         }
         break;
   }
}

bool sandboxRunning(){
   return inSandbox;
}

void sandboxReturn(){
   inSandbox = false;
}

#else
void sandboxInit(){}
uint32_t sandboxCommand(uint32_t test, void* data){return 0;}
void sandboxOnOpcodeRun(){}
bool sandboxRunning(){return false;}
void sandboxReturn(){}
#endif
