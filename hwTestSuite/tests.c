#include "testSuite.h"
#include "tools.h"


var testFileAccessWorks(){
   uint32_t testTook = UINT32_C(0xFFFFFE00);
   makeFile((uint8_t*)&testTook, 4, "FILEOUT.BIN");
   exitSubprogram();
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}
