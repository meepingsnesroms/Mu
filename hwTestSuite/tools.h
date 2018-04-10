#ifndef TOOLS_HEADER
#define TOOLS_HEADER

#include <PalmOS.h>
#include "testSuite.h"

Err makeFile(uint8_t* data, uint32_t size, char* fileName);
var hexRamBrowser();
var getTrapAddress();
var manualLssa();
var dumpBootloaderToFile();
var toggleUnsafeMode();
#endif
