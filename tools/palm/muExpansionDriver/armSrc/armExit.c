#include <Standalone.h>

#include <stdint.h>


STANDALONE_CODE_RESOURCE_ID(0);


void start(void){
   register uint32_t r4 asm("r4");
   
   r4 = 0;
   
   /*hypercall*/
   asm(".long 0xF7BBBBBB");
}
