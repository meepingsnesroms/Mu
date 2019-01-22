#ifndef PATCHER_H
#define PATCHER_H

#include <stdint.h>

void initBoot(uint32_t* configFile);
void reinit(uint32_t* configFile);

#endif
