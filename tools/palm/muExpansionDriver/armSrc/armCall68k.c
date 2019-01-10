#include <Standalone.h>

#include <stdint.h>


STANDALONE_CODE_RESOURCE_ID(1);


unsigned long armCall68k(const void* emulStateP, unsigned long trapOrFunction, const void* argsOnStackP, unsigned long argsSizeAndwantA0){
   register uint32_t r0 asm("r0");
   register uint32_t r4 asm("r4");
   register uint32_t r5 asm("r5");
   register uint32_t r6 asm("r6");
   register uint32_t r7 asm("r7");
   
   r4 = 1;
   r5 = (uint32_t)trapOrFunction;
   r6 = (uint32_t)argsOnStackP;
   r7 = (uint32_t)argsSizeAndwantA0;
   
   /*hypercall*/
   asm(".long 0xF7BBBBBB");
   
   /*return value of the 68k call is put in r0 by the PceNativeCall implementation*/
   return r0;
   
   /*
   register uint32_t r0 asm("r0");
    
   asm("mov r4, #1");
   asm("mov r5, r1");
   asm("mov r6, r2");
   asm("mov r7, r3");
   asm(".long 0xF7BBBBBB");
   
   return r0;
   */
}
