#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "emulator.h"
#include "specs/dragonballVzRegisterSpec.h"
#include "hardwareRegisters.h"
#include "memoryAccess.h"
#include "portability.h"
#include "flx68000.h"
#include "ads7846.h"
#include "sdCard.h"
#include "audio/blip_buf.h"
#include "debug/sandbox.h"


chip_t   chips[CHIP_END];
int8_t   pllSleepWait;
int8_t   pllWakeWait;
uint32_t clk32Counter;
double   pctlrCpuClockDivider;
double   timerCycleCounter[2];
uint16_t timerStatusReadAcknowledge[2];
uint8_t  portDInterruptLastValue;//used for edge triggered interrupt timing
uint16_t spi1RxFifo[9];
uint16_t spi1TxFifo[9];
uint8_t  spi1RxReadPosition;
uint8_t  spi1RxWritePosition;
bool     spi1RxOverflowed;
uint8_t  spi1TxReadPosition;
uint8_t  spi1TxWritePosition;
int32_t  pwm1ClocksToNextSample;
uint8_t  pwm1Fifo[6];
uint8_t  pwm1ReadPosition;
uint8_t  pwm1WritePosition;


static void checkInterrupts(void);
static void checkPortDInterrupts(void);
static void pllWakeCpuIfOff(void);
static double sysclksPerClk32(void);
static int32_t audioGetFramePercentIncrementFromClk32s(int32_t count);
static int32_t audioGetFramePercentIncrementFromSysclks(double count);
static int32_t audioGetFramePercentage(void);

#include "hardwareRegistersAccessors.c.h"
#include "hardwareRegistersTiming.c.h"

bool pllIsOn(void){
   return !(palmSysclksPerClk32 < 1.0);
}

bool backlightAmplifierState(void){
   return !!(getPortKValue() & 0x02);
}

bool registersAreXXFFMapped(void){
   return !!(registerArrayRead8(SCR) & 0x04);
}

bool sed1376ClockConnected(void){
   //this is the clock output pin for the SED1376, if its disabled so is the LCD controller
   //port f pin 2 is not GPIO and PLLCR CLKEN is false enabling clock output on port f pin 2
   return !(registerArrayRead8(PFSEL) & 0x04) && !(registerArrayRead16(PLLCR) & 0x0010);
}

void ads7846OverridePenState(bool value){
   //causes a temporary override of the touchscreen value, used to trigger fake interrupts from line noise when reading from the ADS7846
   if(value != (ads7846PenIrqEnabled ? !palmInput.touchscreenTouched : true)){
      if(!(registerArrayRead8(PFSEL) & registerArrayRead8(PFDIR) & 0x02)){
         if(value == !!(registerArrayRead16(ICR) & 0x0080))
            setIprIsrBit(INT_IRQ5);
         else
            clearIprIsrBit(INT_IRQ5);
      }
      checkInterrupts();

      //override over, put back real state
      updateTouchState();
      checkInterrupts();
   }
}

void refreshTouchState(void){
   //called when ads7846PenIrqEnabled is changed
   updateTouchState();
   checkInterrupts();
}

void refreshInputState(void){
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
   if(vectorOffset)
      vector = vectorOffset | intLevel;
   else
      vector = 15;//EXCEPTION_UNINITIALIZED_INTERRUPT

   //only active interrupts should wake the CPU if the PLL is off
   pllWakeCpuIfOff();

   //the interrupt should only be cleared after its been handled
   return vector;
}

void setBusErrorTimeOut(uint32_t address, bool isWrite){
   uint8_t scr = registerArrayRead8(SCR);
   debugLog("Bus error timeout, PC:0x%08X\n", flx68000GetPc());
   registerArrayWrite8(SCR, scr | 0x80);
   if(scr & 0x10)
      flx68000BusError(address, isWrite);
}

void setPrivilegeViolation(uint32_t address, bool isWrite){
   uint8_t scr = registerArrayRead8(SCR);
   debugLog("Privilege violation, PC:0x%08X\n", flx68000GetPc());
   registerArrayWrite8(SCR, scr | 0x20);
   if(scr & 0x10)
      flx68000BusError(address, isWrite);
}

void setWriteProtectViolation(uint32_t address){
   uint8_t scr = registerArrayRead8(SCR);
   debugLog("Write protect violation, PC:0x%08X\n", flx68000GetPc());
   registerArrayWrite8(SCR, scr | 0x40);
   if(scr & 0x10)
      flx68000BusError(address, true);
}

static void pllWakeCpuIfOff(void){
   const int8_t pllWaitTable[4] = {32, 48, 64, 96};

   //PLL is off and not already in the process of waking up
   if(!pllIsOn() && pllWakeWait == -1)
      pllWakeWait = pllWaitTable[registerArrayRead16(PLLCR) & 0x0003];
}

static void checkInterrupts(void){
   uint32_t activeInterrupts = registerArrayRead32(ISR);
   uint16_t interruptLevelControlRegister = registerArrayRead16(ILCR);
   uint8_t spi1IrqLevel = interruptLevelControlRegister >> 12;
   uint8_t uart2IrqLevel = interruptLevelControlRegister >> 8 & 0x0007;
   uint8_t pwm2IrqLevel = interruptLevelControlRegister >> 4 & 0x0007;
   uint8_t timer2IrqLevel = interruptLevelControlRegister & 0x0007;
   uint8_t intLevel = 0;

   //static interrupts
   if(activeInterrupts & INT_EMIQ)
      intLevel = 7;//EMIQ - Emulator IRQ, has nothing to do with emulation, used for debugging on a dev board

   if(intLevel < 6 && activeInterrupts & (INT_TMR1 | INT_PWM1 | INT_IRQ6))
      intLevel = 6;

   if(intLevel < 5 && activeInterrupts & INT_IRQ5)
      intLevel = 5;

   if(intLevel < 4 && activeInterrupts & (INT_SPI2 | INT_UART1 | INT_WDT | INT_RTC | INT_KB | INT_RTI | INT_INT0 | INT_INT1 | INT_INT2 | INT_INT3))
      intLevel = 4;

   if(intLevel < 3 && activeInterrupts & INT_IRQ3)
      intLevel = 3;

   if(intLevel < 2 && activeInterrupts & INT_IRQ2)
      intLevel = 2;

   if(intLevel < 1 && activeInterrupts & INT_IRQ1)
      intLevel = 1;

   //configureable interrupts
   if(intLevel < spi1IrqLevel && activeInterrupts & INT_SPI1)
      intLevel = spi1IrqLevel;

   if(intLevel < uart2IrqLevel && activeInterrupts & INT_UART2)
      intLevel = uart2IrqLevel;

   if(intLevel < pwm2IrqLevel && activeInterrupts & INT_PWM2)
      intLevel = pwm2IrqLevel;

   if(intLevel < timer2IrqLevel && activeInterrupts & INT_TMR2)
      intLevel = timer2IrqLevel;

   //even masked interrupts turn off PCTLR, 4.5.4 Power Control Register MC68VZ328UM.pdf
   if(intLevel > 0 && registerArrayRead8(PCTLR) & 0x80){
      registerArrayWrite8(PCTLR, registerArrayRead8(PCTLR) & 0x1F);
      pctlrCpuClockDivider = 1.0;
   }

   //should be called even if intLevel is 0, that is how the interrupt state gets cleared
   flx68000SetIrq(intLevel);
}

static void checkPortDInterrupts(void){
   uint16_t icr = registerArrayRead16(ICR);
   uint8_t icrPolSwap = (!!(icr & 0x1000) << 7 | !!(icr & 0x2000) << 6 | !!(icr & 0x4000) << 5 | !!(icr & 0x8000) << 4) ^ 0xF0;//shifted to match port d layout
   uint8_t icrEdgeTriggered = !!(icr & 0x0100) << 7 | !!(icr & 0x0200) << 6 | !!(icr & 0x0400) << 5 | !!(icr & 0x0800) << 4;//shifted to match port d layout
   uint8_t portDInterruptValue = getPortDValue() ^ icrPolSwap;//not the same as the actual pin values, this already has all polarity swaps applied
   uint8_t portDInterruptEdgeTriggered = icrEdgeTriggered | registerArrayRead8(PDIRQEG);
   uint8_t portDInterruptEnabled = (~registerArrayRead8(PDSEL) & 0xF0) | registerArrayRead8(PDIRQEN);
   uint8_t portDIsInput = ~registerArrayRead8(PDDIR);
   uint8_t portDInterruptTriggered = portDInterruptValue & portDInterruptEnabled & portDIsInput & (~portDInterruptEdgeTriggered | ~portDInterruptLastValue & (pllIsOn() ? 0xFF : 0xF0));

   if(portDInterruptTriggered & 0x01)
      setIprIsrBit(INT_INT0);
   else if(!(portDInterruptEdgeTriggered & 0x01))
      clearIprIsrBit(INT_INT0);

   if(portDInterruptTriggered & 0x02)
      setIprIsrBit(INT_INT1);
   else if(!(portDInterruptEdgeTriggered & 0x02))
      clearIprIsrBit(INT_INT1);

   if(portDInterruptTriggered & 0x04)
      setIprIsrBit(INT_INT2);
   else if(!(portDInterruptEdgeTriggered & 0x04))
      clearIprIsrBit(INT_INT2);

   if(portDInterruptTriggered & 0x08)
      setIprIsrBit(INT_INT3);
   else if(!(portDInterruptEdgeTriggered & 0x08))
      clearIprIsrBit(INT_INT3);

   if(portDInterruptTriggered & 0x10)
      setIprIsrBit(INT_IRQ1);
   else if(!(portDInterruptEdgeTriggered & 0x10))
      clearIprIsrBit(INT_IRQ1);

   if(portDInterruptTriggered & 0x20)
      setIprIsrBit(INT_IRQ2);
   else if(!(portDInterruptEdgeTriggered & 0x20))
      clearIprIsrBit(INT_IRQ2);

   if(portDInterruptTriggered & 0x40)
      setIprIsrBit(INT_IRQ3);
   else if(!(portDInterruptEdgeTriggered & 0x40))
      clearIprIsrBit(INT_IRQ3);

   if(portDInterruptTriggered & 0x80)
      setIprIsrBit(INT_IRQ6);
   else if(!(portDInterruptEdgeTriggered & 0x80))
      clearIprIsrBit(INT_IRQ6);

   //active low/off level triggered interrupt(triggers on 0, not a pull down resistor)
   //The SELx, POLx, IQENx, and IQEGx bits have no effect on the functionality of KBENx, 10.4.5.8 Port D Keyboard Enable Register MC68VZ328UM.pdf
   //the above has finally been verified to be correct!
   if(registerArrayRead8(PDKBEN) & ~(getPortDValue() ^ registerArrayRead8(PDPOL)) & portDIsInput)
      setIprIsrBit(INT_KB);
   else
      clearIprIsrBit(INT_KB);

   //save to check against next time this function is called
   portDInterruptLastValue = portDInterruptTriggered;

   checkInterrupts();
}

static void printHwRegAccess(uint32_t address, uint32_t value, uint32_t size, bool isWrite){
   if(isWrite)
      debugLog("CPU wrote %d bits of 0x%08X to register 0x%03X, PC:0x%08X.\n", size, value, address, flx68000GetPc());
   else
      debugLog("CPU read %d bits from register 0x%03X, PC:0x%08X.\n", size, address, flx68000GetPc());
}

uint8_t getHwRegister8(uint32_t address){
#if !defined(EMU_NO_SAFETY)
   if((address & 0x0000F000) != 0x0000F000){
      setBusErrorTimeOut(address, false);
      return 0x00;
   }
#endif

   address &= 0x00000FFF;

   if(sandboxRunning() && address < 0xE00)
      printHwRegAccess(address, 0, 8, false);

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

      case PWMCNT1:
         debugLog("PWMCNT1 not implimented\n");
         return 0x00;

      //16 bit registers being read as 8 bit
      case SPICONT1:
      case SPICONT1 + 1:
      case SPIINTCS:
      case SPIINTCS + 1:
      case PLLFSR:
      case PLLFSR + 1:

      //basic non GPIO functions
      case SCR:
      case LCKCON:
      case IVR:
      case PWMP1:

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

         if(!sandboxRunning())//dont double unknown accesses when sandbox is active
            printHwRegAccess(address, 0, 8, false);
         return 0x00;
   }
}

uint16_t getHwRegister16(uint32_t address){
#if !defined(EMU_NO_SAFETY)
   if((address & 0x0000F000) != 0x0000F000){
      setBusErrorTimeOut(address, false);
      return 0x0000;
   }
#endif

   address &= 0x00000FFF;

   if(sandboxRunning() && address < 0xE00)
      printHwRegAccess(address, 0, 16, false);

   switch(address){
      case TSTAT1:
         timerStatusReadAcknowledge[0] |= registerArrayRead16(TSTAT1);//active bits acknowledged
         return registerArrayRead16(TSTAT1);

      case TSTAT2:
         timerStatusReadAcknowledge[1] |= registerArrayRead16(TSTAT2);//active bits acknowledged
         return registerArrayRead16(TSTAT2);

      case PWMC1:
         return getPwmc1();

      case SPITEST:
         //SSTATUS is unemulated because the datasheet has no descrption of how it works
         return spi1RxFifoEntrys() << 4 | spi1TxFifoEntrys();

      case SPIRXD:{
            uint16_t fifoVal = spi1RxFifoRead();
            //check if SPI1 interrupts changed
            setSpiIntCs(registerArrayRead16(SPIINTCS));
            //debugLog("SPIRXD read, FIFO value:0x%04X, SPIINTCS:0x%04X\n", fifoVal, registerArrayRead16(SPIINTCS));
            return fifoVal;
         }

      case PLLFSR:
         //this is a hack, it makes the busy wait in HwrDelay finish instantly, prevents issues with the power button
         registerArrayWrite16(PLLFSR, registerArrayRead16(PLLFSR) ^ 0x8000);
         return registerArrayRead16(PLLFSR);

      //32 bit registers accessed as 16 bit
      case IDR:
      case IDR + 2:
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
      case SPIINTCS:
      case SPICONT2:
      case SPIDATA2:
         //simple read, no actions needed
         return registerArrayRead16(address);

      default:
         //bootloader
         if(address >= 0xE00)
            return registerArrayRead16(address);

         if(!sandboxRunning())//dont double unknown accesses when sandbox is active
            printHwRegAccess(address, 0, 16, false);
         return 0x0000;
   }
}

uint32_t getHwRegister32(uint32_t address){
#if !defined(EMU_NO_SAFETY)
   if((address & 0x0000F000) != 0x0000F000){
      setBusErrorTimeOut(address, false);
      return 0x00000000;
   }
#endif

   address &= 0x00000FFF;

   if(sandboxRunning() && address < 0xE00)
      printHwRegAccess(address, 0, 32, false);

   switch(address){
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

         if(!sandboxRunning())//dont double unknown accesses when sandbox is active
            printHwRegAccess(address, 0, 32, false);
         return 0x00000000;
   }
}

void setHwRegister8(uint32_t address, uint8_t value){
#if !defined(EMU_NO_SAFETY)
   if((address & 0x0000F000) != 0x0000F000){
      setBusErrorTimeOut(address, true);
      return;
   }
#endif

   address &= 0x00000FFF;

   if(sandboxRunning() && address < 0xE00)
      printHwRegAccess(address, value, 8, true);

   switch(address){
      case SCR:
         setScr(value);
         return;

      case PWMS1 + 1:
         //write only if PWM1 enabled
         if(registerArrayRead16(PWMC1) & 0x0010)
            pwm1FifoWrite(value);
         return;

      case PWMP1:
         //write only if PWM1 enabled
         if(registerArrayRead16(PWMC1) & 0x0010)
            registerArrayWrite8(address, value);
         return;

      case PCTLR:
         registerArrayWrite8(address, value & 0x9F);
         if(value & 0x80)
            pctlrCpuClockDivider = (value & 0x1F) / 31.0;
         return;

      case IVR:
         //write without the bottom 3 bits
         registerArrayWrite8(address, value & 0xF8);
         return;

      case PBSEL:
      case PBDIR:
      case PBDATA:
         registerArrayWrite8(address, value);
         updatePowerButtonLedStatus();
         return;

      case PDSEL:
         //write without the bottom 4 bits
         registerArrayWrite8(address, value & 0xF0);
         checkPortDInterrupts();
         return;

      case PDPOL:
      case PDIRQEN:
      case PDIRQEG:
         //write without the top 4 bits
         registerArrayWrite8(address, value & 0x0F);
         checkPortDInterrupts();
         return;

      case PDDATA:
      case PDKBEN:
         //can change interrupt state
         registerArrayWrite8(address, value);
         checkPortDInterrupts();
         return;

      case PFSEL:
         //this register controls the clock output pin for the SED1376 and IRQ line for PENIRQ
         registerArrayWrite8(PFSEL, value);
         setSed1376Attached(sed1376ClockConnected());
#if !defined(EMU_NO_SAFETY)
         updateTouchState();
         checkInterrupts();
#endif
         return;

      case PFDIR:
         //this register controls the IRQ line for PENIRQ
         registerArrayWrite8(PFDIR, value);
#if !defined(EMU_NO_SAFETY)
         updateTouchState();
         checkInterrupts();
#endif
         return;

      case PGSEL:
      case PGDIR:
      case PGDATA:
         //write without the top 2 bits
         registerArrayWrite8(address, value & 0x3F);
         updateAds7846ChipSelectStatus();
         return;

      case PJSEL:
      case PJDIR:
      case PJDATA:
         registerArrayWrite8(address, value);
         updateSdCardChipSelectStatus();
         return;

      case PKSEL:
      case PKDIR:
      case PKDATA:
         registerArrayWrite8(address, value);
         checkPortDInterrupts();
         updateVibratorStatus();
         updateBacklightAmplifierStatus();
         return;

      case PMSEL:
      case PMDIR:
      case PMDATA:
         //unemulated
         //infrared shutdown
         registerArrayWrite8(address, value & 0x3F);
         return;

      case PMPUEN:
      case PGPUEN:
         //write without the top 2 bits
         registerArrayWrite8(address, value & 0x3F);
         return;

      //select between GPIO or special function
      case PCSEL:
      case PESEL:

      //direction select
      case PADIR:
      case PCDIR:
      case PDDIR:
      case PEDIR:

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
      case PEDATA:
      case PFDATA:

      //dragonball LCD controller, not attached to anything in Palm m515
      case LCKCON:
         //simple write, no actions needed
         registerArrayWrite8(address, value);
         return;

      default:
         //writeable bootloader region
         if(address >= 0xFC0){
            registerArrayWrite32(address, value);
            return;
         }

         if(!sandboxRunning())//dont double unknown accesses when sandbox is active
            printHwRegAccess(address, value, 8, true);
         return;
   }
}

void setHwRegister16(uint32_t address, uint16_t value){
#if !defined(EMU_NO_SAFETY)
   if((address & 0x0000F000) != 0x0000F000){
      setBusErrorTimeOut(address, true);
      return;
   }
#endif

   address &= 0x00000FFF;

   if(sandboxRunning() && address < 0xE00)
      printHwRegAccess(address, value, 16, true);

   switch(address){
      case RTCIENR:
         //missing bits 6 and 7
         registerArrayWrite16(address, value & 0xFF3F);
         return;

      case RTCCTL:
         registerArrayWrite16(address, value & 0x00A0);
         return;

      case IMR:
         //this is a 32 bit register but Palm OS writes to it as 16 bit chunks
         registerArrayWrite16(IMR, value & 0x00FF);
         registerArrayWrite16(ISR, registerArrayRead16(IPR) & ~registerArrayRead16(IMR));
         checkInterrupts();
         return;
      case IMR + 2:
         //this is a 32 bit register but Palm OS writes to it as 16 bit chunks
         registerArrayWrite16(IMR + 2, value & 0xFFFF);//Palm OS writes to reserved bits 14 and 15
         registerArrayWrite16(ISR + 2, registerArrayRead16(IPR + 2) & ~registerArrayRead16(IMR + 2));
         checkInterrupts();
         return;

      case ISR:
         setIsr(value << 16, true, false);
         return;
      case ISR + 2:
         setIsr(value, false, true);
         return;

      case TCTL1:
      case TCTL2:
         registerArrayWrite16(address, value & 0x01FF);
         return;

      case TSTAT1:
         setTstat1(value);
         return;

      case TSTAT2:
         setTstat2(value);
         return;

      case WATCHDOG:
         //writing to the watchdog resets the counter bits(8 and 9) to 0
         //1 must be written to clear INTF
         registerArrayWrite16(WATCHDOG, (value & 0x0003) | (registerArrayRead16(WATCHDOG) & (~value & 0x0080)));
         if(!(registerArrayRead16(WATCHDOG) & 0x0080))
            clearIprIsrBit(INT_WDT);
         return;

      case RTCISR:
         registerArrayWrite16(RTCISR, registerArrayRead16(RTCISR) & ~value);
         if(!(registerArrayRead16(RTCISR) & 0xFF00))
            clearIprIsrBit(INT_RTI);
         if(!(registerArrayRead16(RTCISR) & 0x003F))
            clearIprIsrBit(INT_RTC);
         checkInterrupts();
         return;

      case PLLFSR:
         setPllfsr(value);
         return;

      case PLLCR:
         //CLKEN is required for SED1376 operation
         registerArrayWrite16(PLLCR, value & 0x3FBB);
         palmSysclksPerClk32 = sysclksPerClk32();
         setSed1376Attached(sed1376ClockConnected());

         if(value & 0x0008)
            pllSleepWait = 30;//The PLL shuts down 30 clocks of CLK32 after the DISPLL bit is set in the PLLCR
         else
            pllSleepWait = -1;//allow the CPU to cancel the shut down
         return;

      case ICR:
         registerArrayWrite16(ICR, value & 0xFF80);
         updateTouchState();
         checkPortDInterrupts();//this calls checkInterrupts() so it doesnt need to be called above
         return;

      case ILCR:
         setIlcr(value);
         return;

      case DRAMC:
         //somewhat unemulated
         //missing bit 7 and 6
         //debugLog("Set DRAMC, old value:0x%04X, new value:0x%04X, PC:0x%08X\n", registerArrayRead16(address), value, flx68000GetPc());
         registerArrayWrite16(DRAMC, value & 0xFF3F);
         updateCsdAddressLines();//the EDO bit can disable SDRAM access
         return;

      case DRAMMC:
         //unemulated, address line remapping, too CPU intensive to emulate
         registerArrayWrite16(address, value);
         return;

      case SDCTRL:
         //missing bits 13, 9, 8 and 7
         //debugLog("Set SDCTRL, old value:0x%04X, new value:0x%04X, PC:0x%08X\n", registerArrayRead16(address), value, flx68000GetPc());
         registerArrayWrite16(SDCTRL, value & 0xDC7F);
         updateCsdAddressLines();
         return;

      case CSA:{
            uint16_t oldCsa = registerArrayRead16(CSA);
            bool oldBootMode = chips[CHIP_A0_ROM].inBootMode;

            setCsa(value);

            //only reset address space if size changed, enabled/disabled or exiting boot mode
            if((value & 0x000F) != (oldCsa & 0x000F) || chips[CHIP_A0_ROM].inBootMode != oldBootMode)
               resetAddressSpace();
         }
         return;

      case CSB:{
            uint16_t oldCsb = registerArrayRead16(CSB);

            setCsb(value);

            //only reset address space if size changed or enabled/disabled
            if((value & 0x000F) != (oldCsb & 0x000F))
               resetAddressSpace();
         }
         return;

      case CSC:
         registerArrayWrite16(CSC, value & 0xF9FF);
         return;

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
         return;

      case CSGBA:
         //sets the starting location of ROM(0x10000000) and the PDIUSBD12 chip
         if((value & 0xFFFE) != registerArrayRead16(CSGBA)){
            setCsgba(value);
            resetAddressSpace();
         }
         return;

      case CSGBB:
         //sets the starting location of the SED1376(0x1FF80000)
         if((value & 0xFFFE) != registerArrayRead16(CSGBB)){
            setCsgbb(value);
            resetAddressSpace();
         }
         return;

      case CSGBC:
         registerArrayWrite16(CSGBC, value & 0xFFFE);
         return;

      case CSGBD:
         //sets the starting location of RAM(0x00000000)
         if((value & 0xFFFE) != registerArrayRead16(CSGBD)){
            setCsgbd(value);
            resetAddressSpace();
         }
         return;

      case CSUGBA:
         if((value & 0xF777) != registerArrayRead16(CSUGBA)){
            registerArrayWrite16(CSUGBA, value & 0xF777);
            //refresh all chip select address lines
            setCsgba(registerArrayRead16(CSGBA));
            setCsgbb(registerArrayRead16(CSGBB));
            setCsgbd(registerArrayRead16(CSGBD));
            resetAddressSpace();
         }
         return;

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
         return;

      case SPICONT1:
         setSpiCont1(value);
         return;

      case SPIINTCS:
         setSpiIntCs(value);
         return;

      case SPITEST:
         debugLog("SPITEST write not implented yet\n");
         return;

      case SPITXD:
         spi1TxFifoWrite(value);
         //check if SPI1 interrupts changed
         setSpiIntCs(registerArrayRead16(SPIINTCS));
         return;

      case SPICONT2:
         setSpiCont2(value);
         return;

      case SPIDATA2:
         //ignore writes when SPICONT2 ENABLE is not set
         if(registerArrayRead16(SPICONT2) & 0x0200)
            registerArrayWrite16(SPIDATA2, value);
         return;

      case PWMC1:
         setPwmc1(value);
         return;

      case PWMS1:
         //write only if PWM1 enabled
         if(registerArrayRead16(PWMC1) & 0x0010){
            pwm1FifoWrite(value >> 8);
            pwm1FifoWrite(value & 0xFF);
         }
         return;

      case USTCNT1:
         registerArrayWrite16(UBAUD1, value);
         //needs to recalculate interrupts here
         return;

      case UBAUD1:
         //just does timing stuff, should be OK to ignore
         registerArrayWrite16(UBAUD1, value & 0x2F3F);
         return;

      case NIPR1:
         //just does timing stuff, should be OK to ignore
         registerArrayWrite16(NIPR1, value & 0x87FF);
         return;

      case SPISPC:
         //SPI1 timing, unemulated for now

      case TCMP1:
      case TCMP2:
      case TPRER1:
      case TPRER2:
         //simple write, no actions needed
         registerArrayWrite16(address, value);
         return;

      default:
         //writeable bootloader region
         if(address >= 0xFC0){
            registerArrayWrite16(address, value);
            return;
         }

         if(!sandboxRunning())//dont double unknown accesses when sandbox is active
            printHwRegAccess(address, value, 16, true);
         return;
   }
}

void setHwRegister32(uint32_t address, uint32_t value){
#if !defined(EMU_NO_SAFETY)
   if((address & 0x0000F000) != 0x0000F000){
      setBusErrorTimeOut(address, true);
      return;
   }
#endif

   address &= 0x00000FFF;

   if(sandboxRunning() && address < 0xE00)
      printHwRegAccess(address, value, 32, true);

   switch(address){
      case RTCTIME:
      case RTCALRM:
         registerArrayWrite32(address, value & 0x1F3F003F);
         return;

      case IDR:
      case IPR:
         //write to read only register, do nothing
         return;

      case ISR:
         setIsr(value, true, true);
         return;

      case IMR:
         registerArrayWrite32(IMR, value & 0x00FFFFFF);//Palm OS writes to reserved bits 14 and 15
         registerArrayWrite32(ISR, registerArrayRead32(IPR) & ~registerArrayRead32(IMR));
         checkInterrupts();
         return;

      case LSSA:
         //simple write, no actions needed
         registerArrayWrite32(address, value);
         return;

      default:
         //writeable bootloader region
         if(address >= 0xFC0){
            registerArrayWrite32(address, value);
            return;
         }

         if(!sandboxRunning())//dont double unknown accesses when sandbox is active
            printHwRegAccess(address, value, 32, true);
         return;
   }
}

void resetHwRegisters(void){
   uint32_t oldRtc = registerArrayRead32(RTCTIME);//preserve RTCTIME
   uint16_t oldDayr = registerArrayRead16(DAYR);//preserve DAYR

   memset(palmReg, 0x00, REG_SIZE - BOOTLOADER_SIZE);
   palmSysclksPerClk32 = 0.0;
   clk32Counter = 0;
   pctlrCpuClockDivider = 1.0;
   pllSleepWait = -1;
   pllWakeWait = -1;
   timerCycleCounter[0] = 0.0;
   timerCycleCounter[1] = 0.0;
   timerStatusReadAcknowledge[0] = 0x0000;
   timerStatusReadAcknowledge[1] = 0x0000;
   portDInterruptLastValue = 0x00;
   memset(spi1RxFifo, 0x00, sizeof(spi1RxFifo));
   memset(spi1TxFifo, 0x00, sizeof(spi1TxFifo));
   spi1RxReadPosition = 0;
   spi1RxWritePosition = 0;
   spi1RxOverflowed = false;
   spi1TxReadPosition = 0;
   spi1TxWritePosition = 0;
   pwm1ClocksToNextSample = 0;
   memset(pwm1Fifo, 0x00, sizeof(pwm1Fifo));
   pwm1ReadPosition = 0;
   pwm1WritePosition = 0;

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
   registerArrayWrite32(IDR, 0x57000000);//value of IDR on actual hardware

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
   registerArrayWrite32(IMR, 0x00FFFFFF);//the data sheet says 0x00FFFFFF and 0x00FF3FFF, using 0x00FFFFFF because thats how its set on the device
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
   //registerArrayWrite16(PWMC1, 0x0020);//FIFOAV is controlled by getPwmc1()
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
   updateSdCardChipSelectStatus();
   updateBacklightAmplifierStatus();

   palmSysclksPerClk32 = sysclksPerClk32();
}

void setRtc(uint16_t days, uint8_t hours, uint8_t minutes, uint8_t seconds){
   registerArrayWrite32(RTCTIME, hours << 24 & 0x1F000000 | minutes << 16 & 0x003F0000 | seconds & 0x0000003F);
   registerArrayWrite16(DAYR, days & 0x01FF);
}
