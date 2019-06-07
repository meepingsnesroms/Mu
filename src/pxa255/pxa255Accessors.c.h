static uint8_t pxa255_io_read_byte(uint32_t addr){
   //is only vaild for UART stuff
}

static uint32_t pxa255_io_read_word(uint32_t addr){
   uint32_t out;

   switch(addr >> 16){
      case PXA255_CLOCK_MANAGER_BASE >> 16:
         pxa255pwrClkPrvClockMgrMemAccessF(&pxa255PwrClk, addr, 4, false, &out);
         break;
      case PXA255_POWER_MANAGER_BASE >> 16:
         pxa255pwrClkPrvPowerMgrMemAccessF(&pxa255PwrClk, addr, 4, false, &out);
         break;
      case PXA255_TIMR_BASE >> 16:
         pxa255timrPrvMemAccessF(&pxa255Timer, addr, 4, false, &out);
         break;
      case PXA255_GPIO_BASE >> 16:
         pxa255gpioPrvMemAccessF(&pxa255Gpio, addr, 4, false, &out);
         break;

      case PXA255_DMA_BASE >> 16:
      case PXA255_IC_BASE >> 16:
      case PXA255_RTC_BASE >> 16:
      case PXA255_FFUART_BASE >> 16:
      case PXA255_BTUART_BASE >> 16:
      case PXA255_STUART_BASE >> 16:
         //need to implement these
         debugLog("Unimplemented 32 bit PXA255 register read:0x%08X\n", addr);
         out = 0x00000000;
         break;

      default:
         debugLog("Invalid 32 bit PXA255 register read:0x%08X\n", addr);
         out = 0x00000000;
         break;
   }

   return out;
}

static void pxa255_io_write_byte(uint32_t addr, uint8_t value){
   //is only vaild for UART stuff
}

static void pxa255_io_write_word(uint32_t addr, uint32_t value){
   switch(addr >> 16){
      case PXA255_CLOCK_MANAGER_BASE >> 16:
         pxa255pwrClkPrvClockMgrMemAccessF(&pxa255PwrClk, addr, 4, true, &value);
         break;
      case PXA255_POWER_MANAGER_BASE >> 16:
         pxa255pwrClkPrvPowerMgrMemAccessF(&pxa255PwrClk, addr, 4, true, &value);
         break;
      case PXA255_TIMR_BASE >> 16:
         pxa255timrPrvMemAccessF(&pxa255Timer, addr, 4, true, &value);
         break;
      case PXA255_GPIO_BASE >> 16:
         pxa255gpioPrvMemAccessF(&pxa255Gpio, addr, 4, true, &value);
         break;

      case PXA255_DMA_BASE >> 16:
      case PXA255_IC_BASE >> 16:
      case PXA255_RTC_BASE >> 16:
      case PXA255_FFUART_BASE >> 16:
      case PXA255_BTUART_BASE >> 16:
      case PXA255_STUART_BASE >> 16:
         //need to implement these
         debugLog("Unimplemented 32 bit PXA255 register write:0x%08X, value:0x%08X\n", addr, value);
         break;

      default:
         debugLog("Invalid 32 bit PXA255 register write:0x%08X, value:0x%08X\n", addr, value);
         break;
   }
}

static uint32_t pxa255_lcd_read_word(uint32_t addr){
   uint32_t out;
   pxa255lcdPrvMemAccessF(&pxa255Lcd, addr, 4, false, &out);
   debugLog("32 bit PXA255 LCD register read:0x%08X\n", addr);
   return out;
}

static void pxa255_lcd_write_word(uint32_t addr, uint32_t value){
   pxa255lcdPrvMemAccessF(&pxa255Lcd, addr, 4, true, &value);
   debugLog("32 bit PXA255 LCD register write:0x%08X, value:0x%08X\n", addr, value);
}

static uint32_t pxa255_memctrl_read_word(uint32_t addr){
   debugLog("32 bit PXA255 MEMCTRL register read:0x%08X\n", addr);
   return 0x00000000;
}

static void pxa255_memctrl_write_word(uint32_t addr, uint32_t value){
   debugLog("32 bit PXA255 MEMCTRL register write:0x%08X, value:0x%08X\n", addr, value);
}

static uint8_t pxa255_pcmcia0_read_byte(uint32_t addr){
   debugLog("PCMCIA0 8 bit read:0x%08X\n", addr);
   return 0x00;
}

static uint16_t pxa255_pcmcia0_read_half(uint32_t addr){
   debugLog("PCMCIA0 16 bit read:0x%08X\n", addr);
   return 0x0000;
}

static uint32_t pxa255_pcmcia0_read_word(uint32_t addr){
   debugLog("PCMCIA0 32 bit read:0x%08X\n", addr);
   return 0x00000000;
}

static void pxa255_pcmcia0_write_byte(uint32_t addr, uint8_t value){
   debugLog("PCMCIA0 8 bit write:0x%08X, value:0x%02X\n", addr, value);
}

static void pxa255_pcmcia0_write_half(uint32_t addr, uint16_t value){
   debugLog("PCMCIA0 16 bit write:0x%08X, value:0x%04X\n", addr, value);
}

static void pxa255_pcmcia0_write_word(uint32_t addr, uint32_t value){
   debugLog("PCMCIA0 32 bit write:0x%08X, value:0x%08X\n", addr, value);
}

static uint8_t pxa255_pcmcia1_read_byte(uint32_t addr){
   debugLog("PCMCIA1 8 bit read:0x%08X\n", addr);
   return 0x00;
}

static uint16_t pxa255_pcmcia1_read_half(uint32_t addr){
   debugLog("PCMCIA1 16 bit read:0x%08X\n", addr);
   return 0x0000;
}

static uint32_t pxa255_pcmcia1_read_word(uint32_t addr){
   debugLog("PCMCIA1 32 bit read:0x%08X\n", addr);
   return 0x00000000;
}

static void pxa255_pcmcia1_write_byte(uint32_t addr, uint8_t value){
   debugLog("PCMCIA1 8 bit write:0x%08X, value:0x%02X\n", addr, value);
}

static void pxa255_pcmcia1_write_half(uint32_t addr, uint16_t value){
   debugLog("PCMCIA1 16 bit write:0x%08X, value:0x%04X\n", addr, value);
}

static void pxa255_pcmcia1_write_word(uint32_t addr, uint32_t value){
   debugLog("PCMCIA1 32 bit write:0x%08X, value:0x%08X\n", addr, value);
}
