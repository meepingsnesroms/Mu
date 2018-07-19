#pragma once

enum{
   SANDBOX_TEST_OS_VER = 0,
   SANDBOX_TEST_HWR_ADC
};

void sandboxTest(uint32_t test);
void sandboxReturn();//should only be called called by 68k code
