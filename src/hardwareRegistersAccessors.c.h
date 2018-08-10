//declare I/O port functions in advance
static inline uint8_t getPortAValue();
static inline uint8_t getPortBValue();
static inline uint8_t getPortCValue();
static inline uint8_t getPortDValue();
static inline uint8_t getPortEValue();
static inline uint8_t getPortFValue();
static inline uint8_t getPortGValue();
static inline uint8_t getPortJValue();
static inline uint8_t getPortKValue();
static inline uint8_t getPortMValue();

//basic accessors
static inline uint8_t registerArrayRead8(uint32_t address){return BUFFER_READ_8(palmReg, address, 0xFFF);}
static inline uint16_t registerArrayRead16(uint32_t address){return BUFFER_READ_16(palmReg, address, 0xFFF);}
static inline uint32_t registerArrayRead32(uint32_t address){return BUFFER_READ_32(palmReg, address, 0xFFF);}
static inline void registerArrayWrite8(uint32_t address, uint8_t value){BUFFER_WRITE_8(palmReg, address, 0xFFF, value);}
static inline void registerArrayWrite16(uint32_t address, uint16_t value){BUFFER_WRITE_16(palmReg, address, 0xFFF, value);}
static inline void registerArrayWrite32(uint32_t address, uint32_t value){BUFFER_WRITE_32(palmReg, address, 0xFFF, value);}

//register setters
static inline void setIprIsrBit(uint32_t interruptBit){
   //allows for setting an interrupt with masking by IMR and logging in IPR
   uint32_t newIpr = registerArrayRead32(IPR) | interruptBit;
   registerArrayWrite32(IPR, newIpr);
   registerArrayWrite32(ISR, newIpr & ~registerArrayRead32(IMR));
}

static inline void clearIprIsrBit(uint32_t interruptBit){
   uint32_t newIpr = registerArrayRead32(IPR) & ~interruptBit;
   registerArrayWrite32(IPR, newIpr);
   registerArrayWrite32(ISR, newIpr & ~registerArrayRead32(IMR));
}

static inline void setCsa(uint16_t value){
   chips[CHIP_A_ROM].enable = value & 0x0001;
   chips[CHIP_A_ROM].readOnly = value & 0x8000;
   chips[CHIP_A_ROM].lineSize = 0x20000/*128kb*/ << (value >> 1 & 0x0007);

   //CSA is now just a normal chipselect
   if(chips[CHIP_A_ROM].enable && chips[CHIP_A_ROM].inBootMode)
      chips[CHIP_A_ROM].inBootMode = false;

   registerArrayWrite16(CSA, value & 0x81FF);
}

static inline void setCsb(uint16_t value){
   uint16_t csControl1 = registerArrayRead16(CSCTRL1);

   chips[CHIP_B_SED].enable = value & 0x0001;
   chips[CHIP_B_SED].readOnly = value & 0x8000;
   chips[CHIP_B_SED].lineSize = 0x20000/*128kb*/ << (value >> 1 & 0x0007);

   //attributes
   chips[CHIP_B_SED].supervisorOnlyProtectedMemory = value & 0x4000;
   chips[CHIP_B_SED].readOnlyForProtectedMemory = value & 0x2000;
   if(csControl1 & 0x4000 && csControl1 & 0x0001)
      chips[CHIP_B_SED].unprotectedSize = chips[CHIP_B_SED].lineSize / (1 << 7 - ((value >> 11 & 0x0003) | 0x0004));
   else
      chips[CHIP_B_SED].unprotectedSize = chips[CHIP_B_SED].lineSize / (1 << 7 - (value >> 11 & 0x0003));

   registerArrayWrite16(CSB, value & 0xF9FF);
}

static inline void setCsc(uint16_t value){
   uint16_t csControl1 = registerArrayRead16(CSCTRL1);
   bool csdDramBit = registerArrayRead16(CSD) & 0x0200;

   chips[CHIP_C_USB].enable = value & 0x0001;
   chips[CHIP_C_USB].readOnly = value & 0x8000;
   if(csControl1 & 0x0040 && csdDramBit)
      chips[CHIP_C_USB].lineSize = 0x800000/*8mb*/ << (value >> 1 & 0x0001);
   else
      chips[CHIP_C_USB].lineSize = 0x8000/*32kb*/ << (value >> 1 & 0x0007);

   //attributes
   chips[CHIP_C_USB].supervisorOnlyProtectedMemory = value & 0x4000;
   chips[CHIP_C_USB].readOnlyForProtectedMemory = value & 0x2000;
   if(csControl1 & 0x4000 && csControl1 & 0x0004)
      chips[CHIP_C_USB].unprotectedSize = chips[CHIP_C_USB].lineSize / (1 << 7 - ((value >> 11 & 0x0003) | 0x0004));
   else
      chips[CHIP_C_USB].unprotectedSize = chips[CHIP_C_USB].lineSize / (1 << 7 - (value >> 11 & 0x0003));

   registerArrayWrite16(CSC, value & 0xF9FF);
}

static inline void setCsd(uint16_t value){
   uint16_t csControl1 = registerArrayRead16(CSCTRL1);

   chips[CHIP_D_RAM].enable = value & 0x0001;
   chips[CHIP_D_RAM].readOnly = value & 0x8000;
   if(csControl1 & 0x0040 && value & 0x0200)
      chips[CHIP_D_RAM].lineSize = 0x800000/*8mb*/ << (value >> 1 & 0x0001);
   else
      chips[CHIP_D_RAM].lineSize = 0x8000/*32kb*/ << (value >> 1 & 0x0007);

   //attributes
   chips[CHIP_D_RAM].supervisorOnlyProtectedMemory = value & 0x4000;
   chips[CHIP_D_RAM].readOnlyForProtectedMemory = value & 0x2000;
   if(csControl1 & 0x4000 && csControl1 & 0x0010)
      chips[CHIP_D_RAM].unprotectedSize = chips[CHIP_D_RAM].lineSize / (1 << 7 - ((value >> 11 & 0x0003) | 0x0004));
   else
      chips[CHIP_D_RAM].unprotectedSize = chips[CHIP_D_RAM].lineSize / (1 << 7 - (value >> 11 & 0x0003));

   registerArrayWrite16(CSD, value);
}

static inline void setCsgba(uint16_t value){
   uint16_t csugba = registerArrayRead16(CSUGBA);

   //add extra address bits if enabled
   if(csugba & 0x8000)
      chips[CHIP_A_ROM].start = (csugba >> 12 & 0x0007) << 29 | value >> 1 << 14;
   else
      chips[CHIP_A_ROM].start = value >> 1 << 14;

   registerArrayWrite16(CSGBA, value & 0xFFFE);
}

static inline void setCsgbb(uint16_t value){
   uint16_t csugba = registerArrayRead16(CSUGBA);

   //add extra address bits if enabled
   if(csugba & 0x8000)
      chips[CHIP_B_SED].start = (csugba >> 8 & 0x0007) << 29 | value >> 1 << 14;
   else
      chips[CHIP_B_SED].start = value >> 1 << 14;

   registerArrayWrite16(CSGBB, value & 0xFFFE);
}

static inline void setCsgbc(uint16_t value){
   uint16_t csugba = registerArrayRead16(CSUGBA);

   //add extra address bits if enabled
   if(csugba & 0x8000)
      chips[CHIP_C_USB].start = (csugba >> 4 & 0x0007) << 29 | value >> 1 << 14;
   else
      chips[CHIP_C_USB].start = value >> 1 << 14;

   registerArrayWrite16(CSGBC, value & 0xFFFE);
}

static inline void setCsgbd(uint16_t value){
   uint16_t csugba = registerArrayRead16(CSUGBA);

   //add extra address bits if enabled
   if(csugba & 0x8000)
      chips[CHIP_D_RAM].start = (csugba & 0x0007) << 29 | value >> 1 << 14;
   else
      chips[CHIP_D_RAM].start = value >> 1 << 14;

   registerArrayWrite16(CSGBD, value & 0xFFFE);
}

static inline void updateCsdAddressLines(){
   uint16_t dramc = registerArrayRead16(DRAMC);
   uint16_t sdctrl = registerArrayRead16(SDCTRL);

   if(registerArrayRead16(CSD) & 0x0200 && sdctrl & 0x8000 && dramc & 0x8000 && !(dramc & 0x0400)){
      //this register can remap address lines, that behavior is way too CPU intensive and complicated so only the "memory testing" and "correct" behavior is being emulated
      chips[CHIP_D_RAM].mask = 0x003FFFFF;

      //address line 23 is enabled
      if((sdctrl & 0x000C) == 0x0008)
         chips[CHIP_D_RAM].mask |= 0x00800000;

      //address line 22 is enabled
      if((sdctrl & 0x0030) == 0x0010)
         chips[CHIP_D_RAM].mask |= 0x00400000;
   }
   else{
      //RAM is not enabled properly
      chips[CHIP_D_RAM].mask = 0x00000000;
   }
}

static inline void setPllfsr(uint16_t value){
   uint16_t oldPllfsr = registerArrayRead16(PLLFSR);
   if(!(oldPllfsr & 0x4000)){
      //frequency protect bit not set
      registerArrayWrite16(PLLFSR, (value & 0x4FFF) | (oldPllfsr & 0x8000));//preserve CLK32 bit
      recalculateCpuSpeed();
   }
}

static inline void setScr(uint8_t value){
   uint8_t oldScr = registerArrayRead8(SCR);
   uint8_t newScr = value & 0x1F;

   //preserve privilege violation, write protect violation and bus error timeout
   newScr |= oldScr & 0xE0;

   //clear violations on writing 1 to them
   newScr &= ~(value & 0xE0);

   chips[CHIP_REGISTERS].supervisorOnlyProtectedMemory = value & 0x08;

   registerArrayWrite8(SCR, newScr);//must be written before calling setRegisterFFFFAccessMode
   if((newScr & 0x04) != (oldScr & 0x04)){
      if(newScr & 0x04)
         setRegisterXXFFAccessMode();
      else
         setRegisterFFFFAccessMode();
   }
}

static inline void setIlcr(uint16_t value){
   uint16_t oldIlcr = registerArrayRead16(ILCR);
   uint16_t newIlcr = 0;

   //SPI1, interrupt level 0 an 7 are invalid values that cause the register not to update
   if((value & 0x7000) != 0x0000 && (value & 0x7000) != 0x7000)
      newIlcr |= value & 0x7000;
   else
      newIlcr |= oldIlcr & 0x7000;

   //UART2, interrupt level 0 an 7 are invalid values that cause the register not to update
   if((value & 0x0700) != 0x0000 && (value & 0x0700) != 0x0700)
      newIlcr |= value & 0x0700;
   else
      newIlcr |= oldIlcr & 0x0700;

   //PWM2, interrupt level 0 an 7 are invalid values that cause the register not to update
   if((value & 0x0070) != 0x0000 && (value & 0x0070) != 0x0070)
      newIlcr |= value & 0x0070;
   else
      newIlcr |= oldIlcr & 0x0070;

   //TMR2, interrupt level 0 an 7 are invalid values that cause the register not to update
   if((value & 0x0007) != 0x0000 && (value & 0x0007) != 0x0007)
      newIlcr |= value & 0x0007;
   else
      newIlcr |= oldIlcr & 0x0007;

   registerArrayWrite16(ILCR, newIlcr);
}

static inline void setSpiCont1(uint16_t value){
   //only master mode is implemented!!!
   uint16_t oldSpiCont1 = registerArrayRead16(SPICONT1);

   //do a transfer
   if(value & oldSpiCont1 & 0x0200 && value & 0x0100){
      //enabled and exchange set
      uint8_t bitCount = (value & 0x000F) + 1;
      uint16_t startBit = 1 << (bitCount - 1);
      uint16_t currentTxFifoEntry = spi1TxFifo[0];
      uint16_t newRxFifoEntry = 0;

      for(uint8_t bits = 0; bits < bitCount; bits++){
         newRxFifoEntry |= sdCardExchangeBit(currentTxFifoEntry & startBit);
         newRxFifoEntry <<= 1;
         currentTxFifoEntry <<= 1;
      }

      //add received data to RX FIFO
      if(spi1RxPosition < 8){
         //not full add entry
         spi1RxPosition = newRxFifoEntry;
         spi1RxPosition++;
      }

      //remove used TX FIFO entry
      for(uint8_t count = 0; count < 7; count++)
         spi1TxFifo[count] = spi1TxFifo[count + 1];
      spi1TxPosition--;
   }

   registerArrayWrite16(SPICONT1, value);
}

static inline void setSpiCont2(uint16_t value){
   //the ENABLE bit must be set before the transfer and in the transfer command
   //important bits are ENABLE, XCH, IRQ, IRQEN and BITCOUNT
   uint16_t oldSpiCont2 = registerArrayRead16(SPICONT2);

   //force or clear an interrupt
   if((value & 0x00C0) == 0x00C0)
      setIprIsrBit(INT_SPI2);
   else
      clearIprIsrBit(INT_SPI2);

   /*
   if(value & 0x0200 && !(oldSpiCont2 & 0x0200)){
      //there may be some random shifing when enableing/disableing the register
      for(uint8_t skip = 0; skip < 3; skip++)//started with 20, 1 and 2 causes power down, 3 is the minimum that works
         ads7846ExchangeBit(false);
      //this for some reason prevents the battery from shooting up to 100% after a few seconds?
   }
   */

   //do a transfer
   if(value & oldSpiCont2 & 0x0200 && value & 0x0100){
      //enabled and exchange set
      uint8_t bitCount = (value & 0x000F) + 1;
      uint16_t startBit = 1 << (bitCount - 1);
      uint16_t spi2Data = registerArrayRead16(SPIDATA2);
      bool ads7846ChipSelect = !(getPortGValue() & 0x04);//this is unproven, but having it high makes the ADS7846 not work on hardware
      //uint16_t oldSpi2Data = spi2Data;

      //the input data is shifted into the unused bits if the transfer is less than 16 bits
      for(uint8_t bits = 0; bits < bitCount; bits++){
         bool newBit = true;

         if(ads7846ChipSelect)
            newBit = ads7846ExchangeBit(spi2Data & startBit);

         //debugLog("Sent Bit:%d\n", (bool)(spi2Data & startBit));
         spi2Data <<= 1;
         spi2Data |= newBit;
      }
      registerArrayWrite16(SPIDATA2, spi2Data);

      //debugLog("SPI2 transfer, ENABLE:%s, XCH:%s, IRQ:%s, IRQEN:%s, BITCOUNT:%d\n", boolString(value & 0x0200), boolString(value & 0x0100), boolString(value & 0x0080), boolString(value & 0x0400), (value & 0x000F) + 1);
      //debugLog("SPI2 transfer, before:0x%04X, after:0x%04X, PC:0x%08X\n", oldSpi2Data, spi2Data, m68k_get_reg(NULL, M68K_REG_PPC));

      //unset XCH, transfers are instant since timing is not emulated
      value &= 0xFEFF;

      //acknowledge transfer finished, transfers are instant since timing is not emulated
      value |= 0x0080;

      //IRQEN set, send an interrupt after transfer
      if(value & 0x0040)
         setIprIsrBit(INT_SPI2);
   }

   //check for any interrupts from the transfer
   checkInterrupts();

   registerArrayWrite16(SPICONT2, value & 0xE3FF);
}

static inline void setTstat1(uint16_t value){
   uint16_t oldTstat1 = registerArrayRead16(TSTAT1);
   uint16_t newTstat1 = (value & timerStatusReadAcknowledge[0]) | (oldTstat1 & ~timerStatusReadAcknowledge[0]);

   //debugLog("TSTAT1 write, old value:0x%04X, new value:0x%04X, write value:0x%04X\n", oldTstat1, newTstat1, value);

   if(!(newTstat1 & 0x0001) && (oldTstat1 & 0x0001)){
      //debugLog("Timer 1 interrupt cleared.\n");
      clearIprIsrBit(INT_TMR1);
      checkInterrupts();
   }
   timerStatusReadAcknowledge[0] &= newTstat1;//clear acknowledged reads cleared bits
   registerArrayWrite16(TSTAT1, newTstat1);
}

static inline void setTstat2(uint16_t value){
   uint16_t oldTstat2 = registerArrayRead16(TSTAT2);
   uint16_t newTstat2 = (value & timerStatusReadAcknowledge[1]) | (oldTstat2 & ~timerStatusReadAcknowledge[1]);

   //debugLog("TSTAT2 write, old value:0x%04X, new value:0x%04X, write value:0x%04X\n", oldTstat2, newTstat2, value);

   if(!(newTstat2 & 0x0001) && (oldTstat2 & 0x0001)){
      //debugLog("Timer 2 interrupt cleared.\n");
      clearIprIsrBit(INT_TMR2);
      checkInterrupts();
   }
   timerStatusReadAcknowledge[1] &= newTstat2;//clear acknowledged reads for cleared bits
   registerArrayWrite16(TSTAT2, newTstat2);
}

static inline void setPwmc1(uint16_t value){
   if((value & 0x00D0) == 0x00D0){
      //enabled, interrupt enabled and interrupt set
      //this register allows forcing an interrupt by writing a 1 to its IRQ bit
      setIprIsrBit(INT_PWM1);
      checkInterrupts();
   }

   if(!(value & 0x0010)){
      //PWM1 set to disabled

      value &= 0x80FF;//clear PWM prescaler value

      //need to flush PWM1 FIFO, unemulated
   }

   registerArrayWrite16(PWMC1, value);
}

static inline void setIsr(uint32_t value, bool useTopWord, bool useBottomWord){
   //Palm OS uses this 32 bit register as 2 16 bit registers

   //prevent any internal hardware interrupts from being cleared
   value &= 0x000F0F00;//IRQ5 is always level triggered

   if(useTopWord){
      uint16_t interruptControlRegister = registerArrayRead16(ICR);

      if(!(interruptControlRegister & 0x0800)){
         //IRQ1 is not edge triggered
         value &= ~INT_IRQ1;
      }

      if(!(interruptControlRegister & 0x0400)){
         //IRQ2 is not edge triggered
         value &= ~INT_IRQ2;
      }

      if(!(interruptControlRegister & 0x0200)){
         //IRQ3 is not edge triggered
         value &= ~INT_IRQ3;
      }

      if(!(interruptControlRegister & 0x0100)){
         //IRQ6 is not edge triggered
         value &= ~INT_IRQ6;
      }

      registerArrayWrite16(IPR, registerArrayRead16(IPR) & ~(value >> 16));
      registerArrayWrite16(ISR, registerArrayRead16(ISR) & ~(value >> 16));
   }
   if(useBottomWord){
      uint8_t portDEdgeSelect = registerArrayRead8(PDIRQEG);

      registerArrayWrite16(IPR + 2, registerArrayRead16(IPR + 2) & ~(value & 0xFFFF & portDEdgeSelect << 8));
      registerArrayWrite16(ISR + 2, registerArrayRead16(ISR + 2) & ~(value & 0xFFFF & portDEdgeSelect << 8));
   }

   checkInterrupts();
}

//register getters
static inline uint8_t getPortDInputPinValues(){
   //uint8_t requestedRow = ~(registerArrayRead8(PKDATA) & registerArrayRead8(PKSEL) & registerArrayRead8(PKDIR));//keys are requested on port k(set low to enable) and read on port d
   uint8_t requestedRow = ~getPortKValue();
   uint8_t portDInputValues = 0x00;

   //portDInputValues |= 0x80;//battery dead bit, dont know the proper level to set this

   if(palmSdCard.inserted)
      portDInputValues |= 0x20;

   //kbd row 0
   if(requestedRow & 0x20)
      portDInputValues |= palmInput.buttonCalendar | palmInput.buttonAddress << 1 | palmInput.buttonTodo << 2 | palmInput.buttonNotes << 3;

   //kbd row 1
   if(requestedRow & 0x40)
      portDInputValues |= palmInput.buttonUp | palmInput.buttonDown << 1;

   //kbd row 2
   if(requestedRow & 0x80)
      portDInputValues |= palmInput.buttonPower | palmInput.buttonContrast << 1 | palmInput.buttonAddress << 3;

   //the port d pins state is high for false, invert them on return, Palm OS uses PDPOL to swap back to pressed == 1
   return ~portDInputValues;
}

static inline uint8_t getPortAValue(){
   //not attached, used as data lines
   return 0x00;
}

static inline uint8_t getPortBValue(){
   return ((registerArrayRead8(PBDATA) & registerArrayRead8(PBDIR)) | ~registerArrayRead8(PBDIR)) & registerArrayRead8(PBSEL);
}

static inline uint8_t getPortCValue(){
   //port c uses pull downs not pull ups
   return registerArrayRead8(PCDATA) & registerArrayRead8(PCDIR) & registerArrayRead8(PCSEL);
}

static inline uint8_t getPortDValue(){
   uint8_t portDValue = getPortDInputPinValues();
   uint8_t portDData = registerArrayRead8(PDDATA);
   uint8_t portDDir = registerArrayRead8(PDDIR);
   uint8_t portDPolarity = registerArrayRead8(PDPOL);

   portDValue ^= portDPolarity;//only input polarity is affected by PDPOL
   portDValue &= ~portDDir;//only use above pin values for inputs, port d allows using special function pins as inputs while active unlike other ports
   portDValue |= portDData & portDDir;//if a pin is an output and has its data bit set return that too

   return portDValue;
}

static inline uint8_t getPortEValue(){
   return ((registerArrayRead8(PEDATA) & registerArrayRead8(PEDIR)) | ~registerArrayRead8(PEDIR)) & registerArrayRead8(PESEL);
}

static inline uint8_t getPortFValue(){
   uint8_t portFValue = 0x00;
   uint8_t portFData = registerArrayRead8(PKDATA);
   uint8_t portFDir = registerArrayRead8(PKDIR);
   uint8_t portFSel = registerArrayRead8(PKSEL);
   bool penIrqPin = !(ads7846PenIrqEnabled && palmInput.touchscreenTouched);//penIrqPin pulled low on touch

   portFValue |= penIrqPin << 1;
   portFValue |= 0x85;//bit 7 & 2-0 have pull ups, bits 6-3 have pull downs, bit 2 is occupied by PENIRQ
   portFValue &= ~portFDir & (portFSel | 0x02);//IRQ5 is readable even when PFSEL bit 2 is false
   portFValue |= portFData & portFDir & portFSel;

   return portFValue;
}

static inline uint8_t getPortGValue(){
   //port g only has 6 pins not 8
   uint8_t portGValue = 0x00;
   uint8_t portGData = registerArrayRead8(PGDATA);
   uint8_t portGDir = registerArrayRead8(PGDIR);
   uint8_t portGSel = registerArrayRead8(PGSEL);

   portGValue |= (palmMisc.backlightLevel > 0) << 1;
   portGValue |= 0x3D;//floating pins are high
   portGValue &= ~portGDir & portGSel;
   portGValue |= portGData & portGDir & portGSel;

   return portGValue;
}

static inline uint8_t getPortJValue(){
   return ((registerArrayRead8(PJDATA) & registerArrayRead8(PJDIR)) | ~registerArrayRead8(PJDIR)) & registerArrayRead8(PJSEL);
}

static inline uint8_t getPortKValue(){
   uint8_t portKValue = 0x00;
   uint8_t portKData = registerArrayRead8(PKDATA);
   uint8_t portKDir = registerArrayRead8(PKDIR);
   uint8_t portKSel = registerArrayRead8(PKSEL);

   portKValue |= !(palmMisc.dataPort == PORT_USB_CRADLE || palmMisc.dataPort == PORT_SERIAL_CRADLE) << 2;//true if charging
   portKValue |= 0xFB;//floating pins are high
   portKValue &= ~portKDir & portKSel;
   portKValue |= portKData & portKDir & portKSel;

   return portKValue;
}

static inline uint8_t getPortMValue(){
   //bit 5 has a pull up not pull down, bits 4-0 have a pull down, bit 7-6 are not active at all
   return ((registerArrayRead8(PMDATA) & registerArrayRead8(PMDIR)) | (~registerArrayRead8(PMDIR) & 0x20)) & registerArrayRead8(PMSEL);
}

static inline uint16_t getSpiTest(){
   //SSTATUS is unemulated because the datasheet has no descrption of how it works
   return spi1RxPosition << 4 | spi1TxPosition;
}

static inline uint16_t getPwmc1(){
   uint16_t returnValue = registerArrayRead16(PWMC1);

   //clear INT_PWM1 if active
   if(returnValue & 0x0080){
      clearIprIsrBit(INT_PWM1);
      checkInterrupts();
      registerArrayWrite16(PWMC1, returnValue & 0xFF7F);
   }

   //the REPEAT field is write only
   returnValue &= 0xFFF3;

   return returnValue;
}

//updaters
static inline void updatePowerButtonLedStatus(){
   palmMisc.powerButtonLed = (bool)(getPortBValue() & 0x40) != palmMisc.batteryCharging;
}

static inline void updateVibratorStatus(){
   palmMisc.vibratorOn = getPortKValue() & 0x10;
}

static inline void updateBacklightAmplifierStatus(){
   palmMisc.backlightLevel = (palmMisc.backlightLevel > 0) ? (1 + backlightAmplifierState()) : 0;
}
