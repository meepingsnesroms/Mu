#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "dbvz.h"
#include "emulator.h"
#include "m5XXBus.h"
#include "portability.h"
#include "flx68000.h"
#include "ads7846.h"
#include "sdCard.h"
#include "sed1376.h"
#include "audio/blip_buf.h"
#include "m68k/m68k.h"


#include "dbvzRegisterNames.c.h"


dbvz_chip_t dbvzChipSelects[DBVZ_CHIP_END];
uint8_t     dbvzReg[DBVZ_REG_SIZE];
uint16_t*   dbvzFramebuffer;
uint16_t    dbvzFramebufferWidth;
uint16_t    dbvzFramebufferHeight;

static bool     dbvzInterruptChanged;//reduces time wasted on checking interrupts that where updated to a new value identical to the old one, does not need to be in states
static double   dbvzSysclksPerClk32;//how many SYSCLK cycles before toggling the 32.768 kHz crystal
static uint32_t dbvzFrameClk32s;//how many CLK32s have happened in the current frame
static double   dbvzClk32Sysclks;//how many SYSCLKs have happened in the current CLK32
static int8_t   pllSleepWait;
static int8_t   pllWakeWait;
static uint32_t clk32Counter;
static double   pctlrCpuClockDivider;
static double   timerCycleCounter[2];
static uint16_t timerStatusReadAcknowledge[2];
static uint8_t  portDInterruptLastValue;//used for edge triggered interrupt timing
static uint16_t spi1RxFifo[9];
static uint16_t spi1TxFifo[9];
static uint8_t  spi1RxReadPosition;
static uint8_t  spi1RxWritePosition;
static bool     spi1RxOverflowed;
static uint8_t  spi1TxReadPosition;
static uint8_t  spi1TxWritePosition;
static int32_t  pwm1ClocksToNextSample;
static uint8_t  pwm1Fifo[6];
static uint8_t  pwm1ReadPosition;
static uint8_t  pwm1WritePosition;


static void checkInterrupts(void);
static void checkPortDInterrupts(void);
static void pllWakeCpuIfOff(void);
static double sysclksPerClk32(void);
static int32_t audioGetFramePercentIncrementFromClk32s(int32_t count);
static int32_t audioGetFramePercentIncrementFromSysclks(double count);
static int32_t audioGetFramePercentage(void);

#include "dbvzRegisterAccessors.c.h"
#include "dbvzTiming.c.h"

void dbvzLcdRender(void){
   static const uint16_t masterColorLut[16] = {0x746D, 0x6C0C, 0x63CB, 0x5B8A, 0x534A, 0x4AE9, 0x42A8, 0x3A67, 0x3A27, 0x31C6, 0x2985, 0x2144, 0x1904, 0x10A3, 0x0862, 0x0000};
   static const uint8_t bppLut[4] = {1, 2, 4, 0};
   uint16_t colorLut2Bpp[4];//stores indexes to masterColorLut
   uint32_t startAddress = registerArrayRead32(LSSA);
   uint8_t bitsPerPixel = bppLut[registerArrayRead8(LPICF) & 0x03];
   uint16_t pageWidth = registerArrayRead8(LVPW) * 2;//in bytes
   bool invertColors = registerArrayRead8(LPOLCF) & 0x01;
   uint8_t pixelShift = registerArrayRead8(LPOSR);
   uint16_t width = registerArrayRead16(LXMAX);
   uint16_t height = registerArrayRead16(LYMAX) + 1;
   uint16_t y;
   uint16_t x;

   //dont render if LCD controller is disabled
   if(!(registerArrayRead8(LCKCON) & 0x80)){
      memset(dbvzFramebuffer, 0x00, dbvzFramebufferWidth * dbvzFramebufferHeight * sizeof(uint16_t));
      return;
   }

   width = FAST_MIN(width, dbvzFramebufferWidth);
   height = FAST_MIN(height, dbvzFramebufferHeight);

   //TODO: cursor not implemented, not that anything will use a hardware terminal cursor on Palm OS
   //m500 ROM I am using has the hardware feature bit for inverting color when backlight is on disabled, the backlight does not invert the colors even though HW supports it

   switch(bitsPerPixel){
      case 1:
         for(y = 0; y < height; y++){
            for(x = 0; x < width / 16; x++){
               uint16_t dataUnit = m68k_read_memory_16(startAddress + y * pageWidth + x * 2);
               uint8_t index;

               for(index = 0; index < 16; index++)
                  if(x * 16 + index >= pixelShift)
                     dbvzFramebuffer[y * dbvzFramebufferWidth + x * 16 + index - pixelShift] = !!(dataUnit & 1 << 15 - index) == invertColors ? masterColorLut[0] : masterColorLut[15];
            }
         }
         break;

      case 2:
         colorLut2Bpp[0] = 0;
         colorLut2Bpp[1] = registerArrayRead8(LGPMR) & 0x0F;
         colorLut2Bpp[2] = registerArrayRead8(LGPMR) >> 4;
         colorLut2Bpp[3] = 15;

         for(y = 0; y < height; y++){
            for(x = 0; x < width / 8; x++){
               uint16_t dataUnit = m68k_read_memory_16(startAddress + y * pageWidth + x * 2);
               uint8_t index;

               for(index = 0; index < 8; index++)
                  if(x * 8 + index >= pixelShift)
                     dbvzFramebuffer[y * dbvzFramebufferWidth + x * 8 + index - pixelShift] = invertColors ? masterColorLut[15 - colorLut2Bpp[dataUnit >> 14 - index * 2 & 0x03]] : masterColorLut[colorLut2Bpp[dataUnit >> 14 - index * 2 & 0x03]];
            }
         }
         break;

      case 4:
         for(y = 0; y < height; y++){
            for(x = 0; x < width / 4; x++){
               uint16_t dataUnit = m68k_read_memory_16(startAddress + y * pageWidth + x * 2);
               uint8_t index;

               for(index = 0; index < 4; index++)
                  if(x * 4 + index >= pixelShift)
                     dbvzFramebuffer[y * dbvzFramebufferWidth + x * 4 + index - pixelShift] = invertColors ? masterColorLut[15 - (dataUnit >> 12 - index * 4 & 0x0F)] : masterColorLut[dataUnit >> 12 - index * 4 & 0x0F];
            }
         }
         break;

      default:
         debugLog("Invalid DBVZ LCD controller pixel depth %d!\n", bitsPerPixel);
         break;
   }
}

bool dbvzIsPllOn(void){
   return !(dbvzSysclksPerClk32 < 1.0);
}

bool m515BacklightAmplifierState(void){
   return !!(getPortKValue() & 0x02);
}

bool dbvzAreRegistersXXFFMapped(void){
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
            setIprIsrBit(DBVZ_INT_IRQ5);
         else
            clearIprIsrBit(DBVZ_INT_IRQ5);
         checkInterrupts();
      }

      //override over, put back real state
      updateTouchState();
      checkInterrupts();
   }
}

void m5XXRefreshTouchState(void){
   //called when ads7846PenIrqEnabled is changed
   updateTouchState();
   checkInterrupts();
}

void m5XXRefreshInputState(void){
   //update power button LED state incase palmMisc.batteryCharging changed
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

void dbvzSetBusErrorTimeOut(uint32_t address, bool isWrite){
   uint8_t scr = registerArrayRead8(SCR);
   debugLog("Bus error timeout at:0x%08X, PC:0x%08X\n", address, flx68000GetPc());
   registerArrayWrite8(SCR, scr | 0x80);
   if(scr & 0x10)
      flx68000BusError(address, isWrite);
}

void dbvzSetPrivilegeViolation(uint32_t address, bool isWrite){
   uint8_t scr = registerArrayRead8(SCR);
   debugLog("Privilege violation at:0x%08X, PC:0x%08X\n", address, flx68000GetPc());
   registerArrayWrite8(SCR, scr | 0x20);
   if(scr & 0x10)
      flx68000BusError(address, isWrite);
}

void dbvzSetWriteProtectViolation(uint32_t address){
   uint8_t scr = registerArrayRead8(SCR);
   debugLog("Write protect violation at:0x%08X, PC:0x%08X\n", address, flx68000GetPc());
   registerArrayWrite8(SCR, scr | 0x40);
   if(scr & 0x10)
      flx68000BusError(address, true);
}

static void pllWakeCpuIfOff(void){
   static const int8_t pllWaitTable[4] = {32, 48, 64, 96};

   //PLL is off and not already in the process of waking up
   if(!dbvzIsPllOn() && pllWakeWait == -1)
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

   //dont waste time if nothing changed
   if(!dbvzInterruptChanged)
      return;

   //static interrupts
   if(activeInterrupts & DBVZ_INT_EMIQ)
      intLevel = 7;//EMIQ - Emulator IRQ, has nothing to do with emulation, used for debugging on a dev board

   if(intLevel < 6 && activeInterrupts & (DBVZ_INT_TMR1 | DBVZ_INT_PWM1 | DBVZ_INT_IRQ6))
      intLevel = 6;

   if(intLevel < 5 && activeInterrupts & DBVZ_INT_IRQ5)
      intLevel = 5;

   if(intLevel < 4 && activeInterrupts & (DBVZ_INT_SPI2 | DBVZ_INT_UART1 | DBVZ_INT_WDT | DBVZ_INT_RTC | DBVZ_INT_KB | DBVZ_INT_RTI | DBVZ_INT_INT0 | DBVZ_INT_INT1 | DBVZ_INT_INT2 | DBVZ_INT_INT3))
      intLevel = 4;

   if(intLevel < 3 && activeInterrupts & DBVZ_INT_IRQ3)
      intLevel = 3;

   if(intLevel < 2 && activeInterrupts & DBVZ_INT_IRQ2)
      intLevel = 2;

   if(intLevel < 1 && activeInterrupts & DBVZ_INT_IRQ1)
      intLevel = 1;

   //configureable interrupts
   if(intLevel < spi1IrqLevel && activeInterrupts & DBVZ_INT_SPI1)
      intLevel = spi1IrqLevel;

   if(intLevel < uart2IrqLevel && activeInterrupts & DBVZ_INT_UART2)
      intLevel = uart2IrqLevel;

   if(intLevel < pwm2IrqLevel && activeInterrupts & DBVZ_INT_PWM2)
      intLevel = pwm2IrqLevel;

   if(intLevel < timer2IrqLevel && activeInterrupts & DBVZ_INT_TMR2)
      intLevel = timer2IrqLevel;

   //even masked interrupts turn off PCTLR, 4.5.4 Power Control Register MC68VZ328UM.pdf
   if(intLevel > 0 && registerArrayRead8(PCTLR) & 0x80){
      registerArrayWrite8(PCTLR, registerArrayRead8(PCTLR) & 0x1F);
      pctlrCpuClockDivider = 1.0;
   }

   //should be called even if intLevel is 0, that is how the interrupt state gets cleared
   flx68000SetIrq(intLevel);

   //no interrupts have changed since the last call to this function, which is now
   dbvzInterruptChanged = false;
}

static void checkPortDInterrupts(void){
   uint16_t icr = registerArrayRead16(ICR);
   uint8_t icrPolSwap = (!!(icr & 0x1000) << 7 | !!(icr & 0x2000) << 6 | !!(icr & 0x4000) << 5 | !!(icr & 0x8000) << 4) ^ 0xF0;//shifted to match port d layout
   uint8_t icrEdgeTriggered = !!(icr & 0x0100) << 7 | !!(icr & 0x0200) << 6 | !!(icr & 0x0400) << 5 | !!(icr & 0x0800) << 4;//shifted to match port d layout
   uint8_t portDInterruptValue = getPortDValue() ^ icrPolSwap;//not the same as the actual pin values, this already has all polarity swaps applied
   uint8_t portDInterruptEdgeTriggered = icrEdgeTriggered | registerArrayRead8(PDIRQEG);
   uint8_t portDInterruptEnabled = (~registerArrayRead8(PDSEL) & 0xF0) | registerArrayRead8(PDIRQEN);
   uint8_t portDIsInput = ~registerArrayRead8(PDDIR);
   uint8_t portDInterruptTriggered = portDInterruptValue & portDInterruptEnabled & portDIsInput & (~portDInterruptEdgeTriggered | ~portDInterruptLastValue & (dbvzIsPllOn() ? 0xFF : 0xF0));

   if(portDInterruptTriggered & 0x01)
      setIprIsrBit(DBVZ_INT_INT0);
   else if(!(portDInterruptEdgeTriggered & 0x01))
      clearIprIsrBit(DBVZ_INT_INT0);

   if(portDInterruptTriggered & 0x02)
      setIprIsrBit(DBVZ_INT_INT1);
   else if(!(portDInterruptEdgeTriggered & 0x02))
      clearIprIsrBit(DBVZ_INT_INT1);

   if(portDInterruptTriggered & 0x04)
      setIprIsrBit(DBVZ_INT_INT2);
   else if(!(portDInterruptEdgeTriggered & 0x04))
      clearIprIsrBit(DBVZ_INT_INT2);

   if(portDInterruptTriggered & 0x08)
      setIprIsrBit(DBVZ_INT_INT3);
   else if(!(portDInterruptEdgeTriggered & 0x08))
      clearIprIsrBit(DBVZ_INT_INT3);

   if(portDInterruptTriggered & 0x10)
      setIprIsrBit(DBVZ_INT_IRQ1);
   else if(!(portDInterruptEdgeTriggered & 0x10))
      clearIprIsrBit(DBVZ_INT_IRQ1);

   if(portDInterruptTriggered & 0x20)
      setIprIsrBit(DBVZ_INT_IRQ2);
   else if(!(portDInterruptEdgeTriggered & 0x20))
      clearIprIsrBit(DBVZ_INT_IRQ2);

   if(portDInterruptTriggered & 0x40)
      setIprIsrBit(DBVZ_INT_IRQ3);
   else if(!(portDInterruptEdgeTriggered & 0x40))
      clearIprIsrBit(DBVZ_INT_IRQ3);

   if(portDInterruptTriggered & 0x80)
      setIprIsrBit(DBVZ_INT_IRQ6);
   else if(!(portDInterruptEdgeTriggered & 0x80))
      clearIprIsrBit(DBVZ_INT_IRQ6);

   //active low/off level triggered interrupt(triggers on 0, not a pull down resistor)
   //The SELx, POLx, IQENx, and IQEGx bits have no effect on the functionality of KBENx, 10.4.5.8 Port D Keyboard Enable Register MC68VZ328UM.pdf
   //the above has finally been verified to be correct!
   if(registerArrayRead8(PDKBEN) & ~(getPortDValue() ^ registerArrayRead8(PDPOL)) & portDIsInput)
      setIprIsrBit(DBVZ_INT_KB);
   else
      clearIprIsrBit(DBVZ_INT_KB);

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

uint8_t dbvzGetRegister8(uint32_t address){
#if !defined(EMU_NO_SAFETY)
   if((address & 0x0000F000) != 0x0000F000){
      dbvzSetBusErrorTimeOut(address, false);
      return 0x00;
   }
#endif

   address &= 0x00000FFF;

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

      //LCD controller
      case LPICF:
      case LPOLCF:

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

         printHwRegAccess(address, 0, 8, false);
         return 0x00;
   }
}

uint16_t dbvzGetRegister16(uint32_t address){
#if !defined(EMU_NO_SAFETY)
   if((address & 0x0000F000) != 0x0000F000){
      dbvzSetBusErrorTimeOut(address, false);
      return 0x0000;
   }
#endif

   address &= 0x00000FFF;

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

      case UTX1:{
            uint16_t uart1TxStatus = registerArrayRead16(UTX1);
            uint8_t entrys = uart1TxFifoEntrys();

            uart1TxStatus |= (entrys == 0) << 15;
            uart1TxStatus |= (entrys < 4) << 14;
            uart1TxStatus |= (entrys < 8) << 13;

            return uart1TxStatus;
         }

      case UTX2:{
            uint16_t uart2TxStatus = registerArrayRead16(UTX2);
            uint8_t entrys = uart2TxFifoEntrys();

            uart2TxStatus |= (entrys == 0) << 15;
            uart2TxStatus |= (entrys < 4) << 14;
            uart2TxStatus |= (entrys < 8) << 13;

            return uart2TxStatus;
         }

      case PLLFSR:
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
      case SPISPC:
      case SPICONT2:
      case SPIDATA2:
      case USTCNT1:
      case UBAUD1:
      case UMISC1:
      case NIPR1:
      case USTCNT2:
      case UBAUD2:
      case UMISC2:
      case NIPR2:
      case HMARK:
      case LCXP:
      case PWMR:
      case LXMAX:
      case LYMAX:
         //simple read, no actions needed
         return registerArrayRead16(address);

      default:
         //bootloader
         if(address >= 0xE00)
            return registerArrayRead16(address);

         printHwRegAccess(address, 0, 16, false);
         return 0x0000;
   }
}

uint32_t dbvzGetRegister32(uint32_t address){
#if !defined(EMU_NO_SAFETY)
   if((address & 0x0000F000) != 0x0000F000){
      dbvzSetBusErrorTimeOut(address, false);
      return 0x00000000;
   }
#endif

   address &= 0x00000FFF;

   switch(address){
      case ISR:
      case IPR:
      case IMR:
      case RTCTIME:
      case IDR:
      case LSSA:
         //simple read, no actions needed
         return registerArrayRead32(address);

      default:
         //bootloader
         if(address >= 0xE00)
            return registerArrayRead32(address);

         printHwRegAccess(address, 0, 32, false);
         return 0x00000000;
   }
}

void dbvzSetRegister8(uint32_t address, uint8_t value){
#if !defined(EMU_NO_SAFETY)
   if((address & 0x0000F000) != 0x0000F000){
      dbvzSetBusErrorTimeOut(address, true);
      return;
   }
#endif

   address &= 0x00000FFF;

   switch(address){
      case SCR:
         setScr(value);
         return;

      case UTX1 + 1:
         //this is a 16 bit register but Palm OS writes to the 8 bit FIFO section alone
         //send byte and update interrupts if enabled
         if((registerArrayRead16(USTCNT1) & 0xA000) == 0xA000){
            uart1TxFifoWrite(value);
            updateUart1Interrupt();
         }
         return;

      case UTX2 + 1:
         //this is a 16 bit register but Palm OS writes to the 8 bit FIFO section alone
         //send byte and update interrupts if enabled
         if((registerArrayRead16(USTCNT2) & 0xA000) == 0xA000){
            uart2TxFifoWrite(value);
            updateUart2Interrupt();
         }
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

      case LPICF:
      case LPOLCF:
      case LPOSR:
         registerArrayWrite8(address, value & 0x0F);
         return;

      case LPXCD:
         registerArrayWrite8(LPXCD, value & 0x3F);
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
         m515SetSed1376Attached(sed1376ClockConnected());
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
         if(palmEmulatingM500)
            palmMisc.backlightLevel = !!(getPortGValue() & 0x02) * 100;
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
         if(!palmEmulatingM500)
            sed1376UpdateLcdStatus();
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

      //dragonball LCD controller
      case LVPW:
      case LCKCON:
      case LBLKC:
      case LACDRC:
      case LGPMR:
         //simple write, no actions needed
         registerArrayWrite8(address, value);
         return;

      default:
         //writeable bootloader region
         if(address >= 0xFC0){
            registerArrayWrite32(address, value);
            return;
         }

         printHwRegAccess(address, value, 8, true);
         return;
   }
}

void dbvzSetRegister16(uint32_t address, uint16_t value){
#if !defined(EMU_NO_SAFETY)
   if((address & 0x0000F000) != 0x0000F000){
      dbvzSetBusErrorTimeOut(address, true);
      return;
   }
#endif

   address &= 0x00000FFF;

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
         dbvzInterruptChanged |= true;
         checkInterrupts();
         return;
      case IMR + 2:
         //this is a 32 bit register but Palm OS writes to it as 16 bit chunks
         registerArrayWrite16(IMR + 2, value & 0xFFFF);//Palm OS writes to reserved bits 14 and 15
         registerArrayWrite16(ISR + 2, registerArrayRead16(IPR + 2) & ~registerArrayRead16(IMR + 2));
         dbvzInterruptChanged |= true;
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
            clearIprIsrBit(DBVZ_INT_WDT);
         return;

      case RTCISR:
         registerArrayWrite16(RTCISR, registerArrayRead16(RTCISR) & ~value);
         if(!(registerArrayRead16(RTCISR) & 0xFF00))
            clearIprIsrBit(DBVZ_INT_RTI);
         if(!(registerArrayRead16(RTCISR) & 0x003F))
            clearIprIsrBit(DBVZ_INT_RTC);
         checkInterrupts();
         return;

      case PLLFSR:
         setPllfsr(value);
         return;

      case PLLCR:
         //CLKEN is required for SED1376 operation
         registerArrayWrite16(PLLCR, value & 0x3FBB);
         dbvzSysclksPerClk32 = sysclksPerClk32();
         m515SetSed1376Attached(sed1376ClockConnected());

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
            bool oldBootMode = dbvzChipSelects[DBVZ_CHIP_A0_ROM].inBootMode;

            setCsa(value);

            //only reset address space if size changed, enabled/disabled or exiting boot mode
            if((value & 0x000F) != (oldCsa & 0x000F) || dbvzChipSelects[DBVZ_CHIP_A0_ROM].inBootMode != oldBootMode)
               dbvzResetAddressSpace();
         }
         return;

      case CSB:{
            uint16_t oldCsb = registerArrayRead16(CSB);

            setCsb(value);

            //only reset address space if size changed or enabled/disabled
            if((value & 0x000F) != (oldCsb & 0x000F))
               dbvzResetAddressSpace();
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
               dbvzResetAddressSpace();
         }
         return;

      case CSGBA:
         //sets the starting location of ROM(0x10000000) and the PDIUSBD12 chip
         if((value & 0xFFFE) != registerArrayRead16(CSGBA)){
            setCsgba(value);
            dbvzResetAddressSpace();
         }
         return;

      case CSGBB:
         //sets the starting location of the SED1376(0x1FF80000)
         if((value & 0xFFFE) != registerArrayRead16(CSGBB)){
            setCsgbb(value);
            dbvzResetAddressSpace();
         }
         return;

      case CSGBC:
         registerArrayWrite16(CSGBC, value & 0xFFFE);
         return;

      case CSGBD:
         //sets the starting location of RAM(0x00000000)
         if((value & 0xFFFE) != registerArrayRead16(CSGBD)){
            setCsgbd(value);
            dbvzResetAddressSpace();
         }
         return;

      case CSUGBA:
         if((value & 0xF777) != registerArrayRead16(CSUGBA)){
            registerArrayWrite16(CSUGBA, value & 0xF777);
            //refresh all chip select address lines
            setCsgba(registerArrayRead16(CSGBA));
            setCsgbb(registerArrayRead16(CSGBB));
            setCsgbd(registerArrayRead16(CSGBD));
            dbvzResetAddressSpace();
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
               dbvzResetAddressSpace();
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
         if(registerArrayRead16(SPICONT1) & 0x0200){
            spi1TxFifoWrite(value);
            //check if SPI1 interrupts changed
            setSpiIntCs(registerArrayRead16(SPIINTCS));
         }
         return;

      case SPICONT2:
         setSpiCont2(value);
         return;

      case SPIDATA2:
         //ignore writes when SPICONT2 ENABLE is not set
         if(registerArrayRead16(SPICONT2) & 0x0200)
            registerArrayWrite16(SPIDATA2, value);
         return;

      case USTCNT1:
         setUstcnt1(value);
         return;

      case UBAUD1:
         //just does timing stuff, should be OK to ignore
         registerArrayWrite16(UBAUD1, value & 0x2F3F);
         return;

      case UMISC1:
         //TODO: most of the bits here are for factory testing and can be ignored but not all of them can be
         registerArrayWrite16(UMISC1, value & 0xFCFC);
         return;

      case UTX1:
         registerArrayWrite16(UTX1, value & 0x1F00);

         //send byte and update interrupts if enabled
         if((registerArrayRead16(USTCNT1) & 0xA000) == 0xA000){
            uart1TxFifoWrite(value & 0x1000 ? value & 0xFF : EMU_SERIAL_BREAK);
            updateUart1Interrupt();
         }
         return;

      case USTCNT2:
         setUstcnt2(value);
         return;

      case UBAUD2:
         //just does timing stuff, should be OK to ignore
         registerArrayWrite16(UBAUD2, value & 0x2F3F);
         return;

      case UMISC2:
         //TODO: most of the bits here are for factory testing and can be ignored but not all of them can be
         registerArrayWrite16(UMISC2, value & 0xFCFC);
         return;

      case UTX2:
         registerArrayWrite16(UTX2, value & 0x1F00);

         //send byte and update interrupts if enabled
         if((registerArrayRead16(USTCNT2) & 0xA000) == 0xA000){
            uart2TxFifoWrite(value & 0x1000 ? value & 0xFF : EMU_SERIAL_BREAK);
            updateUart2Interrupt();
         }
         return;

      case HMARK:
         registerArrayWrite16(HMARK, value & 0x0F0F);
         updateUart2Interrupt();
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

      case NIPR1:
         //just does timing stuff, should be OK to ignore
         registerArrayWrite16(NIPR1, value & 0x87FF);
         return;

      case LXMAX:
         registerArrayWrite16(LXMAX, value & 0x03F0);
         return;

      case LYMAX:
         registerArrayWrite16(LYMAX, value & 0x01FF);
         return;

      case LCXP:
         registerArrayWrite16(LCXP, value & 0xC3FF);
         return;

      case LCYP:
         registerArrayWrite16(LCYP, value & 0x01FF);
         return;

      case LCWCH:
         registerArrayWrite16(LCWCH, value & 0x1F1F);
         return;

      case LRRA:
         registerArrayWrite16(LRRA, value & 0x03FF);
         return;

      case PWMR:
         registerArrayWrite16(PWMR, value & 0x07FF);
         return;

      case SPISPC:
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

         printHwRegAccess(address, value, 16, true);
         return;
   }
}

void dbvzSetRegister32(uint32_t address, uint32_t value){
#if !defined(EMU_NO_SAFETY)
   if((address & 0x0000F000) != 0x0000F000){
      dbvzSetBusErrorTimeOut(address, true);
      return;
   }
#endif

   address &= 0x00000FFF;

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
         dbvzInterruptChanged |= true;
         checkInterrupts();
         return;

      case LSSA:
         registerArrayWrite32(address, value & 0xFFFFFFFE);
         return;

      default:
         //writeable bootloader region
         if(address >= 0xFC0){
            registerArrayWrite32(address, value);
            return;
         }

         printHwRegAccess(address, value, 32, true);
         return;
   }
}

void dbvzReset(void){
   uint32_t oldRtc = registerArrayRead32(RTCTIME);//preserve RTCTIME
   uint16_t oldDayr = registerArrayRead16(DAYR);//preserve DAYR

   dbvzInterruptChanged = false;//speed hack variable
   memset(dbvzReg, 0x00, DBVZ_REG_SIZE - DBVZ_BOOTLOADER_SIZE);
   dbvzSysclksPerClk32 = 0.0;
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

   memset(dbvzChipSelects, 0x00, sizeof(dbvzChipSelects));
   //all chip selects are disabled at boot and CSA0 is mapped to 0x00000000 and covers the entire address range until CSA is set enabled
   dbvzChipSelects[DBVZ_CHIP_A0_ROM].inBootMode = true;

   //default sizes
   dbvzChipSelects[DBVZ_CHIP_A0_ROM].lineSize = 0x20000;
   dbvzChipSelects[DBVZ_CHIP_A1_USB].lineSize = 0x20000;
   dbvzChipSelects[DBVZ_CHIP_B0_SED].lineSize = 0x20000;
   dbvzChipSelects[DBVZ_CHIP_DX_RAM].lineSize = 0x8000;

   //masks for reading and writing
   dbvzChipSelects[DBVZ_CHIP_A0_ROM].mask = 0x003FFFFF;//4mb
   dbvzChipSelects[DBVZ_CHIP_A1_USB].mask = 0x00000002;//A1 is used as USB chip A0
   dbvzChipSelects[DBVZ_CHIP_B0_SED].mask = 0x0001FFFF;
   dbvzChipSelects[DBVZ_CHIP_DX_RAM].mask = 0x00000000;//16mb, no RAM enabled until the DRAM module is initialized

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
   if(!palmEmulatingM500)
      sed1376UpdateLcdStatus();

   dbvzSysclksPerClk32 = sysclksPerClk32();

   dbvzResetAddressSpace();
   flx68000Reset();
}

void dbvzLoadBootloader(uint8_t* data, uint32_t size){
   uint16_t index;

   if(!data)
      size = 0;

   size = FAST_MIN(size, DBVZ_BOOTLOADER_SIZE);

   //copy size bytes from buffer to bootloader area
   for(index = 0; index < size; index++)
      registerArrayWrite8(DBVZ_REG_SIZE - DBVZ_BOOTLOADER_SIZE + index, data[index]);

   //fill remainig space with 0x00
   for(; index < DBVZ_BOOTLOADER_SIZE; index++)
      registerArrayWrite8(DBVZ_REG_SIZE - DBVZ_BOOTLOADER_SIZE + index, 0x00);
}

void dbvzSetRtc(uint16_t days, uint8_t hours, uint8_t minutes, uint8_t seconds){
   registerArrayWrite32(RTCTIME, hours << 24 & 0x1F000000 | minutes << 16 & 0x003F0000 | seconds & 0x0000003F);
   registerArrayWrite16(DAYR, days & 0x01FF);
}

uint32_t dbvzStateSize(void){
   uint32_t size = 0;

   size += flx68000StateSize();
   size += DBVZ_REG_SIZE;//hardware registers
   size += DBVZ_TOTAL_MEMORY_BANKS;
   size += sizeof(uint32_t) * 4 * DBVZ_CHIP_END;//chip select states
   size += sizeof(uint8_t) * 5 * DBVZ_CHIP_END;//chip select states
   size += sizeof(uint64_t) * 5;//32.32 fixed point double, timerXCycleCounter and CPU cycle timers
   size += sizeof(int8_t);//pllSleepWait
   size += sizeof(int8_t);//pllWakeWait
   size += sizeof(uint32_t);//clk32Counter
   size += sizeof(uint64_t);//pctlrCpuClockDivider
   size += sizeof(uint16_t) * 2;//timerStatusReadAcknowledge
   size += sizeof(uint8_t);//portDInterruptLastValue
   size += sizeof(uint16_t) * 9;//RX 8 * 16 SPI1 FIFO, 1 index is for FIFO full
   size += sizeof(uint16_t) * 9;//TX 8 * 16 SPI1 FIFO, 1 index is for FIFO full
   size += sizeof(uint8_t) * 5;//spi1(R/T)x(Read/Write)Position / spi1RxOverflowed
   size += sizeof(int32_t);//pwm1ClocksToNextSample
   size += sizeof(uint8_t) * 6;//pwm1Fifo[6]
   size += sizeof(uint8_t) * 2;//pwm1(Read/Write)

   return size;
}

void dbvzSaveState(uint8_t* data){
   uint32_t offset = 0;
   uint8_t index;

   //CPU core
   flx68000SaveState(data + offset);
   offset += flx68000StateSize();

   //memory
   memcpy(data + offset, dbvzReg, DBVZ_REG_SIZE);
   swap16BufferIfLittle(data + offset, DBVZ_REG_SIZE / sizeof(uint16_t));
   offset += DBVZ_REG_SIZE;
   memcpy(data + offset, dbvzBankType, DBVZ_TOTAL_MEMORY_BANKS);
   offset += DBVZ_TOTAL_MEMORY_BANKS;
   for(index = DBVZ_CHIP_BEGIN; index < DBVZ_CHIP_END; index++){
      writeStateValue8(data + offset, dbvzChipSelects[index].enable);
      offset += sizeof(uint8_t);
      writeStateValue32(data + offset, dbvzChipSelects[index].start);
      offset += sizeof(uint32_t);
      writeStateValue32(data + offset, dbvzChipSelects[index].lineSize);
      offset += sizeof(uint32_t);
      writeStateValue32(data + offset, dbvzChipSelects[index].mask);
      offset += sizeof(uint32_t);
      writeStateValue8(data + offset, dbvzChipSelects[index].inBootMode);
      offset += sizeof(uint8_t);
      writeStateValue8(data + offset, dbvzChipSelects[index].readOnly);
      offset += sizeof(uint8_t);
      writeStateValue8(data + offset, dbvzChipSelects[index].readOnlyForProtectedMemory);
      offset += sizeof(uint8_t);
      writeStateValue8(data + offset, dbvzChipSelects[index].supervisorOnlyProtectedMemory);
      offset += sizeof(uint8_t);
      writeStateValue32(data + offset, dbvzChipSelects[index].unprotectedSize);
      offset += sizeof(uint32_t);
   }

   //timing
   writeStateValueDouble(data + offset, dbvzSysclksPerClk32);
   offset += sizeof(uint64_t);
   writeStateValueDouble(data + offset, palmCycleCounter);
   offset += sizeof(uint64_t);
   writeStateValueDouble(data + offset, palmClockMultiplier);
   offset += sizeof(uint64_t);
   writeStateValue8(data + offset, pllSleepWait);
   offset += sizeof(int8_t);
   writeStateValue8(data + offset, pllWakeWait);
   offset += sizeof(int8_t);
   writeStateValue32(data + offset, clk32Counter);
   offset += sizeof(uint32_t);
   writeStateValueDouble(data + offset, pctlrCpuClockDivider);
   offset += sizeof(uint64_t);
   writeStateValueDouble(data + offset, timerCycleCounter[0]);
   offset += sizeof(uint64_t);
   writeStateValueDouble(data + offset, timerCycleCounter[1]);
   offset += sizeof(uint64_t);
   writeStateValue16(data + offset, timerStatusReadAcknowledge[0]);
   offset += sizeof(uint16_t);
   writeStateValue16(data + offset, timerStatusReadAcknowledge[1]);
   offset += sizeof(uint16_t);
   writeStateValue8(data + offset, portDInterruptLastValue);
   offset += sizeof(uint8_t);

   //SPI1
   for(index = 0; index < 9; index++){
      writeStateValue16(data + offset, spi1RxFifo[index]);
      offset += sizeof(uint16_t);
   }
   for(index = 0; index < 9; index++){
      writeStateValue16(data + offset, spi1TxFifo[index]);
      offset += sizeof(uint16_t);
   }
   writeStateValue8(data + offset, spi1RxReadPosition);
   offset += sizeof(uint8_t);
   writeStateValue8(data + offset, spi1RxWritePosition);
   offset += sizeof(uint8_t);
   writeStateValue8(data + offset, spi1RxOverflowed);
   offset += sizeof(uint8_t);
   writeStateValue8(data + offset, spi1TxReadPosition);
   offset += sizeof(uint8_t);
   writeStateValue8(data + offset, spi1TxWritePosition);
   offset += sizeof(uint8_t);

   //PWM1, audio
   writeStateValue32(data + offset, pwm1ClocksToNextSample);
   offset += sizeof(int32_t);
   for(index = 0; index < 6; index++){
      writeStateValue8(data + offset, pwm1Fifo[index]);
      offset += sizeof(uint8_t);
   }
   writeStateValue8(data + offset, pwm1ReadPosition);
   offset += sizeof(uint8_t);
   writeStateValue8(data + offset, pwm1WritePosition);
   offset += sizeof(uint8_t);
}

void dbvzLoadState(uint8_t* data){
   uint32_t offset = 0;
   uint8_t index;

   //CPU core
   flx68000LoadState(data + offset);
   offset += flx68000StateSize();

   //memory
   memcpy(dbvzReg, data + offset, DBVZ_REG_SIZE);
   swap16BufferIfLittle(dbvzReg, DBVZ_REG_SIZE / sizeof(uint16_t));
   offset += DBVZ_REG_SIZE;
   memcpy(dbvzBankType, data + offset, DBVZ_TOTAL_MEMORY_BANKS);
   offset += DBVZ_TOTAL_MEMORY_BANKS;
   for(index = DBVZ_CHIP_BEGIN; index < DBVZ_CHIP_END; index++){
      dbvzChipSelects[index].enable = readStateValue8(data + offset);
      offset += sizeof(uint8_t);
      dbvzChipSelects[index].start = readStateValue32(data + offset);
      offset += sizeof(uint32_t);
      dbvzChipSelects[index].lineSize = readStateValue32(data + offset);
      offset += sizeof(uint32_t);
      dbvzChipSelects[index].mask = readStateValue32(data + offset);
      offset += sizeof(uint32_t);
      dbvzChipSelects[index].inBootMode = readStateValue8(data + offset);
      offset += sizeof(uint8_t);
      dbvzChipSelects[index].readOnly = readStateValue8(data + offset);
      offset += sizeof(uint8_t);
      dbvzChipSelects[index].readOnlyForProtectedMemory = readStateValue8(data + offset);
      offset += sizeof(uint8_t);
      dbvzChipSelects[index].supervisorOnlyProtectedMemory = readStateValue8(data + offset);
      offset += sizeof(uint8_t);
      dbvzChipSelects[index].unprotectedSize = readStateValue32(data + offset);
      offset += sizeof(uint32_t);
   }

   //timing
   dbvzSysclksPerClk32 = readStateValueDouble(data + offset);
   offset += sizeof(uint64_t);
   palmCycleCounter = readStateValueDouble(data + offset);
   offset += sizeof(uint64_t);
   palmClockMultiplier = readStateValueDouble(data + offset);
   offset += sizeof(uint64_t);
   pllSleepWait = readStateValue8(data + offset);
   offset += sizeof(int8_t);
   pllWakeWait = readStateValue8(data + offset);
   offset += sizeof(int8_t);
   clk32Counter = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   pctlrCpuClockDivider = readStateValueDouble(data + offset);
   offset += sizeof(uint64_t);
   timerCycleCounter[0] = readStateValueDouble(data + offset);
   offset += sizeof(uint64_t);
   timerCycleCounter[1] = readStateValueDouble(data + offset);
   offset += sizeof(uint64_t);
   timerStatusReadAcknowledge[0] = readStateValue16(data + offset);
   offset += sizeof(uint16_t);
   timerStatusReadAcknowledge[1] = readStateValue16(data + offset);
   offset += sizeof(uint16_t);
   portDInterruptLastValue = readStateValue8(data + offset);
   offset += sizeof(uint8_t);

   //SPI1
   for(index = 0; index < 9; index++){
      spi1RxFifo[index] = readStateValue16(data + offset);
      offset += sizeof(uint16_t);
   }
   for(index = 0; index < 9; index++){
      spi1TxFifo[index] = readStateValue16(data + offset);
      offset += sizeof(uint16_t);
   }
   spi1RxReadPosition = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   spi1RxWritePosition = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   spi1RxOverflowed = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   spi1TxReadPosition = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   spi1TxWritePosition = readStateValue8(data + offset);
   offset += sizeof(uint8_t);

   //PWM1, audio
   pwm1ClocksToNextSample = readStateValue32(data + offset);
   offset += sizeof(int32_t);
   for(index = 0; index < 6; index++){
      pwm1Fifo[index] = readStateValue8(data + offset);
      offset += sizeof(uint8_t);
   }
   pwm1ReadPosition = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   pwm1WritePosition = readStateValue8(data + offset);
   offset += sizeof(uint8_t);

   //UART1, cant load state while beaming
   uart1RxFifoFlush();

   //UART2, cant load state while syncing
   uart2RxFifoFlush();
}

void dbvzLoadStateFinished(void){
   flx68000LoadStateFinished();
}

void dbvzExecute(void){
   uint32_t samples;

   //I/O
   m5XXRefreshInputState();

   //CPU
   dbvzFrameClk32s = 0;
   for(; palmCycleCounter < (double)M5XX_CRYSTAL_FREQUENCY / EMU_FPS; palmCycleCounter += 1.0){
      uint8_t cpuTimeSegments;

      dbvzBeginClk32();

      for(cpuTimeSegments = 0; cpuTimeSegments < 2; cpuTimeSegments++){
         double cyclesRemaining = dbvzSysclksPerClk32 / 2.0;

         while(cyclesRemaining >= 1.0){
            double sysclks = FAST_MIN(cyclesRemaining, DBVZ_SYSCLK_PRECISION);
            int32_t cpuCycles = sysclks * pctlrCpuClockDivider * palmClockMultiplier;

            if(cpuCycles > 0)
               flx68000Execute(cpuCycles);
            dbvzAddSysclks(sysclks);

            cyclesRemaining -= sysclks;
         }

         //toggle CLK32 bit in PLLFSR, it indicates the current state of CLK32 so it must start false and be changed to true in the middle of CLK32
         registerArrayWrite16(PLLFSR, registerArrayRead16(PLLFSR) ^ 0x8000);
      }

      dbvzEndClk32();
      dbvzFrameClk32s++;
   }
   palmCycleCounter -= (double)M5XX_CRYSTAL_FREQUENCY / EMU_FPS;

   //audio
   blip_end_frame(palmAudioResampler, blip_clocks_needed(palmAudioResampler, AUDIO_SAMPLES_PER_FRAME));
   blip_read_samples(palmAudioResampler, palmAudio, AUDIO_SAMPLES_PER_FRAME, true);
   MULTITHREAD_LOOP(samples) for(samples = 0; samples < AUDIO_SAMPLES_PER_FRAME * 2; samples += 2)
      palmAudio[samples + 1] = palmAudio[samples];
}
