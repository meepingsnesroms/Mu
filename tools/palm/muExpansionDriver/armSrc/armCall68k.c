#include <Standalone.h>

#include <stdint.h>


STANDALONE_CODE_RESOURCE_ID(1);


void start(void){
   register uint32_t r1 asm("r1");
   register uint32_t r2 asm("r2");
   register uint32_t r3 asm("r3");
   register uint32_t r4 asm("r4");
   register uint32_t r5 asm("r5");
   register uint32_t r6 asm("r6");
   register uint32_t r7 asm("r7");
   
   r4 = 1;
   r5 = r1;
   r6 = r2;
   r7 = r3;
   
   /*hypercall*/
   asm(".long 0xF7BBBBBB");
   /*return value of the 68k call is put in r0 by the PceNativeCall implementation*/
}
