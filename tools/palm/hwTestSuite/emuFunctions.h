#ifndef EMU_FUNCTIONS_H
#define EMU_FUNCTIONS_H

#include <PalmOS.h>
#include <stdint.h>

#include "specs/emuFeatureRegisterSpec.h"/*needed for feature names*/

Boolean isEmulator(void);
Boolean isEmulatorFeatureEnabled(uint32_t feature);

#endif

