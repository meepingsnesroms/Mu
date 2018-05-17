#include <PalmOS.h>
#include <stdint.h>

#include "testSuite.h"
#include "specs/hardwareRegisterNames.h"
#include "irda.h"
#include "cpu.h"


/*the OS doesnt seem to use timer 2(at least on Sony Clie PEG-SL10) since it was added after*/
/*the OS was written since it was originally for the Dragonball 328,*/
/*this makes it perfect for using as a hardware thread*/


static Boolean vncRunning = false;
static void (*palmOsIntLevel6Handler)();


static void vncInterruptHandler(){
   /*called when VNC is enabled and a level 6 interrupt occurs*/
   if(readArbitraryMemory16(HW_REG_ADDR(TSTAT2))){
      /*the interrupt was called by timer 2*/
      irdaHandleCommands();
      
      /*clear timer 2 interrupt, it must be read before writing it is possible but that is performed by the check above*/
      writeArbitraryMemory16(HW_REG_ADDR(TSTAT2), 0x0000);
   }
   else{
      /*not timer 2, send interrupt to Palm OS*/
      palmOsIntLevel6Handler();
   }
}

Boolean vncInit(){
   if(!vncRunning){
      uint16_t ilcr;
      
      turnInterruptsOff();
      
      /*install interrupt handler*/
      palmOsIntLevel6Handler = (void*)readArbitraryMemory32(readArbitraryMemory8(HW_REG_ADDR(IVR) | 6/*int level*/) * 4);
      writeArbitraryMemory32(readArbitraryMemory8(HW_REG_ADDR(IVR) | 6/*int level*/) * 4, (uint32_t)vncInterruptHandler);
      
      /*set timer 2 interrupt priority, using level 6 because its the highest*/
      ilcr = readArbitraryMemory16(HW_REG_ADDR(ILCR));
      ilcr &= 0x7770;/*remove TMR2 field*/
      ilcr |= 0x0006;/*set new TMR2 field*/
      writeArbitraryMemory16(HW_REG_ADDR(ILCR), ilcr);
      
      /*set interrupt to trigger every 32768 / 15 seconds, 15 fps*/
      readArbitraryMemory16(HW_REG_ADDR(TSTAT2));/*clear any existing interrupt*/
      writeArbitraryMemory16(HW_REG_ADDR(TSTAT2), 0x0000);/*clear any existing interrupt*/
      writeArbitraryMemory16(HW_REG_ADDR(TPRER2), 0x0000);/*timer prescaler off*/
      writeArbitraryMemory16(HW_REG_ADDR(TCMP2), 32768 / 15);/*timer duration*/
      writeArbitraryMemory16(HW_REG_ADDR(TCTL2), 0x001F);/*turn on timer 2, using CLK32 as the source*/
      
      turnInterruptsOn();
      
      vncRunning = true;
   }
   
   return true;/*its already running, return success*/
}

void vncExit(){
   if(vncRunning){
      turnInterruptsOff();
      
      /*turn off timer 2*/
      writeArbitraryMemory16(HW_REG_ADDR(TCTL2), 0x0000);
      
      writeArbitraryMemory32(readArbitraryMemory8(HW_REG_ADDR(IVR) | 6/*int level*/) * 4, (uint32_t)palmOsIntLevel6Handler);
      
      turnInterruptsOn();
      
      vncRunning = false;
   }
}
