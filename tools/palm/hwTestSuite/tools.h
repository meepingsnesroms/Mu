#ifndef TOOLS_H
#define TOOLS_H

#include <PalmOS.h>

#include "testSuite.h"

Err makeFile(uint8_t* data, uint32_t size, char* fileName);
uint16_t ads7846GetValue(uint8_t channel, Boolean referenceMode, Boolean mode8Bit);
var hexRamBrowser();
var getTrapAddress();
var manualLssa();
var dumpBootloaderToFile();
var listRomInfo();
var listRamInfo();
var listChipSelects();
var getTouchscreenLut();

#endif
