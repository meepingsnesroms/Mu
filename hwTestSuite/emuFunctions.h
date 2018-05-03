#ifndef EMU_FUNCTIONS_HEADER
#define EMU_FUNCTIONS_HEADER

#include <PalmOS.h>
#include <stdint.h>
#include "emuFeatureRegistersSpec.h"

Boolean isEmulator();
Boolean isEmulatorFeatureEnabled(uint32_t feature);

#endif

