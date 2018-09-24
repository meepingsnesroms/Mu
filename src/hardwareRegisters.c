#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "emulator.h"
#include "specs/hardwareRegisterNames.h"
#include "hardwareRegisters.h"
#include "memoryAccess.h"
#include "m68328.h"
#include "portability.h"
#include "ads7846.h"
#include "sdCard.h"
#include "m68k/m68k.h"
#include "debug/sandbox.h"


chip_t   chips[CHIP_END];
int32_t  pllWakeWait;
uint32_t clk32Counter;
double   timerCycleCounter[2];
uint16_t timerStatusReadAcknowledge[2];
uint32_t interruptEdgeTriggered;
uint16_t spi1RxFifo[9];
uint16_t spi1TxFifo[9];
uint8_t  spi1RxReadPosition;
uint8_t  spi1RxWritePosition;
uint8_t  spi1TxReadPosition;
uint8_t  spi1TxWritePosition;

//warning: pwm1 is not in savestates yet!!!
uint16_t pwm1ClocksToNextSample;
uint8_t  pwm1Fifo[6];
uint8_t  pwm1ReadPosition;
uint8_t  pwm1WritePosition;


static void checkInterrupts();
static void checkPortDInterrupts();
static void recalculateCpuSpeed();
static void pllWakeCpuIfOff();

#include "hardwareRegistersAccessors.c.h"
#include "hardwareRegistersTiming.c.h"

bool pllIsOn(){
   return !(registerArrayRead16(PLLCR) & 0x0008);
}

bool backlightAmplifierState(){
   return getPortKValue() & 0x02;
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
   //update power button LED state if palmMisc.batteryCharging changed
   updatePowerButtonLedStatus();

   //update touchscreen
   updateTouchState();

   //check for button presses and interrupts
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

   //only active interrupts should wake the CPU if the PLL is off
   pllWakeCpuIfOff();

   //the interrupt should only be cleared after its been handled

   return vector;
}

void setBusErrorTimeOut(uint32_t address, bool isWrite){
   uint8_t scr = registerArrayRead8(SCR);
   debugLog("Bus error timeout, PC:0x%08X\n", m68k_get_reg(NULL, M68K_REG_PPC));
   if(scr & 0x10)
      m68328BusError(address, isWrite);
   registerArrayWrite8(SCR, scr | 0x80);
}

void setPrivilegeViolation(uint32_t address, bool isWrite){
   uint8_t scr = registerArrayRead8(SCR);
   debugLog("Privilege violation, PC:0x%08X\n", m68k_get_reg(NULL, M68K_REG_PPC));
   if(scr & 0x10)
      m68328BusError(address, isWrite);
   registerArrayWrite8(SCR, scr | 0x20);
}

void setWriteProtectViolation(uint32_t address){
   uint8_t scr = registerArrayRead8(SCR);
   debugLog("Write protect violation, PC:0x%08X\n", m68k_get_reg(NULL, M68K_REG_PPC));
   if(scr & 0x10)
      m68328BusError(address, true);
   registerArrayWrite8(SCR, scr | 0x40);
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
         newCpuSpeed *= (pctlr & 0x1F) / 31.0;
   }
   else{
      newCpuSpeed = 0.0;
   }

   /*
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
   */

   palmCrystalCycles = newCpuSpeed;
}

static void pllWakeCpuIfOff(){
   if(!pllIsOn() && pllWakeWait == -1){
      //PLL is off and not already in the process of waking up
      uint8_t pllWaitTable[4] = {32, 48, 64, 96};
      pllWakeWait = pllWaitTable[registerArrayRead16(PLLCR) & 0x0003];
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

   //even masked interrupts turn off PCTLR, Chapter 4.5.4 MC68VZ328UM.pdf
   if(intLevel > 0 && registerArrayRead8(PCTLR) & 0x80){
      registerArrayWrite8(PCTLR, registerArrayRead8(PCTLR) & 0x1F);
      recalculateCpuSpeed();
   }

   m68k_set_irq(intLevel);//should be called even if intLevel is 0, that is how the interrupt state gets cleared
}

static void checkPortDInterrupts(){
   uint8_t portDInputValues = getPortDInputPinValues();
   uint8_t portDInputValuesWithPolarity = portDInputValues ^ registerArrayRead8(PDPOL);
   uint8_t portDDir = registerArrayRead8(PDDIR);
   uint8_t portDKeyboardEnable = registerArrayRead8(PDKBEN);
   uint8_t portDIrqPins = ~registerArrayRead8(PDSEL);
   uint8_t portDEdgeTriggered = registerArrayRead8(PDIRQEG);
   uint16_t interruptControlRegister = registerArrayRead16(ICR);
   uint8_t triggeredIntXInterrupts = portDInputValuesWithPolarity & registerArrayRead8(PDIRQEN) & ~portDDir;

   if(triggeredIntXInterrupts & 0x01){
      if(!(portDEdgeTriggered & 0x01) || (!(interruptEdgeTriggered & INT_INT0) && pllIsOn()))
         setIprIsrBit(INT_INT0);
      interruptEdgeTriggered |= INT_INT0;
   }
   else{
      if(!(portDEdgeTriggered & 0x01))
         clearIprIsrBit(INT_INT0);
      interruptEdgeTriggered &= ~INT_INT0;
   }

   if(triggeredIntXInterrupts & 0x02){
      if(!(portDEdgeTriggered & 0x02) || (!(interruptEdgeTriggered & INT_INT1) && pllIsOn()))
         setIprIsrBit(INT_INT1);
      interruptEdgeTriggered |= INT_INT1;
   }
   else{
      if(!(portDEdgeTriggered & 0x02))
         clearIprIsrBit(INT_INT1);
      interruptEdgeTriggered &= ~INT_INT1;
   }

   if(triggeredIntXInterrupts & 0x04){
      if(!(portDEdgeTriggered & 0x04) || (!(interruptEdgeTriggered & INT_INT2) && pllIsOn()))
         setIprIsrBit(INT_INT2);
      interruptEdgeTriggered |= INT_INT2;
   }
   else{
      if(!(portDEdgeTriggered & 0x04))
         clearIprIsrBit(INT_INT2);
      interruptEdgeTriggered &= ~INT_INT2;
   }

   if(triggeredIntXInterrupts & 0x08){
      if(!(portDEdgeTriggered & 0x08) || (!(interruptEdgeTriggered & INT_INT3) && pllIsOn()))
         setIprIsrBit(INT_INT3);
      interruptEdgeTriggered |= INT_INT3;
   }
   else{
      if(!(portDEdgeTriggered & 0x08))
         clearIprIsrBit(INT_INT3);
      interruptEdgeTriggered &= ~INT_INT3;
   }
   
   //IRQ1, polarity set in ICR
   if(portDIrqPins & ~portDDir & 0x10 && (bool)(portDInputValues & 0x10) == (bool)(interruptControlRegister & 0x8000)){
      if(!(interruptControlRegister & 0x0800) || !(interruptEdgeTriggered & INT_IRQ1))
         setIprIsrBit(INT_IRQ1);
      interruptEdgeTriggered |= INT_IRQ1;
   }
   else{
      if(!(interruptControlRegister & 0x0800))
         clearIprIsrBit(INT_IRQ1);
      interruptEdgeTriggered &= ~INT_IRQ1;
   }

   //IRQ2, polarity set in ICR
   if(portDIrqPins & ~portDDir & 0x20 && (bool)(portDInputValues & 0x20) == (bool)(interruptControlRegister & 0x4000)){
      if(!(interruptControlRegister & 0x0400) || !(interruptEdgeTriggered & INT_IRQ2))
         setIprIsrBit(INT_IRQ2);
      interruptEdgeTriggered |= INT_IRQ2;
   }
   else{
      if(!(interruptControlRegister & 0x0400))
         clearIprIsrBit(INT_IRQ2);
      interruptEdgeTriggered &= ~INT_IRQ2;
   }
   
   //IRQ3, polarity set in ICR
   if(portDIrqPins & ~portDDir & 0x40 && (bool)(portDInputValues & 0x40) == (bool)(interruptControlRegister & 0x2000)){
      if(!(interruptControlRegister & 0x0200) || !(interruptEdgeTriggered & INT_IRQ3))
         setIprIsrBit(INT_IRQ3);
      interruptEdgeTriggered |= INT_IRQ3;
   }
   else{
      if(!(interruptControlRegister & 0x0200))
         clearIprIsrBit(INT_IRQ3);
      interruptEdgeTriggered &= ~INT_IRQ3;
   }
   
   //IRQ6, polarity set in ICR
   if(portDIrqPins & ~portDDir & 0x80 && (bool)(portDInputValues & 0x80) == (bool)(interruptControlRegister & 0x1000)){
      if(!(interruptControlRegister & 0x0100) || !(interruptEdgeTriggered & INT_IRQ6))
         setIprIsrBit(INT_IRQ6);
      interruptEdgeTriggered |= INT_IRQ6;
   }
   else{
      if(!(interruptControlRegister & 0x0100))
         clearIprIsrBit(INT_IRQ6);
      interruptEdgeTriggered &= ~INT_IRQ6;
   }
   
   //active low/off level triggered interrupt
   //The SELx, POLx, IQENx, and IQEGx bits have no effect on the functionality of KBENx
   if(portDKeyboardEnable & ~portDInputValues & ~portDDir)
      setIprIsrBit(INT_KB);
   else
      clearIprIsrBit(INT_KB);
   
   checkInterrupts();
}

void printUnknownHwAccess(uint32_t address, uint32_t value, uint32_t size, bool isWrite){
   if(isWrite)
      debugLog("CPU wrote %d bits of 0x%08X to register 0x%03X, PC:0x%08X.\n", size, value, address, m68k_get_reg(NULL, M68K_REG_PPC));
   else
      debugLog("CPU read %d bits from register 0x%03X, PC:0x%08X.\n", size, address, m68k_get_reg(NULL, M68K_REG_PPC));
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
                  if(palmSpecialFeatures & FEATURE_DEBUG)
                     sandboxReturn();
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

   if(sandboxRunning() && address < 0xE00)
      printUnknownHwAccess(address, 0, 8, false);

   switch(address){
      case PADATA:
         return getPortAValue();

      case PBDATA:
         return getPortBValue();

      case PCDATA:
         return getPortCValue();

      case PDDATA:
         return getPortDValue();

      case PEDATA:
         return getPortEValue();

      case PFDATA:
         return getPortFValue();

      case PGDATA:
         return getPortGValue();

      case PJDATA:
         return getPortJValue();

      case PKDATA:
         return getPortKValue();

      case PMDATA:
         return getPortMValue();

      //basic non GPIO functions
      case SCR:
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
         if(address < 0xE00)
            printUnknownHwAccess(address, 0, 8, false);
         return 0x00;
   }
}

uint16_t getHwRegister16(uint32_t address){
   //not emu or hardware register, invalid access
   if((address & 0x0000F000) != 0x0000F000)
      return 0x0000;
   
   address &= 0x00000FFF;

   if(sandboxRunning() && address < 0xE00)
      printUnknownHwAccess(address, 0, 16, false);

   switch(address){
      case TSTAT1:
         timerStatusReadAcknowledge[0] |= registerArrayRead16(TSTAT1);//active bits acknowledged
         return registerArrayRead16(TSTAT1);

      case TSTAT2:
         timerStatusReadAcknowledge[1] |= registerArrayRead16(TSTAT2);//active bits acknowledged
         return registerArrayRead16(TSTAT2);

      case PWMC1:
         return getPwmc1();

      case SPIINTCS:
         debugLog("SPIINTCS read not implented yet\n");
         return 0x0000;

      case SPITEST:
         //SSTATUS is unemulated because the datasheet has no descrption of how it works
         return spi1RxFifoEntrys() << 4 | spi1TxFifoEntrys();

      case SPIRXD:
         return spi1RxFifoRead();
         
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
      case SPICONT1:
      case SPICONT2:
      case SPIDATA2:
         //simple read, no actions needed
         return registerArrayRead16(address);
         
      default:
         //bootloader
         if(address >= 0xE00)
            return registerArrayRead16(address);
         if(address < 0xE00)
            printUnknownHwAccess(address, 0, 16, false);
         return 0x0000;
   }
}

uint32_t getHwRegister32(uint32_t address){
   //32 bit emu register read, valid
   if((address & 0x0000F000) != 0x0000F000){
      if((address & 0x0000F000) == 0x0000E000)
         return getEmuRegister(address);
      return 0x00000000;
   }
   
   address &= 0x00000FFF;

   if(sandboxRunning() && address < 0xE00)
      printUnknownHwAccess(address, 0, 32, false);

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
         if(address < 0xE00)
            printUnknownHwAccess(address, 0, 32, false);
         return 0x00000000;
   }
}


void setHwRegister8(uint32_t address, uint8_t value){
   //not emu or hardware register, invalid access
   if((address & 0x0000F000) != 0x0000F000)
      return;
   
   address &= 0x00000FFF;

   if(sandboxRunning() && address < 0xE00)
      printUnknownHwAccess(address, value, 8, true);

   switch(address){ 
      case SCR:
         setScr(value);
         break;

      case PWMS1 + 1:
         if(pwm1FifoEntrys() < 5)
            pwm1FifoWrite(value);
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
         updatePowerButtonLedStatus();
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
         registerArrayWrite8(PFSEL, value);
         setSed1376Attached(sed1376ClockConnected());
         break;
         
      case PGSEL:
      case PGDIR:
      case PGDATA:
         //write without the top 2 bits
         registerArrayWrite8(address, value & 0x3F);
         updateAds7846ChipSelectStatus();
         break;
         
      case PKSEL:
      case PKDIR:
      case PKDATA:
         registerArrayWrite8(address, value);
         checkPortDInterrupts();
         updateVibratorStatus();
         updateBacklightAmplifierStatus();
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
         if(address < 0xE00)
            printUnknownHwAccess(address, value, 8, true);
         break;
   }
}

void setHwRegister16(uint32_t address, uint16_t value){
   //not emu or hardware register, invalid access
   if((address & 0x0000F000) != 0x0000F000)
      return;
   
   address &= 0x00000FFF;

   if(sandboxRunning() && address < 0xE00)
      printUnknownHwAccess(address, value, 16, true);

   switch(address){  
      case RTCIENR:
         //missing bits 6 and 7
         registerArrayWrite16(address, value & 0xFF3F);
         break;

      case RTCCTL:
         registerArrayWrite16(address, value & 0x00A0);
         break;
         
      case IMR:
         //this is a 32 bit register but Palm OS writes it as 16 bit chunks
         registerArrayWrite16(IMR, value & 0x00FF);
         registerArrayWrite16(ISR, registerArrayRead16(IPR) & registerArrayRead16(IMR));
         break;
      case IMR + 2:
         //this is a 32 bit register but Palm OS writes it as 16 bit chunks
         registerArrayWrite16(IMR + 2, value & 0x03FF);
         registerArrayWrite16(ISR + 2, registerArrayRead16(IPR + 2) & registerArrayRead16(IMR + 2));
         break;

      case ISR:
         setIsr(value << 16, true, false);
         break;
      case ISR + 2:
         setIsr(value, false, true);
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
         //1 must be written to clear INTF
         registerArrayWrite16(WATCHDOG, (value & 0x0003) | (registerArrayRead16(WATCHDOG) & (~value & 0x0080)));
         if(!(registerArrayRead16(WATCHDOG) & 0x0080))
            clearIprIsrBit(INT_WDT);
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
         registerArrayWrite16(PLLCR, value & 0x3FBB);
         recalculateCpuSpeed();
         setSed1376Attached(sed1376ClockConnected());

         if(value & 0x0008){
            //The PLL shuts down 30 clock cycles of SYSCLK after the DISPLL bit is set in the PLLCR
            m68k_modify_timeslice(-m68k_cycles_remaining() + 30);
            debugLog("Disable PLL set, CPU off in 30 cycles!\n");
         }
         break;
         
      case ICR:
         //missing bottom 7 bits
         registerArrayWrite16(address, value & 0xFF80);
         updateTouchState();
         checkPortDInterrupts();//this calls checkInterrupts() so it doesnt need to be called above
         break;

      case ILCR:
         setIlcr(value);
         break;
         
      case DRAMC:
         //somewhat unemulated
         //missing bit 7 and 6
         //debugLog("Set DRAMC, old value:0x%04X, new value:0x%04X, PC:0x%08X\n", registerArrayRead16(address), value, m68k_get_reg(NULL, M68K_REG_PPC));
         registerArrayWrite16(address, value & 0xFF3F);
         updateCsdAddressLines();//the EDO bit can disable SDRAM access
         break;
         
      case DRAMMC:
         //unemulated, address line remapping, too CPU intensive to emulate
         registerArrayWrite16(address, value);
         break;
         
      case SDCTRL:
         //missing bits 13, 9, 8 and 7
         //debugLog("Set SDCTRL, old value:0x%04X, new value:0x%04X, PC:0x%08X\n", registerArrayRead16(address), value, m68k_get_reg(NULL, M68K_REG_PPC));
         registerArrayWrite16(address, value & 0xDC7F);
         updateCsdAddressLines();
         break;

      case CSA:{
            uint16_t oldCsa = registerArrayRead16(CSA);
            bool oldBootMode = chips[CHIP_A0_ROM].inBootMode;

            setCsa(value);

            //only reset address space if size changed, enabled/disabled or exiting boot mode
            if((value & 0x000F) != (oldCsa & 0x000F) || chips[CHIP_A0_ROM].inBootMode != oldBootMode)
               resetAddressSpace();
         }
         break;

      case CSB:{
            uint16_t oldCsb = registerArrayRead16(CSB);

            setCsb(value);

            //only reset address space if size changed or enabled/disabled
            if((value & 0x000F) != (oldCsb & 0x000F))
               resetAddressSpace();
         }
         break;

      case CSC:
         registerArrayWrite16(CSC, value & 0xF9FF);
         break;

      case CSD:{
            uint16_t oldCsd = registerArrayRead16(CSD);

            setCsd(value);

            //CSD DRAM bit changed
            if((value & 0x0200) != (oldCsd & 0x0200))
               updateCsdAddressLines();


            //only reset address space if size changed, enabled/disabled or DRAM bit changed
            if((value & 0x020F) != (oldCsd & 0x020F))
               resetAddressSpace();
         }
         break;

      case CSGBA:
         //sets the starting location of ROM(0x10000000) and the PDIUSBD12 chip
         if((value & 0xFFFE) != registerArrayRead16(CSGBA)){
            setCsgba(value);
            resetAddressSpace();
         }
         break;

      case CSGBB:
         //sets the starting location of the SED1376(0x1FF80000)
         if((value & 0xFFFE) != registerArrayRead16(CSGBB)){
            setCsgbb(value);
            resetAddressSpace();
         }
         break;

      case CSGBC:
         registerArrayWrite16(CSGBC, value & 0xFFFE);
         break;

      case CSGBD:
         //sets the starting location of RAM(0x00000000)
         if((value & 0xFFFE) != registerArrayRead16(CSGBD)){
            setCsgbd(value);
            resetAddressSpace();
         }
         break;

      case CSUGBA:
         if((value & 0xF777) != registerArrayRead16(CSUGBA)){
            registerArrayWrite16(CSUGBA, value & 0xF777);
            //refresh all chip select address lines
            setCsgba(registerArrayRead16(CSGBA));
            setCsgbb(registerArrayRead16(CSGBB));
            setCsgbd(registerArrayRead16(CSGBD));
            resetAddressSpace();
         }
         break;

      case CSCTRL1:{
            uint16_t oldCsctrl1 = registerArrayRead16(CSCTRL1);

            registerArrayWrite16(CSCTRL1, value & 0x7F55);

            if((value & 0x4055) != (oldCsctrl1 & 0x4055)){
               //something important changed, update all chip selects
               //CSA is not dependent on CSCTRL1
               setCsb(registerArrayRead16(CSB));
               setCsd(registerArrayRead16(CSD));
               resetAddressSpace();
            }
         }
         break;

      case SPICONT1:
         setSpiCont1(value);
         break;

      case SPIINTCS:
         debugLog("SPIINTCS write not implented yet\n");
         break;

      case SPITEST:
         debugLog("SPITEST write not implented yet\n");
         break;

      case SPITXD:
         //Writing to TxFIFO is permitted as long as TxFIFO is not full, from MC68VZ328UM.pdf
         if(spi1TxFifoEntrys() < 8)
            spi1TxFifoWrite(value);
         break;

      case SPICONT2:
         setSpiCont2(value);
         break;

      case SPIDATA2:
         //ignore writes when SPICONT2 ENABLE is not set
         if(registerArrayRead16(SPICONT2) & 0x0200)
            registerArrayWrite16(SPIDATA2, value);
         break;

      case PWMC1:
         setPwmc1(value);
         break;

      case PWMS1:
         if(pwm1FifoEntrys() < 5)
            pwm1FifoWrite(value >> 8);
         if(pwm1FifoEntrys() < 5)
            pwm1FifoWrite(value & 0xFF);
         break;

      case SPISPC:
         //SPI1 timing, unemulated for now

      case TCMP1:
      case TCMP2:
      case TPRER1:
      case TPRER2:
         //simple write, no actions needed
         registerArrayWrite16(address, value);
         break;
         
      default:
         //writeable bootloader region
         if(address >= 0xFC0)
            registerArrayWrite16(address, value);
         if(address < 0xE00)
            printUnknownHwAccess(address, value, 16, true);
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

   if(sandboxRunning() && address < 0xE00)
      printUnknownHwAccess(address, value, 32, true);

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
         setIsr(value, true, true);
         break;
         
      case IMR:
         registerArrayWrite32(IMR, value & 0x00FF3FFF);
         registerArrayWrite32(ISR, registerArrayRead32(IPR) & registerArrayRead32(IMR));
         break;
         
      case LSSA:
         //simple write, no actions needed
         registerArrayWrite32(address, value);
         break;
      
      default:
         //writeable bootloader region
         if(address >= 0xFC0)
            registerArrayWrite32(address, value);
         if(address < 0xE00)
            printUnknownHwAccess(address, value, 32, true);
         break;
   }
}


void resetHwRegisters(){
   uint32_t oldRtc = registerArrayRead32(RTCTIME);//preserve RTCTIME
   uint16_t oldDayr = registerArrayRead16(DAYR);//preserve DAYR

   memset(palmReg, 0x00, REG_SIZE - BOOTLOADER_SIZE);
   palmCrystalCycles = 0.0;
   clk32Counter = 0;
   pllWakeWait = -1;
   timerCycleCounter[0] = 0.0;
   timerCycleCounter[1] = 0.0;
   timerStatusReadAcknowledge[0] = 0x0000;
   timerStatusReadAcknowledge[1] = 0x0000;
   interruptEdgeTriggered = 0x00000000;
   memset(spi1RxFifo, 0x00, 8 * sizeof(uint16_t));
   memset(spi1TxFifo, 0x00, 8 * sizeof(uint16_t));
   spi1RxReadPosition = 0;
   spi1RxWritePosition = 0;
   spi1TxReadPosition = 0;
   spi1TxWritePosition = 0;

   memset(chips, 0x00, sizeof(chips));
   //all chip selects are disabled at boot and CSA0 is mapped to 0x00000000 and covers the entire address range until CSA is set enabled
   chips[CHIP_A0_ROM].inBootMode = true;

   //default sizes
   chips[CHIP_A0_ROM].lineSize = 0x20000;
   chips[CHIP_A1_USB].lineSize = 0x20000;
   chips[CHIP_B0_SED].lineSize = 0x20000;
   chips[CHIP_DX_RAM].lineSize = 0x8000;

   //masks for reading and writing
   chips[CHIP_A0_ROM].mask = 0x003FFFFF;//4mb
   chips[CHIP_A1_USB].mask = 0x00000002;//A1 is used as USB chip A0
   chips[CHIP_B0_SED].mask = 0x0001FFFF;
   chips[CHIP_DX_RAM].mask = 0x00000000;//16mb, no RAM enabled until the DRAM module is initialized
   
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
   registerArrayWrite32(IMR, 0x00FF3FFF);//the data sheet says 0x00FFFFFF and 0x00FF3FFF, since I am anding out all reserved bits I am using 0x00FF3FFF
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
   
   //LCD control registers, unused since the SED1376 controls the LCD
   registerArrayWrite8(LVPW, 0xFF);
   registerArrayWrite16(LXMAX, 0x03F0);
   registerArrayWrite16(LYMAX, 0x01FF);
   registerArrayWrite16(LCWCH, 0x0101);
   registerArrayWrite8(LBLKC, 0x7F);
   registerArrayWrite16(LRRA, 0x00FF);
   registerArrayWrite8(LGPMR, 0x84);
   registerArrayWrite8(DMACR, 0x62);
   
   //realtime clock
   registerArrayWrite32(RTCTIME, oldRtc);//RTCTIME is not changed on reset
   registerArrayWrite16(WATCHDOG, 0x0001);
   registerArrayWrite16(RTCCTL, 0x0080);//conflicting size in datasheet, it says its 8 bit but provides 16 bit values
   registerArrayWrite16(STPWCH, 0x003F);//conflicting size in datasheet, it says its 8 bit but provides 16 bit values
   registerArrayWrite16(DAYR, oldDayr);//DAYR is not changed on reset
   
   //SDRAM control, unused since RAM refresh is unemulated
   registerArrayWrite16(SDCTRL, 0x003C);
   
   //move register settings to other I/O
   updatePowerButtonLedStatus();
   updateVibratorStatus();
   updateAds7846ChipSelectStatus();
   updateBacklightAmplifierStatus();

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
