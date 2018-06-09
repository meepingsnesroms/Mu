#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "emulator.h"
#include "specs/hardwareRegisterNames.h"
#include "hardwareRegisters.h"
#include "memoryAccess.h"
#include "68328Functions.h"
#include "portability.h"
#include "ads7846.h"
#include "m68k/m68k.h"


chip_t   chips[CHIP_END];
int32_t  pllWakeWait;
uint32_t clk32Counter;
double   timerCycleCounter[2];
uint16_t timerStatusReadAcknowledge[2];
uint32_t edgeTriggeredInterruptLastValue;
uint16_t spi1RxFifo[8];
uint16_t spi1TxFifo[8];
uint8_t  spi1RxPosition;
uint8_t  spi1TxPosition;
//bool     spi1RxFifoOverflow;//may not be needed, just dont send any data unless FIFO has a slot


bool pllIsOn();
static void checkInterrupts();
static void checkPortDInterrupts();
static void recalculateCpuSpeed();

#include "hardwareRegistersAccessors.c.h"
#include "hardwareRegistersTiming.c.h"

bool pllIsOn(){
   return !(registerArrayRead16(PLLCR) & 0x0008);
}

bool registersAreXXFFMapped(){
   return registerArrayRead8(SCR) & 0x04;
}

bool sed1376ClockConnected(){
   //this is the clock output pin for the SED1376, if its disabled so is the LCD controller
   //port f pin 2 is not GPIO and PLLCR CLKEN is false enabling clock output on port f pin 2
   return !(registerArrayRead8(PFSEL) & 0x04) && !(registerArrayRead16(PLLCR) & 0x0010);
}

void refreshInputState(){
   uint16_t icr = registerArrayRead16(ICR);
   bool penIrqPin = !(ads7846PenIrqEnabled && palmInput.touchscreenTouched);//penIrqPin pulled low on touch

   //IRQ set as pin function and triggered
   if(!(registerArrayRead8(PFSEL) & 0x02) && penIrqPin == (bool)(icr & 0x0080))
      setIprIsrBit(INT_IRQ5);

   /*
   //IRQ set as pin function and triggered, the pen IRQ triggers when going low to high or high to low
   if(!(registerArrayRead8(PFSEL) & 0x02) && (penIrqPin == (bool)(icr & 0x0080)) != (bool)(edgeTriggeredInterruptLastValue & INT_IRQ5))
      setIprIsrBit(INT_IRQ5);

   if(penIrqPin == (bool)(icr & 0x0080))
      edgeTriggeredInterruptLastValue |= INT_IRQ5;
   else
      edgeTriggeredInterruptLastValue &= ~INT_IRQ5;
   */

   checkPortDInterrupts();//this calls checkInterrupts() so it doesnt need to be called above
}

int32_t interruptAcknowledge(int32_t intLevel){
   uint8_t vectorOffset = registerArrayRead8(IVR);
   int32_t vector;

   //If an interrupt occurs before the IVR has been programmed, interrupt vector 15 is returned to the CPU as an uninitialized interrupt.
   if(!vectorOffset)
      vector = 15;//EXCEPTION_UNINITIALIZED_INTERRUPT
   else
      vector = vectorOffset | intLevel;

   lowPowerStopActive = false;
   registerArrayWrite8(PCTLR, registerArrayRead8(PCTLR) & 0x1F);
   recalculateCpuSpeed();

   //the interrupt should only be cleared after its been handled

   return vector;
}

void setBusErrorTimeOut(){
   uint8_t scr = registerArrayRead8(SCR);
   debugLog("Bus error timeout, PC:0x%08X\n", m68k_get_reg(NULL, M68K_REG_PPC));
   if(scr & 0x10){
      //trigger bus error interrupt
   }
   registerArrayWrite8(SCR, scr | 0x80);
}

void setWriteProtectViolation(){
   uint8_t scr = registerArrayRead8(SCR);
   debugLog("Write protect violation, PC:0x%08X\n", m68k_get_reg(NULL, M68K_REG_PPC));
   if(scr & 0x10){
      //trigger bus error interrupt
   }
   registerArrayWrite8(SCR, scr | 0x40);
}

void setPrivilegeViolation(){
   uint8_t scr = registerArrayRead8(SCR);
   debugLog("Privilege violation, PC:0x%08X\n", m68k_get_reg(NULL, M68K_REG_PPC));
   if(scr & 0x10){
      //trigger bus error interrupt
   }
   registerArrayWrite8(SCR, scr | 0x20);
}

static void recalculateCpuSpeed(){
   double newCpuSpeed;

   if(pllIsOn()){
      uint16_t pllfsr = registerArrayRead16(PLLFSR);
      uint8_t pctlr = registerArrayRead8(PCTLR);
      double p = pllfsr & 0x00FF;
      double q = (pllfsr & 0x0F00) >> 8;
      newCpuSpeed = 2.0 * (14.0 * (p + 1.0) + q + 1.0);

      //prescaler 1
      if(registerArrayRead16(PLLCR) & 0x0080)
         newCpuSpeed /= 2.0;

      //power control burst mode
      if(pctlr & 0x80)
         newCpuSpeed *= (double)(pctlr & 0x1F) / 31.0;
   }
   else{
      newCpuSpeed = 0.0;
   }

   if(newCpuSpeed != palmCrystalCycles){
      debugLog("New CPU frequency of:%f cycles per second.\n", newCpuSpeed * CRYSTAL_FREQUENCY);
      debugLog("New CLK32 cycle count of:%f.\n", newCpuSpeed);
      if(newCpuSpeed == 0.0){
         if(pllIsOn())
            debugLog("CPU turned off with PCTLR.\n");
         else
            debugLog("CPU turned off with DISPLL bit.\n");
      }
   }

   palmCrystalCycles = newCpuSpeed;
}

static inline void pllWakeCpuIfOff(){
   if(!pllIsOn() && pllWakeWait == -1){
      //PLL is off and not already in the process of waking up
      switch(registerArrayRead16(PLLCR) & 0x0003){
         case 0x0000:
            pllWakeWait = 32;
            break;

         case 0x0001:
            pllWakeWait = 48;
            break;

         case 0x0002:
            pllWakeWait = 64;
            break;

         case 0x0003:
            pllWakeWait = 96;
            break;
      }
   }
}

static void checkInterrupts(){
   uint32_t activeInterrupts = registerArrayRead32(ISR);
   uint16_t interruptLevelControlRegister = registerArrayRead16(ILCR);
   uint8_t intLevel = 0;

   if(activeInterrupts & INT_EMIQ){
      //EMIQ - Emulator IRQ, has nothing to do with emulation, used for debugging on a dev board
      intLevel = 7;
   }

   if(activeInterrupts & INT_SPI1){
      uint8_t spi1IrqLevel = interruptLevelControlRegister >> 12;
      if(intLevel < spi1IrqLevel)
         intLevel = spi1IrqLevel;
   }

   if(activeInterrupts & INT_IRQ5){
      if(intLevel < 5)
         intLevel = 5;
   }

   if(activeInterrupts & INT_IRQ3){
      if(intLevel < 3)
         intLevel = 3;
   }

   if(activeInterrupts & INT_IRQ2){
      if(intLevel < 2)
         intLevel = 2;
   }

   if(activeInterrupts & INT_IRQ1){
      if(intLevel < 1)
         intLevel = 1;
   }

   if(activeInterrupts & INT_PWM2){
      uint8_t pwm2IrqLevel = interruptLevelControlRegister >> 4 & 0x0007;
      if(intLevel < pwm2IrqLevel)
         intLevel = pwm2IrqLevel;
   }

   if(activeInterrupts & INT_UART2){
      uint8_t uart2IrqLevel = interruptLevelControlRegister >> 8 & 0x0007;
      if(intLevel < uart2IrqLevel)
         intLevel = uart2IrqLevel;
   }

   if(activeInterrupts & INT_TMR2){
      //TMR2 - Timer 2
      uint8_t timer2IrqLevel = interruptLevelControlRegister & 0x0007;
      if(intLevel < timer2IrqLevel)
         intLevel = timer2IrqLevel;
   }

   if(activeInterrupts & (INT_TMR1 | INT_PWM1 | INT_IRQ6)){
      //All Fixed Level 6 Interrupts
      if(intLevel < 6)
         intLevel = 6;
   }

   if(activeInterrupts & (INT_SPI2 | INT_UART1 | INT_WDT | INT_RTC | INT_KB | INT_RTI | INT_INT0 | INT_INT1 | INT_INT2 | INT_INT3)){
      //All Fixed Level 4 Interrupts
      if(intLevel < 4)
         intLevel = 4;
   }

   //some interrupts should probably be auto cleared after being run once, RTI, RTC, WATCHDOG and SPI1/2 seem like they should be cleared this way

   pllWakeCpuIfOff();
   m68k_set_irq(intLevel);//should be called even if intLevel is 0, that is how the interrupt state gets cleared
}

static void checkPortDInterrupts(){
   uint8_t portDValue = getPortDValue();
   uint8_t portDDir = registerArrayRead8(PDDIR);
   uint8_t portDIntEnable = registerArrayRead8(PDIRQEN);
   uint8_t portDKeyboardEnable = registerArrayRead8(PDKBEN);
   uint8_t portDIrqPins = ~registerArrayRead8(PDSEL);
   uint16_t portDEdgeSelect = registerArrayRead16(PDIRQEG);
   uint16_t interruptControlRegister = registerArrayRead16(ICR);
   bool currentPllState = pllIsOn();

   if(portDIntEnable & portDValue & ~portDDir & 0x01){
      //int 0, polarity set with PDPOL
      if(!(portDEdgeSelect & 0x01)){
         //edge triggered
         if(!(edgeTriggeredInterruptLastValue & INT_INT0) && currentPllState)
            setIprIsrBit(INT_INT0);
      }
      else{
         //level triggered
         setIprIsrBit(INT_INT0);
      }

      edgeTriggeredInterruptLastValue |= INT_INT0;
   }
   else{
      edgeTriggeredInterruptLastValue &= ~INT_INT0;
   }
   
   if(portDIntEnable & portDValue & ~portDDir & 0x02){
      //int 1, polarity set with PDPOL
      if(!(portDEdgeSelect & 0x02) || pllIsOn())
         setIprIsrBit(INT_INT1);
   }
   
   if(portDIntEnable & portDValue & ~portDDir & 0x04){
      //int 2, polarity set with PDPOL
      if(!(portDEdgeSelect & 0x04) || pllIsOn())
         setIprIsrBit(INT_INT2);
   }
   
   if(portDIntEnable & portDValue & ~portDDir & 0x08){
      //int 3, polarity set with PDPOL
      if(!(portDEdgeSelect & 0x08) || pllIsOn())
         setIprIsrBit(INT_INT3);
   }
   
   if(portDIrqPins & ~portDDir & 0x10 && (bool)(portDValue & 0x10) == (bool)(interruptControlRegister & 0x8000)){
      //irq 1, polarity set in ICR
      setIprIsrBit(INT_IRQ1);
   }
   
   if(portDIrqPins & ~portDDir & 0x20 && (bool)(portDValue & 0x20) == (bool)(interruptControlRegister & 0x4000)){
      //irq 2, polarity set in ICR
      setIprIsrBit(INT_IRQ2);
   }
   
   if(portDIrqPins & ~portDDir & 0x40 && (bool)(portDValue & 0x40) == (bool)(interruptControlRegister & 0x2000)){
      //irq 3, polarity set in ICR
      setIprIsrBit(INT_IRQ3);
   }
   
   if(portDIrqPins & ~portDDir & 0x80 && (bool)(portDValue & 0x80) == (bool)(interruptControlRegister & 0x1000)){
      //irq 6, polarity set in ICR
      setIprIsrBit(INT_IRQ6);
   }
   
   if(portDKeyboardEnable & ~portDValue & ~portDDir){
      //active low/off level triggered interrupt
      setIprIsrBit(INT_KB);
   }
   
   checkInterrupts();
}

static inline void updateAlarmLedStatus(){
   if(registerArrayRead8(PBDATA) & registerArrayRead8(PBSEL) & registerArrayRead8(PBDIR) & 0x40)
      palmMisc.alarmLed = true;
   else
      palmMisc.alarmLed = false;
}

static inline void updateVibratorStatus(){
   if(registerArrayRead8(PKDATA) & registerArrayRead8(PKSEL) & registerArrayRead8(PKDIR) & 0x10)
      palmMisc.vibratorOn = true;
   else
      palmMisc.vibratorOn = false;
}

void printUnknownHwAccess(uint32_t address, uint32_t value, uint32_t size, bool isWrite){
   if(isWrite)
      debugLog("CPU wrote %d bits of 0x%08X to register 0x%03X, PC 0x%08X.\n", size, value, address, m68k_get_reg(NULL, M68K_REG_PPC));
   else
      debugLog("CPU read %d bits from register 0x%03X, PC 0x%08X.\n", size, address, m68k_get_reg(NULL, M68K_REG_PPC));
}

static uint32_t getEmuRegister(uint32_t address){
   address &= 0xFFF;
   switch(address){

      default:
         debugLog("Invalid read from emu register 0x%08X.\n", address);
         break;
   }

   return 0x00000000;
}

static void setEmuRegister(uint32_t address, uint32_t value){
   address &= 0xFFF;
   switch(address){
      case EMU_CMD:
         if(value >> 16 == EMU_CMD_KEY){
            value &= 0xFFFF;
            switch(value){
               case CMD_EXECUTION_DONE:
#if defined(EMU_DEBUG)
                  if(palmSpecialFeatures & FEATURE_DEBUG)
                     executionFinished = true;
#endif
                  break;

               default:
                  debugLog("Invalid emu command 0x%04X.\n", value);
                  break;
            }
         }
         break;

      default:
         debugLog("Invalid write 0x%08X to emu register 0x%08X.\n", value, address);
         break;
   }
}

uint8_t getHwRegister8(uint32_t address){
   //not emu or hardware register, invalid access
   if((address & 0x0000F000) != 0x0000F000)
      return 0x00;
   
   address &= 0x00000FFF;
#if defined(EMU_DEBUG) && defined(EMU_LOG_REGISTER_ACCESS_ALL)
   printUnknownHwAccess(address, 0, 8, false);
#endif
   switch(address){
      case PADATA:
         //read outputs as is and inputs as true, floating pins are high
         return (registerArrayRead8(PADATA) & registerArrayRead8(PADIR)) | ~registerArrayRead8(PADIR);

      case PBDATA:
         //read outputs as is and inputs as true, floating pins are high
         return (registerArrayRead8(PBDATA) & registerArrayRead8(PBDIR)) | ~registerArrayRead8(PBDIR);

      case PCDATA:
         //read outputs as is and inputs as false, floating pins are low, port c has a pull down not up
         return registerArrayRead8(PCDATA) & registerArrayRead8(PCDIR);

      case PDDATA:
         return getPortDValue();

      case PEDATA:
         //read outputs as is and inputs as true, floating pins are high
         return (registerArrayRead8(PEDATA) & registerArrayRead8(PEDIR)) | ~registerArrayRead8(PEDIR);

      case PFDATA:
         //read outputs as is and inputs as true, floating pins are high
         return (registerArrayRead8(PFDATA) & registerArrayRead8(PFDIR)) | ~registerArrayRead8(PFDIR);

      case PGDATA:
         //read outputs as is and inputs as true, floating pins are high
         return (registerArrayRead8(PGDATA) & registerArrayRead8(PGDIR)) | ~registerArrayRead8(PGDIR);

      case PJDATA:
         //read outputs as is and inputs as true, floating pins are high
         return (registerArrayRead8(PJDATA) & registerArrayRead8(PJDIR)) | ~registerArrayRead8(PJDIR);

      case PKDATA:
         return getPortKValue();

      case PMDATA:
         //read outputs as is and inputs as true, floating pins are high
         return (registerArrayRead8(PMDATA) & registerArrayRead8(PMDIR)) | ~registerArrayRead8(PMDIR);

      //basic non GPIO functions
      case LCKCON:
      case IVR:

      //port d special functions
      case PDPOL:
      case PDIRQEN:
      case PDIRQEG:
      case PDKBEN:
         
      //I/O direction
      case PBDIR:
      case PDDIR:
      case PEDIR:
      case PFDIR:
      case PJDIR:
      case PKDIR:

      //select between GPIO or special function
      case PBSEL:
      case PCSEL:
      case PDSEL:
      case PESEL:
      case PFSEL:
      case PGSEL:
      case PJSEL:
      case PKSEL:
      case PMSEL:
         
      //pull up/down enable
      case PAPUEN:
      case PBPUEN:
      case PCPDEN:
      case PDPUEN:
      case PEPUEN:
      case PFPUEN:
      case PGPUEN:
      case PJPUEN:
      case PKPUEN:
      case PMPUEN:
         //simple read, no actions needed
         //PGPUEN, PGSEL PMSEL and PMPUEN lack the top 2 bits but that is handled on write
         //PDSEL lacks the bottom 4 bits but that is handled on write
         return registerArrayRead8(address);
         
      default:
         //bootloader
         if(address >= 0xE00)
            return registerArrayRead8(address);
#if defined(EMU_DEBUG) && defined(EMU_LOG_REGISTER_ACCESS_UNKNOWN) && !defined(EMU_LOG_REGISTER_ACCESS_ALL)
         else
            printUnknownHwAccess(address, 0, 8, false);
#endif
         return 0x00;
   }
   
   return 0x00;//silence warnings
}

uint16_t getHwRegister16(uint32_t address){
   //not emu or hardware register, invalid access
   if((address & 0x0000F000) != 0x0000F000)
      return 0x0000;
   
   address &= 0x00000FFF;
#if defined(EMU_DEBUG) && defined(EMU_LOG_REGISTER_ACCESS_ALL)
   printUnknownHwAccess(address, 0, 16, false);
#endif
   switch(address){
      case TSTAT1:
         timerStatusReadAcknowledge[0] |= registerArrayRead16(TSTAT1);//active bits acknowledged
         return registerArrayRead16(TSTAT1);

      case TSTAT2:
         timerStatusReadAcknowledge[1] |= registerArrayRead16(TSTAT2);//active bits acknowledged
         return registerArrayRead16(TSTAT2);

      case PWMC1:
         return getPwmc1();
         
      //32 bit registers accessed as 16 bit
      case IMR:
      case IMR + 2:
      case IPR:
      case IPR + 2:
      case ISR:
      case ISR + 2:
         
      case CSA:
      case CSB:
      case CSC:
      case CSD:
      case CSGBA:
      case CSGBB:
      case CSGBC:
      case CSGBD:
      case CSUGBA:
      case PLLCR:
      case PLLFSR:
      case DRAMC:
      case SDCTRL:
      case RTCISR:
      case RTCCTL:
      case RTCIENR:
      case ILCR:
      case ICR:
      case TCMP1:
      case TCMP2:
      case TPRER1:
      case TPRER2:
      case TCTL1:
      case TCTL2:
      case SPICONT2:
      case SPIDATA2:
         //simple read, no actions needed
         return registerArrayRead16(address);
         
      default:
         //bootloader
         if(address >= 0xE00)
            return registerArrayRead16(address);
#if defined(EMU_DEBUG) && defined(EMU_LOG_REGISTER_ACCESS_UNKNOWN) && !defined(EMU_LOG_REGISTER_ACCESS_ALL)
         else
            printUnknownHwAccess(address, 0, 16, false);
#endif
         return 0x0000;
   }
   
   return 0x0000;//silence warnings
}

uint32_t getHwRegister32(uint32_t address){
   //32 bit emu register read, valid
   if((address & 0x0000F000) != 0x0000F000){
      if((address & 0x0000F000) == 0x0000E000)
         return getEmuRegister(address);
      return 0x00000000;
   }
   
   address &= 0x00000FFF;
#if defined(EMU_DEBUG) && defined(EMU_LOG_REGISTER_ACCESS_ALL)
   printUnknownHwAccess(address, 0, 32, false);
#endif
   switch(address){
      //16 bit registers being read as 32 bit
      case PLLFSR:

      case ISR:
      case IPR:
      case IMR:
      case RTCTIME:
      case IDR:
         //simple read, no actions needed
         return registerArrayRead32(address);
         
      default:
         //bootloader
         if(address >= 0xE00)
            return registerArrayRead32(address);
#if defined(EMU_DEBUG) && defined(EMU_LOG_REGISTER_ACCESS_UNKNOWN) && !defined(EMU_LOG_REGISTER_ACCESS_ALL)
         else
            printUnknownHwAccess(address, 0, 32, false);
#endif
         return 0x00000000;
   }
   
   return 0x00000000;//silence warnings
}


void setHwRegister8(uint32_t address, uint8_t value){
   //not emu or hardware register, invalid access
   if((address & 0x0000F000) != 0x0000F000)
      return;
   
   address &= 0x00000FFF;
#if defined(EMU_DEBUG) && defined(EMU_LOG_REGISTER_ACCESS_ALL)
   printUnknownHwAccess(address, value, 8, true);
#endif
   switch(address){ 
      case SCR:
         setScr(value);
         break;

      case PCTLR:
         registerArrayWrite8(address, value & 0x9F);
         recalculateCpuSpeed();
         break;
         
      case IVR:
         //write without the bottom 3 bits
         registerArrayWrite8(address, value & 0xF8);
         break;
         
      case PBSEL:
      case PBDIR:
      case PBDATA:
         registerArrayWrite8(address, value);
         updateAlarmLedStatus();
         break;
         
      case PDSEL:
         //write without the bottom 4 bits
         registerArrayWrite8(address, value & 0xF0);
         checkPortDInterrupts();
         break;

      case PDPOL:
      case PDIRQEN:
      case PDIRQEG:
         //write without the top 4 bits
         registerArrayWrite8(address, value & 0x0F);
         checkPortDInterrupts();
         break;
         
      case PFSEL:
         //this is the clock output pin for the SED1376, if its disabled so is the LCD controller
         setSed1376Attached(!(value & 0x04));
         registerArrayWrite8(PFSEL, value);
         break;
         
      case PGSEL:
      case PGDIR:
      case PGDATA:
         //port g also does SPI stuff, unemulated so far
         //write without the top 2 bits
         registerArrayWrite8(address, value & 0x3F);
         break;
         
      case PKSEL:
      case PKDIR:
      case PKDATA:
         registerArrayWrite8(address, value);
         checkPortDInterrupts();
         updateVibratorStatus();
         break;
      
      case PMSEL:
      case PMDIR:
      case PMDATA:
         //unemulated
         //infrared shutdown
         registerArrayWrite8(address, value & 0x3F);
         break;
         
      case PMPUEN:
      case PGPUEN:
         //write without the top 2 bits
         registerArrayWrite8(address, value & 0x3F);
         break;
         
      //select between GPIO or special function
      case PCSEL:
      case PESEL:
      case PJSEL:
      
      //direction select
      case PADIR:
      case PCDIR:
      case PDDIR:
      case PEDIR:
      case PFDIR:
      case PJDIR:
      
      //pull up/down enable
      case PAPUEN:
      case PBPUEN:
      case PCPDEN:
      case PDPUEN:
      case PEPUEN:
      case PFPUEN:
      case PJPUEN:
      case PKPUEN:
         
      //port data value, nothing known is attached to port
      case PCDATA:
      case PDDATA:
      case PEDATA:
      case PFDATA:
      case PJDATA:
         
      //misc port config
      case PDKBEN:
         
      //dragonball LCD controller, not attached to anything in Palm m515
      case LCKCON:
         //simple write, no actions needed
         registerArrayWrite8(address, value);
         break;
         
      default:
         //writeable bootloader region
         if(address >= 0xFC0)
            registerArrayWrite32(address, value);
#if defined(EMU_DEBUG) && defined(EMU_LOG_REGISTER_ACCESS_UNKNOWN) && !defined(EMU_LOG_REGISTER_ACCESS_ALL)
         else
            printUnknownHwAccess(address, value, 8, true);
#endif
         break;
   }
}

void setHwRegister16(uint32_t address, uint16_t value){
   //not emu or hardware register, invalid access
   if((address & 0x0000F000) != 0x0000F000)
      return;
   
   address &= 0x00000FFF;
#if defined(EMU_DEBUG) && defined(EMU_LOG_REGISTER_ACCESS_ALL)
   printUnknownHwAccess(address, value, 16, true);
#endif
   switch(address){  
      case RTCIENR:
         //missing bits 6 and 7
         registerArrayWrite16(address, value & 0xFF3F);
         break;

      case RTCCTL:
         registerArrayWrite16(address, value & 0x000A);
         break;
         
      case IMR:
         //this is a 32 bit register but Palm OS writes it as 16 bit chunks
         registerArrayWrite16(address, value & 0x00FF);
         break;
      case IMR + 2:
         //this is a 32 bit register but Palm OS writes it as 16 bit chunks
         registerArrayWrite16(address, value & 0x03FF);
         break;

      case ISR:
         //this is a 32 bit register but Palm OS writes it as 16 bit chunks
         registerArrayWrite16(IPR, registerArrayRead16(IPR) & ~(value & 0x001F/*external hardware int mask*/));
         registerArrayWrite16(ISR, registerArrayRead16(ISR) & ~(value & 0x001F/*external hardware int mask*/));
         checkInterrupts();
         break;
      case ISR + 2:
         //this is a 32 bit register but Palm OS writes it as 16 bit chunks
         registerArrayWrite16(IPR + 2, registerArrayRead16(IPR + 2) & ~(value & 0x0F00/*external hardware int mask*/));
         registerArrayWrite16(ISR + 2, registerArrayRead16(ISR + 2) & ~(value & 0x0F00/*external hardware int mask*/));
         checkInterrupts();
         break;
         
      case TCTL1:
      case TCTL2:
         registerArrayWrite16(address, value & 0x01FF);
         break;

      case TSTAT1:
         setTstat1(value);
         break;

      case TSTAT2:
         setTstat2(value);
         break;
         
      case WATCHDOG:
         //writing to the watchdog resets the counter bits(8 and 9) to 0
         registerArrayWrite16(address, value & 0x0083);
         break;
         
      case RTCISR:
         registerArrayWrite16(RTCISR, registerArrayRead16(RTCISR) & ~value);
         if(!(registerArrayRead16(RTCISR) & 0xFF00))
            clearIprIsrBit(INT_RTI);
         if(!(registerArrayRead16(RTCISR) & 0x003F))
            clearIprIsrBit(INT_RTC);
         checkInterrupts();
         break;
         
      case PLLFSR:
         setPllfsr(value);
         break;
         
      case PLLCR:
         //CLKEN is required for SED1376 operation
         if((value & 0x0010) != (registerArrayRead16(PLLCR) & 0x0010))
            setSed1376Attached(!(value & 0x0010));

         registerArrayWrite16(PLLCR, value & 0x3FBB);
         recalculateCpuSpeed();

         if(value & 0x0008){
            //The PLL shuts down 30 clock cycles of SYSCLK after the DISPLL bit is set in the PLLCR
            m68k_modify_timeslice(-m68k_cycles_remaining() + 30);
            debugLog("Disable PLL set, CPU off in 30 cycles!\n");
         }
         break;
         
      case ICR:
         //missing bottom 7 bits
         registerArrayWrite16(address, value & 0xFF80);
         break;

      case ILCR:
         setIlcr(value);
         break;
         
      case DRAMC:
         //unemulated
         //missing bit 7 and 6
         registerArrayWrite16(address, value & 0xFF3F);
         break;
         
      case DRAMMC:
         //unemulated
         registerArrayWrite16(address, value);
         break;
         
      case SDCTRL:
         //unemulated
         //missing bits 13, 9, 8 and 7
         registerArrayWrite16(address, value & 0xDC7F);
         break;

      case CSA:
         setCsa(value);
         resetAddressSpace();
         break;

      case CSB:
         setCsb(value);
         resetAddressSpace();
         break;

      case CSC:
         setCsc(value);
         resetAddressSpace();
         break;

      case CSD:
         setCsd(value);
         resetAddressSpace();
         break;

      case CSGBA:
         //sets the starting location of ROM(0x10000000)
         setCsgba(value);
         resetAddressSpace();
         break;

      case CSGBB:
         //sets the starting location of the SED1376(0x1FF80000)
         setCsgbb(value);
         resetAddressSpace();
         break;

      case CSGBC:
         //sets the starting location of USBPhilipsPDIUSBD12(address 0x10400000)
         //since I dont plan on adding hotsync should be fine to leave unemulated, its unemulated in pose
         setCsgbc(value);
         resetAddressSpace();
         break;

      case CSGBD:
         //sets the starting location of RAM(0x00000000)
         setCsgbd(value);
         resetAddressSpace();
         break;

      case CSUGBA:
         registerArrayWrite16(CSUGBA, value);
         //refresh all chipselect address lines
         setCsgba(registerArrayRead16(CSGBA));
         setCsgbb(registerArrayRead16(CSGBB));
         setCsgbc(registerArrayRead16(CSGBC));
         setCsgbd(registerArrayRead16(CSGBD));
         resetAddressSpace();
         break;

      case CSCTRL1:
         setCsctrl1(value);
         resetAddressSpace();
         break;

      case SPICONT2:
         setSpiCont2(value);
         break;

      case PWMC1:
         setPwmc1(value);
         break;

      case SPISPC:
         //SPI1 timing, unemulated for now

      case TCMP1:
      case TCMP2:
      case TPRER1:
      case TPRER2:
      case SPIDATA2:
         //simple write, no actions needed
         registerArrayWrite16(address, value);
         break;
         
      default:
         //writeable bootloader region
         if(address >= 0xFC0)
            registerArrayWrite16(address, value);
#if defined(EMU_DEBUG) && defined(EMU_LOG_REGISTER_ACCESS_UNKNOWN) && !defined(EMU_LOG_REGISTER_ACCESS_ALL)
         else
            printUnknownHwAccess(address, value, 16, true);
#endif
         break;
   }
}

void setHwRegister32(uint32_t address, uint32_t value){
   //32 bit emu register write, valid
   if((address & 0x0000F000) != 0x0000F000){
      if((address & 0x0000F000) == 0x0000E000)
         setEmuRegister(address, value);
      return;
   }
   
   address &= 0x00000FFF;
#if defined(EMU_DEBUG) && defined(EMU_LOG_REGISTER_ACCESS_ALL)
   printUnknownHwAccess(address, value, 32, true);
#endif
   switch(address){
      case RTCTIME:
      case RTCALRM:
         registerArrayWrite32(address, value & 0x1F3F003F);
         break;
         
      case IDR:
      case IPR:
         //write to read only register, do nothing
         break;
         
      case ISR:
         //clear ISR and IPR for external hardware whereever there is a 1 bit in value
         registerArrayWrite32(IPR, registerArrayRead32(IPR) & ~(value & 0x001F0F00/*external hardware int mask*/));
         registerArrayWrite32(ISR, registerArrayRead32(ISR) & ~(value & 0x001F0F00/*external hardware int mask*/));
         break;
         
      case IMR:
         registerArrayWrite32(address, value & 0x00FF3FFF);
         break;
         
      case LSSA:
         //simple write, no actions needed
         registerArrayWrite32(address, value);
         break;
      
      default:
         //writeable bootloader region
         if(address >= 0xFC0)
            registerArrayWrite32(address, value);
#if defined(EMU_DEBUG) && defined(EMU_LOG_REGISTER_ACCESS_UNKNOWN) && !defined(EMU_LOG_REGISTER_ACCESS_ALL)
         else
            printUnknownHwAccess(address, value, 32, true);
#endif
         break;
   }
}


void resetHwRegisters(){
   memset(palmReg, 0x00, REG_SIZE - BOOTLOADER_SIZE);
   palmCrystalCycles = 0.0;
   clk32Counter = 0;
   pllWakeWait = -1;
   timerCycleCounter[0] = 0.0;
   timerCycleCounter[1] = 0.0;
   timerStatusReadAcknowledge[0] = 0x0000;
   timerStatusReadAcknowledge[1] = 0x0000;
   edgeTriggeredInterruptLastValue = 0x00000000;
   memset(spi1RxFifo, 0x00, 8 * sizeof(uint16_t));
   memset(spi1TxFifo, 0x00, 8 * sizeof(uint16_t));
   spi1RxPosition = 0;
   spi1TxPosition = 0;

   memset(chips, 0x00, sizeof(chips));
   //all chipselects are disabled at boot and CSA is mapped to 0x00000000 and covers the entire address range until CSGBA set otherwise
   chips[CHIP_A_ROM].inBootMode = true;

   //masks for reading and writing
   chips[CHIP_A_ROM].mask = 0x003FFFFF;
   chips[CHIP_B_SED].mask = 0x0003FFFF;
   chips[CHIP_C_USB].mask = 0x00000000;
   chips[CHIP_D_RAM].mask = palmSpecialFeatures & FEATURE_RAM_HUGE ? 0x07FFFFFF/*128mb*/ : 0x00FFFFFF/*16mb*/;
   
   //system control
   registerArrayWrite8(SCR, 0x1C);
   
   //CPU ID
   registerArrayWrite32(IDR, 0x56010000);//value of IDR in POSE
   
   //I/O drive control //probably unused
   registerArrayWrite16(IODCR, 0x1FFF);
   
   //chip selects
   registerArrayWrite16(CSA, 0x00B0);
   registerArrayWrite16(CSD, 0x0200);
   registerArrayWrite16(EMUCS, 0x0060);
   registerArrayWrite16(CSCTRL2, 0x1000);
   registerArrayWrite16(CSCTRL3, 0x9C00);
   
   //phase lock loop
   registerArrayWrite16(PLLCR, 0x24B3);
   registerArrayWrite16(PLLFSR, 0x0347);
   
   //power control
   registerArrayWrite8(PCTLR, 0x1F);
   
   //interrupts
   registerArrayWrite32(IMR, 0x00FFFFFF);
   registerArrayWrite16(ILCR, 0x6533);
   
   //GPIO ports
   registerArrayWrite8(PADATA, 0xFF);
   registerArrayWrite8(PAPUEN, 0xFF);
   
   registerArrayWrite8(PBDATA, 0xFF);
   registerArrayWrite8(PBPUEN, 0xFF);
   registerArrayWrite8(PBSEL, 0xFF);
   
   registerArrayWrite8(PCPDEN, 0xFF);
   registerArrayWrite8(PCSEL, 0xFF);
   
   registerArrayWrite8(PDDATA, 0xFF);
   registerArrayWrite8(PDPUEN, 0xFF);
   registerArrayWrite8(PDSEL, 0xF0);
   
   registerArrayWrite8(PEDATA, 0xFF);
   registerArrayWrite8(PEPUEN, 0xFF);
   registerArrayWrite8(PESEL, 0xFF);
   
   registerArrayWrite8(PFDATA, 0xFF);
   registerArrayWrite8(PFPUEN, 0xFF);
   registerArrayWrite8(PFSEL, 0x87);
   
   registerArrayWrite8(PGDATA, 0x3F);
   registerArrayWrite8(PGPUEN, 0x3D);
   registerArrayWrite8(PGSEL, 0x08);
   
   registerArrayWrite8(PJDATA, 0xFF);
   registerArrayWrite8(PJPUEN, 0xFF);
   registerArrayWrite8(PJSEL, 0xEF);
   
   registerArrayWrite8(PKDATA, 0x0F);
   registerArrayWrite8(PKPUEN, 0xFF);
   registerArrayWrite8(PKSEL, 0xFF);
   
   registerArrayWrite8(PMDATA, 0x20);
   registerArrayWrite8(PMPUEN, 0x3F);
   registerArrayWrite8(PMSEL, 0x3F);
   
   //pulse width modulation control
   registerArrayWrite16(PWMC1, 0x0020);
   registerArrayWrite8(PWMP1, 0xFE);
   
   //timers
   registerArrayWrite16(TCMP1, 0xFFFF);
   registerArrayWrite16(TCMP2, 0xFFFF);
   
   //serial I/O
   registerArrayWrite16(UBAUD1, 0x0002);
   registerArrayWrite16(UBAUD2, 0x0002);
   registerArrayWrite16(HMARK, 0x0102);
   
   //LCD control registers, unused since the SED1376 is present
   registerArrayWrite8(LVPW, 0xFF);
   registerArrayWrite16(LXMAX, 0x03F0);
   registerArrayWrite16(LYMAX, 0x01FF);
   registerArrayWrite16(LCWCH, 0x0101);
   registerArrayWrite8(LBLKC, 0x7F);
   registerArrayWrite16(LRRA, 0x00FF);
   registerArrayWrite8(LGPMR, 0x84);
   registerArrayWrite8(DMACR, 0x62);
   
   //realtime clock
   //RTCTIME is not changed on reset
   registerArrayWrite16(WATCHDOG, 0x0001);
   registerArrayWrite16(RTCCTL, 0x0080);//conflicting size in datasheet, it says its 8 bit but provides 16 bit values
   registerArrayWrite16(STPWCH, 0x003F);//conflicting size in datasheet, it says its 8 bit but provides 16 bit values
   //DAYR is not changed on reset
   
   //SDRAM control, unused since RAM refresh is unemulated
   registerArrayWrite16(SDCTRL, 0x003C);
   
   //add register settings to misc I/O
   updateAlarmLedStatus();
   updateVibratorStatus();

   recalculateCpuSpeed();
}

void setRtc(uint16_t days, uint8_t hours, uint8_t minutes, uint8_t seconds){
   uint32_t rtcTime;
   rtcTime = seconds & 0x0000003F;
   rtcTime |= minutes << 16 & 0x003F0000;
   rtcTime |= hours << 24 & 0x1F000000;
   registerArrayWrite32(RTCTIME, rtcTime);
   registerArrayWrite16(DAYR, days & 0x01FF);
}
