/*modified from libretro-common*/
#include "sdkPatch/PalmOSPatched.h"
#include <stdint.h>


void* memalign_alloc(uint32_t boundary, uint32_t size){
   void** place = NULL;
   uint32_t addr = 0;
   void* ptr = (void*)MemPtrNew(boundary + size + sizeof(uint32_t));
   
   if(!ptr)
      return NULL;

   addr = ((uint32_t)ptr + sizeof(uint32_t) + boundary) & ~(boundary - 1);
   place = (void**)addr;
   place[-1] = ptr;

   return (void*)addr;
}

void memalign_free(void* ptr){
   void** p = NULL;
   
   if(!ptr)
      return;

   p = (void**)ptr;
   free(p[-1]);
}
