static uint8_t pxa260_io_read_byte(uint32_t addr){
   //is only vaild for UART stuff
}

static uint32_t pxa260_io_read_word(uint32_t addr){
   uint32_t out;

   switch(addr >> 16){
      case PXA260_CLOCK_MANAGER_BASE >> 16:
         pxa260pwrClkPrvClockMgrMemAccessF(&pxa260PwrClk, addr, 4, false, &out);
         break;
      case PXA260_POWER_MANAGER_BASE >> 16:
         pxa260pwrClkPrvPowerMgrMemAccessF(&pxa260PwrClk, addr, 4, false, &out);
         break;
      case PXA260_TIMR_BASE >> 16:
         pxa260timrPrvMemAccessF(&pxa260Timer, addr, 4, false, &out);
         break;
      case PXA260_GPIO_BASE >> 16:
         pxa260gpioPrvMemAccessF(&pxa260Gpio, addr, 4, false, &out);
         break;
      case PXA260_I2C_BASE >> 16:
         out = pxa260I2cReadWord(addr);
         break;
      case PXA260_SSP_BASE >> 16:
         out = pxa260SspReadWord(addr);
         break;
      case PXA260_IC_BASE >> 16:
         pxa260icPrvMemAccessF(&pxa260Ic, addr, 4, false, &out);
         break;

      case PXA260_DMA_BASE >> 16:
      case PXA260_RTC_BASE >> 16:
      case PXA260_FFUART_BASE >> 16:
      case PXA260_BTUART_BASE >> 16:
      case PXA260_STUART_BASE >> 16:
         //need to implement these
         debugLog("Unimplemented 32 bit PXA260 register read:0x%08X, PC:0x%08X\n", addr, pxa260GetPc());
         out = 0x00000000;
         break;

      default:
         debugLog("Invalid 32 bit PXA260 register read:0x%08X, PC:0x%08X\n", addr, pxa260GetPc());
         out = 0x00000000;
         break;
   }

   return out;
}

static void pxa260_io_write_byte(uint32_t addr, uint8_t value){
   //is only vaild for UART stuff
}

static void pxa260_io_write_word(uint32_t addr, uint32_t value){
   switch(addr >> 16){
      case PXA260_CLOCK_MANAGER_BASE >> 16:
         pxa260pwrClkPrvClockMgrMemAccessF(&pxa260PwrClk, addr, 4, true, &value);
         break;
      case PXA260_POWER_MANAGER_BASE >> 16:
         pxa260pwrClkPrvPowerMgrMemAccessF(&pxa260PwrClk, addr, 4, true, &value);
         break;
      case PXA260_TIMR_BASE >> 16:
         pxa260timrPrvMemAccessF(&pxa260Timer, addr, 4, true, &value);
         break;
      case PXA260_GPIO_BASE >> 16:
         pxa260gpioPrvMemAccessF(&pxa260Gpio, addr, 4, true, &value);
         break;
      case PXA260_I2C_BASE >> 16:
         pxa260I2cWriteWord(addr, value);
         break;
      case PXA260_SSP_BASE >> 16:
         pxa260SspWriteWord(addr, value);
         break;
      case PXA260_IC_BASE >> 16:
         pxa260icPrvMemAccessF(&pxa260Ic, addr, 4, true, &value);
         break;

      case PXA260_DMA_BASE >> 16:
      case PXA260_RTC_BASE >> 16:
      case PXA260_FFUART_BASE >> 16:
      case PXA260_BTUART_BASE >> 16:
      case PXA260_STUART_BASE >> 16:
         //need to implement these
         debugLog("Unimplemented 32 bit PXA260 register write:0x%08X, value:0x%08X, PC:0x%08X\n", addr, value, pxa260GetPc());
         break;

      default:
         debugLog("Invalid 32 bit PXA260 register write:0x%08X, value:0x%08X, PC:0x%08X\n", addr, value, pxa260GetPc());
         break;
   }
}

static uint32_t pxa260_lcd_read_word(uint32_t addr){
   uint32_t out;
   pxa260lcdPrvMemAccessF(&pxa260Lcd, addr, 4, false, &out);
   debugLog("32 bit PXA260 LCD register read:0x%08X\n", addr);
   return out;
}

static void pxa260_lcd_write_word(uint32_t addr, uint32_t value){
   pxa260lcdPrvMemAccessF(&pxa260Lcd, addr, 4, true, &value);
   debugLog("32 bit PXA260 LCD register write:0x%08X, value:0x%08X\n", addr, value);
}

static uint8_t pxa260_pcmcia0_read_byte(uint32_t addr){
   debugLog("PCMCIA0 8 bit read:0x%08X\n", addr);
   return 0x00;
}

static uint16_t pxa260_pcmcia0_read_half(uint32_t addr){
   debugLog("PCMCIA0 16 bit read:0x%08X\n", addr);
   return 0x0000;
}

static uint32_t pxa260_pcmcia0_read_word(uint32_t addr){
   debugLog("PCMCIA0 32 bit read:0x%08X\n", addr);
   return 0x00000000;
}

static void pxa260_pcmcia0_write_byte(uint32_t addr, uint8_t value){
   debugLog("PCMCIA0 8 bit write:0x%08X, value:0x%02X\n", addr, value);
}

static void pxa260_pcmcia0_write_half(uint32_t addr, uint16_t value){
   debugLog("PCMCIA0 16 bit write:0x%08X, value:0x%04X\n", addr, value);
}

static void pxa260_pcmcia0_write_word(uint32_t addr, uint32_t value){
   debugLog("PCMCIA0 32 bit write:0x%08X, value:0x%08X\n", addr, value);
}

static uint8_t pxa260_pcmcia1_read_byte(uint32_t addr){
   debugLog("PCMCIA1 8 bit read:0x%08X\n", addr);
   return 0x00;
}

static uint16_t pxa260_pcmcia1_read_half(uint32_t addr){
   debugLog("PCMCIA1 16 bit read:0x%08X\n", addr);
   return 0x0000;
}

static uint32_t pxa260_pcmcia1_read_word(uint32_t addr){
   debugLog("PCMCIA1 32 bit read:0x%08X\n", addr);
   return 0x00000000;
}

static void pxa260_pcmcia1_write_byte(uint32_t addr, uint8_t value){
   debugLog("PCMCIA1 8 bit write:0x%08X, value:0x%02X\n", addr, value);
}

static void pxa260_pcmcia1_write_half(uint32_t addr, uint16_t value){
   debugLog("PCMCIA1 16 bit write:0x%08X, value:0x%04X\n", addr, value);
}

static void pxa260_pcmcia1_write_word(uint32_t addr, uint32_t value){
   debugLog("PCMCIA1 32 bit write:0x%08X, value:0x%08X\n", addr, value);
}

static uint16_t pxa260_static_chip_select_2_read_half(uint32_t addr){
   return w86l488Read16(addr & 0x0E);
}

static void pxa260_static_chip_select_2_write_half(uint32_t addr, uint16_t value){
   w86l488Write16(addr & 0x0E, value);
}

