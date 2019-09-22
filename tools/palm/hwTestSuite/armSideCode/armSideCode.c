#include <stdint.h>


unsigned long __attribute__((used)) runTest(const void* emulStateP, void* userData68KP, /*Call68KFuncType*/void* call68KFuncP){
   uint32_t* args = (uint32_t*)userData68KP;

   switch(args[0]){

      default:
         break;
      }

   return 0;
}

void __attribute__((naked,section(".vectors"))) vecs(void){
	asm volatile(
		"B	runTest								\n\t"
	);
}
