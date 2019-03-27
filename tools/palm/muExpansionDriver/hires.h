#ifndef HIRES_H
#define HIRES_H

#include "sdkPatch/PalmOSPatched.h"
#include <stdint.h>

Boolean installTungstenWLcdDrivers(void);
Boolean setDeviceResolution(uint16_t width, uint16_t height);
void screenSignalApplicationStart(void);
void screenSignalApplicationExit(void);

#endif
