#ifndef TOOLS_HEADER
#define TOOLS_HEADER

#include <PalmOS.h>
#include "testSuite.h"

Err makeFile(uint8_t* data, uint32_t size, char* fileName);
uint16_t ads7846GetValue(uint8_t channel, Boolean referenceMode, Boolean mode8bit);
var hexRamBrowser();
var getTrapAddress();
var manualLssa();
var dumpBootloaderToFile();

#endif
