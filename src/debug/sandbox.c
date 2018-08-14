#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "../m68k/m68k.h"
#include "../emulator.h"
#include "../ads7846.h"
#include "../hardwareRegisters.h"
#include "../specs/emuFeatureRegistersSpec.h"
#include "sandbox.h"


#if defined(EMU_DEBUG) && defined(EMU_SANDBOX)
//this wrappers 68k code and allows calling it for tests, this should be useful for determining if hardware accesses are correct
//Note: when running a test the emulator runs at native speed and no clocks are emulated


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
      case 0xA249://sysTrapHwrDelay
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
   uint32_t rangeEnd = chips[CHIP_A_ROM].start + chips[CHIP_A_ROM].lineSize - 1;
   uint32_t address = find68kString(name, chips[CHIP_A_ROM].start, rangeEnd);

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

static uint32_t callFunction(bool fallthrough, uint32_t address, const char* name, const char* prototype, ...){
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
      uint16_t trap = reverseLookupTrap(name);

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

uint32_t makePalmString(const char* str){
   uint32_t strLength = strlen(str) + 1;
   uint32_t strData = callFunction(false, 0x00000000, "MemPtrNew", "p(l)", strLength);

   if(strData != 0)
      for(uint32_t count = 0; count < strLength; count++)
         m68k_write_memory_8(strData + count, str[count]);
   return strData;
}

char* makeNativeString(uint32_t address){
   if(address != 0){
      int16_t strLength = callFunction(false, 0x00000000, "StrLen", "w(p)", address) + 1;
      char* nativeStr = malloc(strLength);

      for(int16_t count = 0; count < strLength; count++)
         nativeStr[count] = m68k_read_memory_8(address + count);
      return nativeStr;
   }
   return NULL;
}

void freePalmString(uint32_t address){
   callFunction(false, 0x00000000, "MemChunkFree", "w(p)", address);
}

void sandboxInit(){
   inSandbox = false;
#if defined(EMU_SANDBOX_OPCODE_LEVEL_DEBUG)
   m68k_set_instr_hook_callback(sandboxOnOpcodeRun);
#endif
}

void sandboxTest(uint32_t test){
   //tests cant run properly(hang forever) unless the debug return hook is enabled, it also completly destroys accuracy to execute hacked in asm buffers
   if(!(palmSpecialFeatures & FEATURE_DEBUG))
      return;

   debugLog("Sandbox: Test %d started\n", test);

   switch(test){
      case SANDBOX_TEST_OS_VER:{
            uint32_t verStrAddr = callFunction(false, 0x00000000, "SysGetOSVersionString", "p()");
            char* nativeStr = makeNativeString(verStrAddr);

            debugLog("Sandbox: OS version is:\"%s\"\n", nativeStr);
            free(nativeStr);
         }
         break;

      case SANDBOX_TEST_TOUCH_READ:{
            //since the sandbox can interrupt any running function(including ADC ones) this safeguard is needed
            /*
            if(ads7846BitsToNextControl == 0){
               uint32_t point;
               int16_t osX;
               int16_t osY;
               input_t oldInput = palmInput;

               palmInput.touchscreenTouched = true;
               palmInput.touchscreenX = randomRange(0, 159);
               palmInput.touchscreenY = randomRange(0, 219);

               //PrvBBGetXY on self dumped ROM
               callFunction(false, 0x100827CE, NULL, "v(L)", &point);
               osX = point >> 16;
               osY = point & 0xFFFF;
               debugLog("Sandbox: Real touch position is x:%d, y:%d\n", palmInput.touchscreenX, palmInput.touchscreenY);
               debugLog("Sandbox: OS reported touch position is x:%d, y:%d\n", osX, osY);
               palmInput = oldInput;
            }
            else{
               debugLog("Sandbox: Unsafe to read touch position.\n");
            }
            */
         }
         break;

      case SANDBOX_SEND_OS_TOUCH:{
            if(!m68k_read_memory_8(0x00000253)){
               //0x00000253 seems to be a mutex for accessing the event queue
               //the hack only seems to work when in an interrupt, a hack for a hack :(
               m68k_set_irq(5);

               if(palmInput.touchscreenTouched){
                  //press
                  m68k_write_memory_16(0xFFFFFFE0 - 4, palmInput.touchscreenX);
                  m68k_write_memory_16(0xFFFFFFE0 - 2, palmInput.touchscreenY);
                  callFunction(false, 0x00000000, "PenScreenToRaw", "w(p)", 0xFFFFFFE0 - 4);
                  callFunction(false, 0x00000000, "EvtEnqueuePenPoint", "w(p)", 0xFFFFFFE0 - 4);
               }
               else{
                  //release
                  m68k_write_memory_16(0xFFFFFFE0 - 4, (uint16_t)-1);
                  m68k_write_memory_16(0xFFFFFFE0 - 2, (uint16_t)-1);
                  callFunction(false, 0x00000000, "PenScreenToRaw", "w(p)", 0xFFFFFFE0 - 4);
                  callFunction(false, 0x00000000, "EvtEnqueuePenPoint", "w(p)", 0xFFFFFFE0 - 4);
               }
            }
         }
         break;
   }

   debugLog("Sandbox: Test %d finished\n", test);
}

void sandboxOnOpcodeRun(){
#if defined(EMU_SANDBOX_LOG_APIS)
   logApiCalls();
#endif
}

bool sandboxRunning(){
   return inSandbox;
}

void sandboxReturn(){
   inSandbox = false;
}

#else
void sandboxInit(){}
void sandboxTest(uint32_t test){}
void sandboxOnOpcodeRun(){}
bool sandboxRunning(){return false;}
void sandboxReturn(){}
#endif
