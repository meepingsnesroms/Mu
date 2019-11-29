#ifndef TOOLS_H
#define TOOLS_H

#include <PalmOS.h>

#include "testSuite.h"

Err makeFile(uint8_t* data, uint32_t size, char* fileName);
uint16_t ads7846GetValue(uint8_t channel, Boolean referenceMode, Boolean mode8Bit);
var hexRamBrowser(void);
var getTrapAddress(void);
var dumpBootloaderToFile(void);
var listRomInfo(void);
var listRamInfo(void);
var listChipSelects(void);
var getTouchscreenLut(void);

#endif
