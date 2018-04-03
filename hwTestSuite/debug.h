#ifndef DEBUG_HEADER
#define DEBUG_HEADER

#include "ugui.h"

#ifdef DEBUG
void setDebugTag(char* tag);
void debugSafeScreenClear(UG_COLOR c);
#else
#define setDebugTag(x)
#define debugSafeScreenClear(x) UG_FillScreen(x)
#endif

#endif


