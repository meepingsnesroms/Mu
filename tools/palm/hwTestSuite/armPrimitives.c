#include <PalmOS.h>
#include "PceNativeCall.h"
#include "ByteOrderUtils.h"
#include <stdint.h>

#include "testSuite.h"


uint8_t armRead8(uint32_t address){
   /*//LDR R0, [PC, 4] //LDRB R0, [R0] //MOV PC, LR //DATA*/
   static ALIGN(4) uint32_t getValue[] = {0x04009FE5, 0x0000D0E5, 0x0EF0A0E1, 0x00000000};
   
   getValue[3] = EndianSwap32(address);
   return PceNativeCall(getValue, NULL) & 0xFF;
}

uint16_t armRead16(uint32_t address){
   /*//LDR R0, [PC, 4] //LDRH R0, [R0] //MOV PC, LR //DATA*/
   static ALIGN(4) uint32_t getValue[] = {0x04009FE5, 0xB000D0E1, 0x0EF0A0E1, 0x00000000};
   
   getValue[3] = EndianSwap32(address);
   return PceNativeCall(getValue, NULL) & 0xFFFF;
}

uint32_t armRead32(uint32_t address){
   /*//LDR R0, [PC, 4] //LDR R0, [R0] //MOV PC, LR //DATA*/
   static ALIGN(4) uint32_t getValue[] = {0x04009FE5, 0x000090E5, 0x0EF0A0E1, 0x00000000};
   
   getValue[3] = EndianSwap32(address);
   return PceNativeCall(getValue, NULL);
}

void armWrite8(uint32_t address, uint8_t value){
   /*//PUSH {R1} //LDR R0, [PC, 12] //LDR R1, [PC, 12] //STRB R1, [R0] //POP{R1} //MOV PC, LR //DATA //DATA*/
   static ALIGN(4) uint32_t setValue[] = {0x04102DE5, 0x0C009FE5, 0x0C109FE5, 0x0010C0E5, 0x04109DE4, 0x0EF0A0E1, 0x00000000, 0x00000000};
   
   setValue[6] = EndianSwap32(address);
   setValue[7] = EndianSwap32(value);
   PceNativeCall(setValue, NULL);
}

void armWrite16(uint32_t address, uint16_t value){
   /*//PUSH {R1} //LDR R0, [PC, 12] //LDR R1, [PC, 12] //STRH R1, [R0] //POP{R1} //MOV PC, LR //DATA //DATA*/
   static ALIGN(4) uint32_t setValue[] = {0x04102DE5, 0x0C009FE5, 0x0C109FE5, 0xB010C0E1, 0x04109DE4, 0x0EF0A0E1, 0x00000000, 0x00000000};
   
   setValue[6] = EndianSwap32(address);
   setValue[7] = EndianSwap32(value);
   PceNativeCall(setValue, NULL);
}

void armWrite32(uint32_t address, uint32_t value){
   /*//PUSH {R1} //LDR R0, [PC, 12] //LDR R1, [PC, 12] //STR R1, [R0] //POP{R1} //MOV PC, LR //DATA //DATA*/
   static ALIGN(4) uint32_t setValue[] = {0x04102DE5, 0x0C009FE5, 0x0C109FE5, 0x001080E5, 0x04109DE4, 0x0EF0A0E1, 0x00000000, 0x00000000};
   
   setValue[6] = EndianSwap32(address);
   setValue[7] = EndianSwap32(value);
   PceNativeCall(setValue, NULL);
}
