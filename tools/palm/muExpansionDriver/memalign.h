/*modified from libretro-common*/
#ifndef MEMALIGN_H
#define MEMALIGN_H

#include <stdint.h>

void* memalign_alloc(uint32_t boundary, uint32_t size);
void memalign_free(void* ptr);

#endif
