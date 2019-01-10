#include <Standalone.h>

#include <stdint.h>


STANDALONE_CODE_RESOURCE_ID(0);


void armExit(void){
#if 0
   register uint32_t r4 asm("r4");
   
   r4 = 0;
      
   /*hypercall*/
   asm(".long 0xF7BBBBBB");
#endif
   asm("mov r4, #0");
   asm(".long 0xF7BBBBBB");
}
