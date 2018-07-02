#ifndef UNDOCUMENTED_APIS_HEADER
#define UNDOCUMENTED_APIS_HEADER

#include <PalmOS.h>
#include <stdint.h>

uint16_t (*customCall_HwrADC)(uint16_t mode, void* returnData);

Boolean initUndocumentedApiHandlers();

#endif
