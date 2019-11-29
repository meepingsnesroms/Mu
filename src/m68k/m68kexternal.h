#ifndef M68KEXTERNAL_HEADER
#define M68KEXTERNAL_HEADER

#include <stdint.h>

int32_t interruptAcknowledge(int32_t intLevel);
void emulatorSoftReset(void);
void flx68000PcLongJump(uint32_t newPc);

#endif
