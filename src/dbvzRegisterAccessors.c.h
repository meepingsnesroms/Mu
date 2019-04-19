//declare I/O port functions in advance
static uint8_t getPortAValue(void);
static uint8_t getPortBValue(void);
static uint8_t getPortCValue(void);
static uint8_t getPortDValue(void);
static uint8_t getPortEValue(void);
static uint8_t getPortFValue(void);
static uint8_t getPortGValue(void);
static uint8_t getPortJValue(void);
static uint8_t getPortKValue(void);
static uint8_t getPortMValue(void);

//basic accessors
static uint8_t registerArrayRead8(uint32_t address){return BUFFER_READ_8(palmReg, address, 0xFFF);}
static uint16_t registerArrayRead16(uint32_t address){return BUFFER_READ_16(palmReg, address, 0xFFF);}
static uint32_t registerArrayRead32(uint32_t address){return BUFFER_READ_32(palmReg, address, 0xFFF);}
static void registerArrayWrite8(uint32_t address, uint8_t value){BUFFER_WRITE_8(palmReg, address, 0xFFF, value);}
static void registerArrayWrite16(uint32_t address, uint16_t value){BUFFER_WRITE_16(palmReg, address, 0xFFF, value);}
static void registerArrayWrite32(uint32_t address, uint32_t value){BUFFER_WRITE_32(palmReg, address, 0xFFF, value);}

//interrupt setters, used for setting an interrupt with masking by IMR and logging in IPR
static void setIprIsrBit(uint32_t interruptBit){
   uint32_t newIpr = registerArrayRead32(IPR) | interruptBit;
   registerArrayWrite32(IPR, newIpr);
   registerArrayWrite32(ISR, newIpr & ~registerArrayRead32(IMR));
}

static void clearIprIsrBit(uint32_t interruptBit){
   uint32_t newIpr = registerArrayRead32(IPR) & ~interruptBit;
   registerArrayWrite32(IPR, newIpr);
   registerArrayWrite32(ISR, newIpr & ~registerArrayRead32(IMR));
}

//SPI1 FIFO accessors
static uint8_t spi1RxFifoEntrys(void){
   //check for wraparound
   if(spi1RxWritePosition < spi1RxReadPosition)
      return spi1RxWritePosition + 9 - spi1RxReadPosition;
   return spi1RxWritePosition - spi1RxReadPosition;
}

static uint16_t spi1RxFifoRead(void){
   if(spi1RxFifoEntrys() > 0)
      spi1RxReadPosition = (spi1RxReadPosition + 1) % 9;
   spi1RxOverflowed = false;
   return spi1RxFifo[spi1RxReadPosition];
}

static void spi1RxFifoWrite(uint16_t value){
   if(spi1RxFifoEntrys() < 8){
      spi1RxWritePosition = (spi1RxWritePosition + 1) % 9;
   }
   else{
      spi1RxOverflowed = true;
      debugLog("SPI1 RX FIFO overflowed\n");
   }
   spi1RxFifo[spi1RxWritePosition] = value;
}

static void spi1RxFifoFlush(void){
   spi1RxReadPosition = spi1RxWritePosition;
}

static uint8_t spi1TxFifoEntrys(void){
   //check for wraparound
   if(spi1TxWritePosition < spi1TxReadPosition)
      return spi1TxWritePosition + 9 - spi1TxReadPosition;
   return spi1TxWritePosition - spi1TxReadPosition;
}

static uint16_t spi1TxFifoRead(void){
   //dont need a safety check here, the emulator will always check that data is present before trying to access it
   spi1TxReadPosition = (spi1TxReadPosition + 1) % 9;
   return spi1TxFifo[spi1TxReadPosition];
}

static void spi1TxFifoWrite(uint16_t value){
   if(spi1TxFifoEntrys() < 8){
      spi1TxWritePosition = (spi1TxWritePosition + 1) % 9;
      spi1TxFifo[spi1TxWritePosition] = value;
   }
}

static void spi1TxFifoFlush(void){
   spi1TxReadPosition = spi1TxWritePosition;
}

//PWM1 FIFO accessors
static uint8_t pwm1FifoEntrys(void){
   //check for wraparound
   if(pwm1WritePosition < pwm1ReadPosition)
      return pwm1WritePosition + 6 - pwm1ReadPosition;
   return pwm1WritePosition - pwm1ReadPosition;
}

int32_t pwm1FifoRunSample(int32_t now, int32_t clockOffset){
   uint16_t period = registerArrayRead8(PWMP1) + 2;
   uint16_t pwmc1 = registerArrayRead16(PWMC1);
   uint8_t prescaler = (pwmc1 >> 8 & 0x7F) + 1;
   uint8_t clockDivider = 2 << (pwmc1 & 0x03);
   uint8_t repeat = 1 << (pwmc1 >> 2 & 0x03);
   int32_t audioNow = now + clockOffset;
   int32_t audioSampleDuration = (pwmc1 & 0x8000)/*CLKSRC*/ ? audioGetFramePercentIncrementFromClk32s(period * prescaler * clockDivider) : audioGetFramePercentIncrementFromSysclks(period * prescaler * clockDivider);
   float dutyCycle;
   uint8_t index;

   //try to get next sample, if none are available play old sample
   if(pwm1FifoEntrys() > 0)
      pwm1ReadPosition = (pwm1ReadPosition + 1) % 6;
   dutyCycle = fMin((float)pwm1Fifo[pwm1ReadPosition] / period, 1.00);

   for(index = 0; index < repeat; index++){
#if !defined(EMU_NO_SAFETY)
      if(audioNow + audioSampleDuration >= AUDIO_CLOCK_RATE)
         break;
#endif

      blip_add_delta(palmAudioResampler, audioNow, dutyCycle * AUDIO_SPEAKER_RANGE);
      blip_add_delta(palmAudioResampler, audioNow + audioSampleDuration * dutyCycle, (dutyCycle - 1.00) * AUDIO_SPEAKER_RANGE);
      audioNow += audioSampleDuration;
   }

   //check for interrupt
   if(pwm1FifoEntrys() < 2){
      //trigger interrupt if enabled
      if(pwmc1 & 0x0040)
         setIprIsrBit(DBVZ_INT_PWM1);
      //checkInterrupts() is run when the clock that called this function is finished

      registerArrayWrite16(PWMC1, pwmc1 | 0x0080);//set IRQ bit
   }

   return audioSampleDuration * repeat;
}

static void pwm1FifoWrite(uint8_t value){
   if(pwm1FifoEntrys() < 5){
      pwm1WritePosition = (pwm1WritePosition + 1) % 6;
      pwm1Fifo[pwm1WritePosition] = value;
   }
}

static void pwm1FifoFlush(void){
   pwm1ReadPosition = pwm1WritePosition;
   pwm1Fifo[pwm1WritePosition] = 0x00;
}

//register setters
static void setCsa(uint16_t value){
   chips[DBVZ_CHIP_A0_ROM].enable = value & 0x0001;
   chips[DBVZ_CHIP_A0_ROM].readOnly = !!(value & 0x8000);
   chips[DBVZ_CHIP_A0_ROM].lineSize = 0x20000/*128kb*/ << (value >> 1 & 0x0007);

   //CSA is now just a normal chip select
   if(chips[DBVZ_CHIP_A0_ROM].enable && chips[DBVZ_CHIP_A0_ROM].inBootMode)
      chips[DBVZ_CHIP_A0_ROM].inBootMode = false;

   chips[DBVZ_CHIP_A1_USB].enable = chips[DBVZ_CHIP_A0_ROM].enable;
   chips[DBVZ_CHIP_A1_USB].readOnly = chips[DBVZ_CHIP_A0_ROM].readOnly;
   chips[DBVZ_CHIP_A1_USB].start = chips[DBVZ_CHIP_A0_ROM].start + chips[DBVZ_CHIP_A0_ROM].lineSize;
   chips[DBVZ_CHIP_A1_USB].lineSize = chips[DBVZ_CHIP_A0_ROM].lineSize;

   registerArrayWrite16(CSA, value & 0x81FF);
}

static void setCsb(uint16_t value){
   uint16_t csControl1 = registerArrayRead16(CSCTRL1);

   chips[DBVZ_CHIP_B0_SED].enable = value & 0x0001;
   chips[DBVZ_CHIP_B0_SED].readOnly = !!(value & 0x8000);
   chips[DBVZ_CHIP_B0_SED].lineSize = 0x20000/*128kb*/ << (value >> 1 & 0x0007);

   //attributes
   chips[DBVZ_CHIP_B0_SED].supervisorOnlyProtectedMemory = !!(value & 0x4000);
   chips[DBVZ_CHIP_B0_SED].readOnlyForProtectedMemory = !!(value & 0x2000);
   if(csControl1 & 0x4000 && csControl1 & 0x0001)
      chips[DBVZ_CHIP_B0_SED].unprotectedSize = chips[DBVZ_CHIP_B0_SED].lineSize / (1 << 7 - ((value >> 11 & 0x0003) | 0x0004));
   else
      chips[DBVZ_CHIP_B0_SED].unprotectedSize = chips[DBVZ_CHIP_B0_SED].lineSize / (1 << 7 - (value >> 11 & 0x0003));

   chips[DBVZ_CHIP_B1_NIL].enable = chips[DBVZ_CHIP_B0_SED].enable;
   chips[DBVZ_CHIP_B1_NIL].readOnly = chips[DBVZ_CHIP_B0_SED].readOnly;
   chips[DBVZ_CHIP_B1_NIL].start = chips[DBVZ_CHIP_B0_SED].start + chips[DBVZ_CHIP_B0_SED].lineSize;
   chips[DBVZ_CHIP_B1_NIL].lineSize = chips[DBVZ_CHIP_B0_SED].lineSize;
   chips[DBVZ_CHIP_B1_NIL].supervisorOnlyProtectedMemory = chips[DBVZ_CHIP_B0_SED].supervisorOnlyProtectedMemory;
   chips[DBVZ_CHIP_B1_NIL].readOnlyForProtectedMemory = chips[DBVZ_CHIP_B0_SED].readOnlyForProtectedMemory;
   chips[DBVZ_CHIP_B1_NIL].unprotectedSize = chips[DBVZ_CHIP_B0_SED].unprotectedSize;

   registerArrayWrite16(CSB, value & 0xF9FF);
}

static void setCsd(uint16_t value){
   uint16_t csControl1 = registerArrayRead16(CSCTRL1);

   chips[DBVZ_CHIP_DX_RAM].enable = value & 0x0001;
   chips[DBVZ_CHIP_DX_RAM].readOnly = !!(value & 0x8000);
   if(csControl1 & 0x0040 && value & 0x0200)
      chips[DBVZ_CHIP_DX_RAM].lineSize = 0x800000/*8mb*/ << (value >> 1 & 0x0001);
   else
      chips[DBVZ_CHIP_DX_RAM].lineSize = 0x8000/*32kb*/ << (value >> 1 & 0x0007);

   //attributes
   chips[DBVZ_CHIP_DX_RAM].supervisorOnlyProtectedMemory = !!(value & 0x4000);
   chips[DBVZ_CHIP_DX_RAM].readOnlyForProtectedMemory = !!(value & 0x2000);
   if(csControl1 & 0x4000 && csControl1 & 0x0010)
      chips[DBVZ_CHIP_DX_RAM].unprotectedSize = chips[DBVZ_CHIP_DX_RAM].lineSize / (1 << 7 - ((value >> 11 & 0x0003) | 0x0004));
   else
      chips[DBVZ_CHIP_DX_RAM].unprotectedSize = chips[DBVZ_CHIP_DX_RAM].lineSize / (1 << 7 - (value >> 11 & 0x0003));

   //debugLog("RAM unprotected size:0x%08X, bits:0x%02X\n", chips[DBVZ_CHIP_DX_RAM].unprotectedSize, ((value >> 11 & 0x0003) | (csControl1 & 0x4000 && csControl1 & 0x0010) * 0x0004));

   registerArrayWrite16(CSD, value);
}

static void setCsgba(uint16_t value){
   uint16_t csugba = registerArrayRead16(CSUGBA);

   //add extra address bits if enabled
   if(csugba & 0x8000)
      chips[DBVZ_CHIP_A0_ROM].start = (csugba >> 12 & 0x0007) << 29 | value >> 1 << 14;
   else
      chips[DBVZ_CHIP_A0_ROM].start = value >> 1 << 14;

   chips[DBVZ_CHIP_A1_USB].start = chips[DBVZ_CHIP_A0_ROM].start + chips[DBVZ_CHIP_A0_ROM].lineSize;

   registerArrayWrite16(CSGBA, value & 0xFFFE);
}

static void setCsgbb(uint16_t value){
   uint16_t csugba = registerArrayRead16(CSUGBA);

   //add extra address bits if enabled
   if(csugba & 0x8000)
      chips[DBVZ_CHIP_B0_SED].start = (csugba >> 8 & 0x0007) << 29 | value >> 1 << 14;
   else
      chips[DBVZ_CHIP_B0_SED].start = value >> 1 << 14;

   chips[DBVZ_CHIP_B1_NIL].start = chips[DBVZ_CHIP_B0_SED].start + chips[DBVZ_CHIP_B0_SED].lineSize;

   registerArrayWrite16(CSGBB, value & 0xFFFE);
}

static void setCsgbd(uint16_t value){
   uint16_t csugba = registerArrayRead16(CSUGBA);

   //add extra address bits if enabled
   if(csugba & 0x8000)
      chips[DBVZ_CHIP_DX_RAM].start = (csugba & 0x0007) << 29 | value >> 1 << 14;
   else
      chips[DBVZ_CHIP_DX_RAM].start = value >> 1 << 14;

   registerArrayWrite16(CSGBD, value & 0xFFFE);
}

static void updateCsdAddressLines(void){
   uint16_t dramc = registerArrayRead16(DRAMC);
   uint16_t sdctrl = registerArrayRead16(SDCTRL);

   if(registerArrayRead16(CSD) & 0x0200 && sdctrl & 0x8000 && dramc & 0x8000 && !(dramc & 0x0400)){
      //this register can remap address lines, that behavior is way too CPU intensive and complicated so only the "memory testing" and "correct" behavior is being emulated
      chips[DBVZ_CHIP_DX_RAM].mask = 0x003FFFFF;

      //address line 23 is enabled
      if((sdctrl & 0x000C) == 0x0008)
         chips[DBVZ_CHIP_DX_RAM].mask |= 0x00800000;

      //address line 22 is enabled
      if((sdctrl & 0x0030) == 0x0010)
         chips[DBVZ_CHIP_DX_RAM].mask |= 0x00400000;
   }
   else{
      //RAM is not enabled properly
      chips[DBVZ_CHIP_DX_RAM].mask = 0x00000000;
   }
}

static void setPllfsr(uint16_t value){
   uint16_t oldPllfsr = registerArrayRead16(PLLFSR);

   //change frequency if frequency protect bit isnt set
   if(!(oldPllfsr & 0x4000)){
      registerArrayWrite16(PLLFSR, (value & 0x4CFF) | (oldPllfsr & 0x8000));//preserve CLK32 bit
      palmSysclksPerClk32 = sysclksPerClk32();
   }
}

static void setScr(uint8_t value){
   uint8_t oldScr = registerArrayRead8(SCR);
   uint8_t newScr = value & 0x1F;

   //preserve privilege violation, write protect violation and bus error timeout
   newScr |= oldScr & 0xE0;

   //clear violations on writing 1 to them
   newScr &= ~(value & 0xE0);

   chips[DBVZ_CHIP_REGISTERS].supervisorOnlyProtectedMemory = value & 0x08;

   registerArrayWrite8(SCR, newScr);//must be written before calling setRegisterFFFFAccessMode
   if((newScr & 0x04) != (oldScr & 0x04)){
      if(newScr & 0x04)
         dbvzSetRegisterXXFFAccessMode();
      else
         dbvzSetRegisterFFFFAccessMode();
   }
}

static void setIlcr(uint16_t value){
   uint16_t oldIlcr = registerArrayRead16(ILCR);
   uint16_t newIlcr = 0x0000;

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

static void setSpiIntCs(uint16_t value){
   uint16_t oldSpiIntCs = registerArrayRead16(SPIINTCS);
   uint16_t newSpiIntCs = value & 0xFF00;
   uint8_t rxEntrys = spi1RxFifoEntrys();
   uint8_t txEntrys = spi1TxFifoEntrys();

   //newSpiIntCs |= spi1TxOverflowed << 7;//BO, slave mode not supported
   newSpiIntCs |= spi1RxOverflowed << 6;//RO
   newSpiIntCs |= (rxEntrys == 8) << 5;//RF
   newSpiIntCs |= (rxEntrys >= 4) << 4;//RH
   newSpiIntCs |= (rxEntrys > 0) << 3;//RR
   newSpiIntCs |= (txEntrys == 8) << 2;//TF
   newSpiIntCs |= (txEntrys >= 4) << 1;//TH, the datasheet contradicts itself on whether its more than or equal to 4 empty or full slots
   newSpiIntCs |= txEntrys == 0;//TE

   //if interrupt state changed update interrupts too, top 8 bits are just the enable bits for the bottom 8
   if(!!(newSpiIntCs >> 8 & newSpiIntCs) != !!(oldSpiIntCs >> 8 & oldSpiIntCs)){
      if(newSpiIntCs >> 8 & newSpiIntCs)
         setIprIsrBit(DBVZ_INT_SPI1);
      else
         clearIprIsrBit(DBVZ_INT_SPI1);
      checkInterrupts();
   }

   registerArrayWrite16(SPIINTCS, newSpiIntCs);
}

static void setSpiCont1(uint16_t value){
   //only master mode is implemented(even then only partially)!!!
   uint16_t oldSpiCont1 = registerArrayRead16(SPICONT1);

   //debugLog("SPICONT1 write, old value:0x%04X, value:0x%04X\n", oldSpiCont1, value);

   //SPI1 disabled
   if(oldSpiCont1 & 0x0200 && !(value & 0x0200)){
      spi1RxFifoFlush();
      spi1TxFifoFlush();
   }

   //slave mode and enabled, dont know what to do
   //if(!(value & 0x0400) && value & 0x0200)
   //   debugLog("SPI1 set to slave mode, PC:0x%08X\n", flx68000GetPc());

   //do a transfer if enabled(this register write and last) and exchange set
   if(value & oldSpiCont1 & 0x0200 && value & 0x0100){
      while(spi1TxFifoEntrys() > 0){
         uint16_t currentTxFifoEntry = spi1TxFifoRead();
         uint16_t newRxFifoEntry = 0x0000;
         uint8_t bitCount = (value & 0x000F) + 1;
         uint16_t startBit = 1 << (bitCount - 1);
         uint8_t bits;

         //debugLog("SPI1 transfer, bitCount:%d, PC:0x%08X\n", bitCount, flx68000GetPc());

         //The most significant bit is output when the CPU loads the transmitted data, 13.2.3 SPI 1 Phase and Polarity Configurations MC68VZ328UM.pdf
         for(bits = 0; bits < bitCount; bits++){
            newRxFifoEntry <<= 1;
            newRxFifoEntry |= sdCardExchangeBit(!!(currentTxFifoEntry & startBit));
            currentTxFifoEntry <<= 1;
         }

         //add received data back to RX FIFO
         spi1RxFifoWrite(newRxFifoEntry);

         //overflow occured, remove 1 FIFO entry
         //I do not currently know if the FIFO entry is removed from the back or front of the FIFO, going with the back for now
         //if(spi1RxFifoEntrys() == 0)
         //   spi1RxFifoRead();
      }
   }

   //update SPIINTCS interrupt bits
   setSpiIntCs(registerArrayRead16(SPIINTCS));

   //unset XCH, transfers are instant since timing is not emulated
   value &= 0xFEFF;

   //debugLog("Transfer complete, SPIINTCS:0x%04X\n", registerArrayRead16(SPIINTCS));

   registerArrayWrite16(SPICONT1, value);
}

static void setSpiCont2(uint16_t value){
   //the ENABLE bit must be set before the transfer and in the transfer command
   //important bits are ENABLE, XCH, IRQ, IRQEN and BITCOUNT
   uint16_t oldSpiCont2 = registerArrayRead16(SPICONT2);

   //force or clear an interrupt
   if((value & 0x00C0) == 0x00C0)
      setIprIsrBit(DBVZ_INT_SPI2);
   else
      clearIprIsrBit(DBVZ_INT_SPI2);

   //do a transfer if enabled(this register write and last) and exchange set
   if(value & oldSpiCont2 & 0x0200 && value & 0x0100){
      uint8_t bitCount = (value & 0x000F) + 1;
      uint16_t startBit = 1 << (bitCount - 1);
      uint16_t spi2Data = registerArrayRead16(SPIDATA2);
      bool spiClk2Enabled = !(registerArrayRead8(PESEL) & 0x04);
      //uint16_t oldSpi2Data = spi2Data;

      //the input data is shifted into the unused bits if the transfer is less than 16 bits
      if(spiClk2Enabled){
         uint8_t bits;

         //shift in valid data
         for(bits = 0; bits < bitCount; bits++){
            bool newBit = ads7846ExchangeBit(!!(spi2Data & startBit));
            spi2Data <<= 1;
            spi2Data |= newBit;
         }
      }
      else{
         //shift in 0s, this is inaccurate, it should be whatever the last bit on SPIRXD(the SPI2 pin, not the SPI1 register) was
         spi2Data <<= bitCount;
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
         setIprIsrBit(DBVZ_INT_SPI2);
   }

   //check for any interrupts from the transfer
   checkInterrupts();

   registerArrayWrite16(SPICONT2, value & 0xE3FF);
}

static void setTstat1(uint16_t value){
   uint16_t oldTstat1 = registerArrayRead16(TSTAT1);
   uint16_t newTstat1 = (value & timerStatusReadAcknowledge[0]) | (oldTstat1 & ~timerStatusReadAcknowledge[0]);

   //debugLog("TSTAT1 write, old value:0x%04X, new value:0x%04X, write value:0x%04X\n", oldTstat1, newTstat1, value);

   if(!(newTstat1 & 0x0001) && (oldTstat1 & 0x0001)){
      //debugLog("Timer 1 interrupt cleared.\n");
      clearIprIsrBit(DBVZ_INT_TMR1);
      checkInterrupts();
   }
   timerStatusReadAcknowledge[0] &= newTstat1;//clear acknowledged reads cleared bits
   registerArrayWrite16(TSTAT1, newTstat1);
}

static void setTstat2(uint16_t value){
   uint16_t oldTstat2 = registerArrayRead16(TSTAT2);
   uint16_t newTstat2 = (value & timerStatusReadAcknowledge[1]) | (oldTstat2 & ~timerStatusReadAcknowledge[1]);

   //debugLog("TSTAT2 write, old value:0x%04X, new value:0x%04X, write value:0x%04X\n", oldTstat2, newTstat2, value);

   if(!(newTstat2 & 0x0001) && (oldTstat2 & 0x0001)){
      //debugLog("Timer 2 interrupt cleared.\n");
      clearIprIsrBit(DBVZ_INT_TMR2);
      checkInterrupts();
   }
   timerStatusReadAcknowledge[1] &= newTstat2;//clear acknowledged reads for cleared bits
   registerArrayWrite16(TSTAT2, newTstat2);
}

static void setPwmc1(uint16_t value){
   uint16_t oldPwmc1 = registerArrayRead16(PWMC1);

   //dont allow manually setting FIFOAV
   value &= 0xFFDF;

   //PWM1 enabled, set IRQ
   if(!(oldPwmc1 & 0x0010) && value & 0x0010){
      value |= 0x0080;//enable IRQ
      pwm1ClocksToNextSample = 0;//when first sample is written output it immediately
   }

   //clear interrupt by write(reading can also clear the interrupt)
   if(oldPwmc1 & 0x0080 && !(value & 0x0080)){
      clearIprIsrBit(DBVZ_INT_PWM1);
      checkInterrupts();
   }

   //interrupt enabled and interrupt set
   if((value & 0x00C0) == 0x00C0){
      //this register also allows forcing an interrupt by writing a 1 to its IRQ bit when IRQEN is enabled
      setIprIsrBit(DBVZ_INT_PWM1);
      checkInterrupts();
   }

   //PWM1 disabled
   if(oldPwmc1 & 0x0010 && !(value & 0x0010)){
      pwm1FifoFlush();
      value &= 0x80FF;//clear PWM prescaler value
   }

   registerArrayWrite16(PWMC1, value);
}

static void setIsr(uint32_t value, bool useTopWord, bool useBottomWord){
   //Palm OS uses this 32 bit register as 2 16 bit registers

   //prevent any internal hardware interrupts from being cleared
   value &= 0x000F0F00;//IRQ5 is always level triggered

   if(useTopWord){
      uint16_t interruptControlRegister = registerArrayRead16(ICR);

      //IRQ1 is not edge triggered
      if(!(interruptControlRegister & 0x0800))
         value &= ~DBVZ_INT_IRQ1;

      //IRQ2 is not edge triggered
      if(!(interruptControlRegister & 0x0400))
         value &= ~DBVZ_INT_IRQ2;

      //IRQ3 is not edge triggered
      if(!(interruptControlRegister & 0x0200))
         value &= ~DBVZ_INT_IRQ3;

      //IRQ6 is not edge triggered
      if(!(interruptControlRegister & 0x0100))
         value &= ~DBVZ_INT_IRQ6;

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
static uint8_t getPortDInputPinValues(void){
   uint8_t requestedRow = ~getPortKValue();
   uint8_t portDInputValues = 0x00;

   //portDInputValues |= 0x80;//battery dead bit, dont know the proper level to set this

   if(palmSdCard.flashChip.data)
      portDInputValues |= 0x20;

   //kbd row 0
   if(requestedRow & 0x20)
      portDInputValues |= palmInput.buttonCalendar | palmInput.buttonAddress << 1 | palmInput.buttonTodo << 2 | palmInput.buttonNotes << 3;

   //kbd row 1
   if(requestedRow & 0x40)
      portDInputValues |= palmInput.buttonUp | palmInput.buttonDown << 1;

   //kbd row 2
   if(requestedRow & 0x80)
      portDInputValues |= palmInput.buttonPower;

   //the port d pins state is high for false, invert them on return, Palm OS uses PDPOL to swap back to pressed == 1
   return ~portDInputValues;
}

static uint8_t getPortAValue(void){
   //not attached, used as CPU data lines
   return 0x00;
}

static uint8_t getPortBValue(void){
   return ((registerArrayRead8(PBDATA) & registerArrayRead8(PBDIR)) | ~registerArrayRead8(PBDIR)) & registerArrayRead8(PBSEL);
}

static uint8_t getPortCValue(void){
   //port c uses pull downs not pull ups
   return registerArrayRead8(PCDATA) & registerArrayRead8(PCDIR) & registerArrayRead8(PCSEL);
}

static uint8_t getPortDValue(void){
   uint8_t portDValue = getPortDInputPinValues();
   uint8_t portDData = registerArrayRead8(PDDATA);
   uint8_t portDDir = registerArrayRead8(PDDIR);
   uint8_t portDPolarity = registerArrayRead8(PDPOL);

   portDValue ^= portDPolarity;//only input polarity is affected by PDPOL
   portDValue &= ~portDDir;//only use above pin values for inputs, port d allows using special function pins as inputs while active unlike other ports
   portDValue |= portDData & portDDir;//if a pin is an output and has its data bit set return that too

   return portDValue;
}

static uint8_t getPortEValue(void){
   return ((registerArrayRead8(PEDATA) & registerArrayRead8(PEDIR)) | ~registerArrayRead8(PEDIR)) & registerArrayRead8(PESEL);
}

static uint8_t getPortFValue(void){
   uint8_t portFValue = 0x00;
   uint8_t portFData = registerArrayRead8(PFDATA);
   uint8_t portFDir = registerArrayRead8(PFDIR);
   uint8_t portFSel = registerArrayRead8(PFSEL);
   bool penIrqPin = ads7846PenIrqEnabled ? !palmInput.touchscreenTouched : true;//penIrqPin pulled low on touch

   portFValue |= penIrqPin << 1;
   portFValue |= 0x85;//bit 7 & 2<->0 have pull ups, bits 6<->3 have pull downs, bit 2 is occupied by PENIRQ
   portFValue &= ~portFDir & (portFSel | 0x02);//IRQ5 is readable even when PFSEL bit 2 is false
   portFValue |= portFData & portFDir & portFSel;

   return portFValue;
}

static uint8_t getPortGValue(void){
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

static uint8_t getPortJValue(void){
   return ((registerArrayRead8(PJDATA) & registerArrayRead8(PJDIR)) | ~registerArrayRead8(PJDIR)) & registerArrayRead8(PJSEL);
}

static uint8_t getPortKValue(void){
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

static uint8_t getPortMValue(void){
   //bit 5 has a pull up not pull down, bits 4<->0 have a pull down, bit 7<->6 are not active at all
   return ((registerArrayRead8(PMDATA) & registerArrayRead8(PMDIR)) | (~registerArrayRead8(PMDIR) & 0x20)) & registerArrayRead8(PMSEL);
}

static void samplePwm1(bool forClk32, double sysclks){
   uint16_t pwmc1 = registerArrayRead16(PWMC1);

   //validate clock mode
   if(forClk32 != !!(pwmc1 & 0x8000))
      return;

   if(pwmc1 & 0x0010){
      int32_t audioNow = audioGetFramePercentage();

      //add cycles
      pwm1ClocksToNextSample -= forClk32 ? audioGetFramePercentIncrementFromClk32s(1) : audioGetFramePercentIncrementFromSysclks(sysclks);

      //use samples
      while(pwm1ClocksToNextSample <= 0){
         //play samples until waiting(pwm1ClocksToNextSample is positive), if no new samples are available the newest old sample will be played
         int32_t audioUsed = pwm1FifoRunSample(audioNow, pwm1ClocksToNextSample);
         pwm1ClocksToNextSample += audioUsed;
         audioNow += audioUsed;
      }
   }
}

static uint16_t getPwmc1(void){
   uint16_t returnValue = registerArrayRead16(PWMC1);

   //have FIFOAV set if a slot is available
   if(pwm1FifoEntrys() < 5)
      returnValue |= 0x0020;

   //clear INT_PWM1 if active
   if(returnValue & 0x0080){
      clearIprIsrBit(DBVZ_INT_PWM1);
      checkInterrupts();
      registerArrayWrite16(PWMC1, returnValue & 0xFF5F);
   }

   //the REPEAT field is write only
   returnValue &= 0xFFF3;

   return returnValue;
}

//updaters
static void updatePowerButtonLedStatus(void){
   palmMisc.powerButtonLed = !!(getPortBValue() & 0x40) != palmMisc.batteryCharging;
}

static void updateVibratorStatus(void){
   palmMisc.vibratorOn = !!(getPortKValue() & 0x10);
}

static void updateAds7846ChipSelectStatus(void){
   ads7846SetChipSelect(!!(getPortGValue() & 0x04));
}

static void updateSdCardChipSelectStatus(void){
   sdCardSetChipSelect(!!(getPortJValue() & 0x08));
}

static void updateBacklightAmplifierStatus(void){
   palmMisc.backlightLevel = (palmMisc.backlightLevel > 0) ? (1 + m515BacklightAmplifierState()) : 0;
}

static void updateTouchState(void){
   if(!(registerArrayRead8(PFSEL) & registerArrayRead8(PFDIR) & 0x02)){
      if((ads7846PenIrqEnabled ? !palmInput.touchscreenTouched : true) == !!(registerArrayRead16(ICR) & 0x0080))
         setIprIsrBit(DBVZ_INT_IRQ5);
      else
         clearIprIsrBit(DBVZ_INT_IRQ5);
   }
}
