#ifndef TOOLS_HEADER
#define TOOLS_HEADER

#include "testSuite.h"

uint32_t makeFile(uint8_t* data, uint32_t size, char* fileName);
var hexRamBrowser();
var dumpBootloaderToFile();
var testFileAccessWorks();
#endif
