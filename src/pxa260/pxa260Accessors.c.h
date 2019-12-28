static uint8_t pxa260_io_read_byte(uint32_t addr){
   debugLog("Invalid 8 bit PXA260 register read:0x%08X, PC:0x%08X\n", addr, pxa260GetPc());
   return 0x00;
}

static uint16_t pxa260_io_read_half(uint32_t addr){
   debugLog("Invalid 16 bit PXA260 register read:0x%08X, PC:0x%08X\n", addr, pxa260GetPc());
   return 0x0000;
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
      case PXA260_UDC_BASE >> 16:
         out = pxa260UdcReadWord(addr);
         break;
      case PXA260_IC_BASE >> 16:
         pxa260icPrvMemAccessF(&pxa260Ic, addr, 4, false, &out);
         break;
      case PXA260_RTC_BASE >> 16:
         pxa260rtcPrvMemAccessF(&pxa260Rtc, addr, 4, false, &out);
         break;

      case PXA260_DMA_BASE >> 16:
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
   debugLog("Invalid 8 bit PXA260 register write:0x%08X, value:0x%02X, PC:0x%08X\n", addr, value, pxa260GetPc());
}

static void pxa260_io_write_half(uint32_t addr, uint16_t value){
   debugLog("Invalid 16 bit PXA260 register write:0x%08X, value:0x%04X, PC:0x%08X\n", addr, value, pxa260GetPc());
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
      case PXA260_UDC_BASE >> 16:
         pxa260UdcWriteWord(addr, value);
         break;
      case PXA260_IC_BASE >> 16:
         pxa260icPrvMemAccessF(&pxa260Ic, addr, 4, true, &value);
         break;
      case PXA260_RTC_BASE >> 16:
         pxa260rtcPrvMemAccessF(&pxa260Rtc, addr, 4, true, &value);
         break;

      case PXA260_DMA_BASE >> 16:
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
   debugLog("PCMCIA0 8 bit read:0x%08X, PC:0x%08X\n", addr, pxa260GetPc());
   return 0x00;
}

static uint16_t pxa260_pcmcia0_read_half(uint32_t addr){
   debugLog("PCMCIA0 16 bit read:0x%08X, PC:0x%08X\n", addr, pxa260GetPc());
   return 0x0000;
}

static uint32_t pxa260_pcmcia0_read_word(uint32_t addr){
   debugLog("PCMCIA0 32 bit read:0x%08X, PC:0x%08X\n", addr, pxa260GetPc());
   return 0x00000000;
}

static void pxa260_pcmcia0_write_byte(uint32_t addr, uint8_t value){
   debugLog("PCMCIA0 8 bit write:0x%08X, value:0x%02X, PC:0x%08X\n", addr, value, pxa260GetPc());
}

static void pxa260_pcmcia0_write_half(uint32_t addr, uint16_t value){
   debugLog("PCMCIA0 16 bit write:0x%08X, value:0x%04X, PC:0x%08X\n", addr, value, pxa260GetPc());
}

static void pxa260_pcmcia0_write_word(uint32_t addr, uint32_t value){
   debugLog("PCMCIA0 32 bit write:0x%08X, value:0x%08X, PC:0x%08X\n", addr, value, pxa260GetPc());
}

static uint8_t pxa260_pcmcia1_read_byte(uint32_t addr){
   debugLog("PCMCIA1 8 bit read:0x%08X, PC:0x%08X\n", addr, pxa260GetPc());
   return 0x00;
}

static uint16_t pxa260_pcmcia1_read_half(uint32_t addr){
   debugLog("PCMCIA1 16 bit read:0x%08X, PC:0x%08X\n", addr, pxa260GetPc());
   return 0x0000;
}

static uint32_t pxa260_pcmcia1_read_word(uint32_t addr){
   debugLog("PCMCIA1 32 bit read:0x%08X, PC:0x%08X\n", addr, pxa260GetPc());
   return 0x00000000;
}

static void pxa260_pcmcia1_write_byte(uint32_t addr, uint8_t value){
   debugLog("PCMCIA1 8 bit write:0x%08X, value:0x%02X, PC:0x%08X\n", addr, value, pxa260GetPc());
}

static void pxa260_pcmcia1_write_half(uint32_t addr, uint16_t value){
   debugLog("PCMCIA1 16 bit write:0x%08X, value:0x%04X, PC:0x%08X\n", addr, value, pxa260GetPc());
}

static void pxa260_pcmcia1_write_word(uint32_t addr, uint32_t value){
   debugLog("PCMCIA1 32 bit write:0x%08X, value:0x%08X, PC:0x%08X\n", addr, value, pxa260GetPc());
}

static uint16_t pxa260_static_chip_select_2_read_half(uint32_t addr){
   return w86l488Read16(addr & 0x0E);
}

static void pxa260_static_chip_select_2_write_half(uint32_t addr, uint16_t value){
   w86l488Write16(addr & 0x0E, value);
}

uint8_t read_byte(uint32_t address){
   if(address >= PXA260_ROM_START_ADDRESS && address < PXA260_ROM_START_ADDRESS + TUNGSTEN_T3_ROM_SIZE)
      return palmRom[PXA260_ADDR_FIX_ENDIAN_8(address) - PXA260_ROM_START_ADDRESS];
   else if(address >= PXA260_RAM_START_ADDRESS && address < PXA260_RAM_START_ADDRESS + TUNGSTEN_T3_RAM_SIZE)
      return palmRam[PXA260_ADDR_FIX_ENDIAN_8(address) - PXA260_RAM_START_ADDRESS];
   else if(address >= PXA260_IO_BASE && address < PXA260_IO_BASE + PXA260_BANK_SIZE)
      return pxa260_io_read_byte(address);

   debugLog("Invalid byte read at address: 0x%08X\n", address);
   return 0x00;
}

uint16_t read_half(uint32_t address){
   if(address >= PXA260_ROM_START_ADDRESS && address < PXA260_ROM_START_ADDRESS + TUNGSTEN_T3_ROM_SIZE)
      return *(uint16_t*)(&palmRom[PXA260_ADDR_FIX_ENDIAN_16(address) - PXA260_ROM_START_ADDRESS]);
   else if(address >= PXA260_RAM_START_ADDRESS && address < PXA260_RAM_START_ADDRESS + TUNGSTEN_T3_RAM_SIZE)
      return *(uint16_t*)(&palmRam[PXA260_ADDR_FIX_ENDIAN_16(address) - PXA260_RAM_START_ADDRESS]);
   else if(address >= TUNGSTEN_T3_W86L488_START_ADDRESS && address < TUNGSTEN_T3_W86L488_START_ADDRESS + TUNGSTEN_T3_W86L488_SIZE)
      return w86l488Read16(address);
   else if(address >= PXA260_IO_BASE && address < PXA260_IO_BASE + PXA260_BANK_SIZE)
      return pxa260_io_read_half(address);

   debugLog("Invalid half read at address: 0x%08X\n", address);
   return 0x0000;
}

uint32_t read_word(uint32_t address){
   if(address >= PXA260_ROM_START_ADDRESS && address < PXA260_ROM_START_ADDRESS + TUNGSTEN_T3_ROM_SIZE)
      return *(uint32_t*)(&palmRom[PXA260_ADDR_FIX_ENDIAN_32(address) - PXA260_ROM_START_ADDRESS]);
   else if(address >= PXA260_RAM_START_ADDRESS && address < PXA260_RAM_START_ADDRESS + TUNGSTEN_T3_RAM_SIZE)
      return *(uint32_t*)(&palmRam[PXA260_ADDR_FIX_ENDIAN_32(address) - PXA260_RAM_START_ADDRESS]);
   else if(address >= PXA260_MEMCTRL_BASE && address < PXA260_MEMCTRL_BASE + PXA260_BANK_SIZE)
      return pxa260MemctrlReadWord(address);
   else if(address >= PXA260_LCD_BASE && address < PXA260_LCD_BASE + PXA260_BANK_SIZE)
      return pxa260_lcd_read_word(address);
   else if(address >= PXA260_IO_BASE && address < PXA260_IO_BASE + PXA260_BANK_SIZE)
      return pxa260_io_read_word(address);

   debugLog("Invalid word read at address: 0x%08X\n", address);
   return 0x00000000;
}

void write_byte(uint32_t address, uint8_t byte){
   if(address >= PXA260_RAM_START_ADDRESS && address < PXA260_RAM_START_ADDRESS + TUNGSTEN_T3_RAM_SIZE)
      palmRam[PXA260_ADDR_FIX_ENDIAN_8(address) - PXA260_RAM_START_ADDRESS] = byte;
   else if(address >= PXA260_IO_BASE && address < PXA260_IO_BASE + PXA260_BANK_SIZE)
      pxa260_io_write_byte(address, byte);
   else
      debugLog("Invalid byte write at address: 0x%08X, value:0x%02X\n", address, byte);
}

void write_half(uint32_t address, uint16_t half){
   if(address >= PXA260_RAM_START_ADDRESS && address < PXA260_RAM_START_ADDRESS + TUNGSTEN_T3_RAM_SIZE)
      *(uint16_t*)(&palmRam[PXA260_ADDR_FIX_ENDIAN_16(address) - PXA260_RAM_START_ADDRESS]) = half;
   else if(address >= TUNGSTEN_T3_W86L488_START_ADDRESS && address < TUNGSTEN_T3_W86L488_START_ADDRESS + TUNGSTEN_T3_W86L488_SIZE)
      w86l488Write16(address, half);
   else if(address >= PXA260_IO_BASE && address < PXA260_IO_BASE + PXA260_BANK_SIZE)
      pxa260_io_write_half(address, half);
   else
      debugLog("Invalid half write at address: 0x%08X, value:0x%04X\n", address, half);
}

void write_word(uint32_t address, uint32_t word){
   if(address >= PXA260_RAM_START_ADDRESS && address < PXA260_RAM_START_ADDRESS + TUNGSTEN_T3_RAM_SIZE)
      *(uint32_t*)(&palmRam[PXA260_ADDR_FIX_ENDIAN_32(address) - PXA260_RAM_START_ADDRESS]) = word;
   else if(address >= PXA260_MEMCTRL_BASE && address < PXA260_MEMCTRL_BASE + PXA260_BANK_SIZE)
      pxa260MemctrlWriteWord(address, word);
   else if(address >= PXA260_LCD_BASE && address < PXA260_LCD_BASE + PXA260_BANK_SIZE)
      pxa260_lcd_write_word(address, word);
   else if(address >= PXA260_IO_BASE && address < PXA260_IO_BASE + PXA260_BANK_SIZE)
      pxa260_io_write_word(address, word);
   else
      debugLog("Invalid word write at address: 0x%08X, value:0x%08X\n", address, word);
}

static Err mmuReadF(void* userData, UInt32* buf, UInt32 pa){
   *(uint32_t*)buf = read_word(pa);
   return 1;
}

static Boolean	uArmMemAccess(struct ArmCpu* cpu, void* buf, UInt32 vaddr, UInt8 size, Boolean write, Boolean priviledged, UInt8* fsr){
   if(!mmuTranslate(&uArmMmu, vaddr, priviledged, write, &vaddr, fsr))
      return false;

   if(write){
      switch(size){
         case 1:
            write_byte(vaddr, *(uint8_t*)buf);
            return true;

         case 2:
            write_half(vaddr, *(uint16_t*)buf);
            return true;

         case 4:
            write_word(vaddr, *(uint32_t*)buf);
            return true;

         default:
            debugLog("uARM wrote memory with invalid byte count:%d\n", size);
            return false;
      }
   }
   else{
      switch(size){
         case 1:
            *(uint8_t*)buf = read_byte(vaddr);
            return true;

         case 2:
            *(uint16_t*)buf = read_half(vaddr);
            return true;

         case 4:
            *(uint32_t*)buf = read_word(vaddr);
            return true;

         case 32:
            ((uint32_t*)buf)[0] = read_word(vaddr);
            ((uint32_t*)buf)[1] = read_word(vaddr + 4);
            ((uint32_t*)buf)[2] = read_word(vaddr + 8);
            ((uint32_t*)buf)[3] = read_word(vaddr + 12);
            ((uint32_t*)buf)[4] = read_word(vaddr + 16);
            ((uint32_t*)buf)[5] = read_word(vaddr + 20);
            ((uint32_t*)buf)[6] = read_word(vaddr + 24);
            ((uint32_t*)buf)[7] = read_word(vaddr + 28);
            return true;

         default:
            debugLog("uARM read memory with invalid byte count:%d\n", size);
            return false;
      }
   }
}

static Boolean	uArmHypercall(struct ArmCpu* cpu){
   //no hypercalls
   return true;
}

static void	uArmEmulErr	(struct ArmCpu* cpu, const char* err_str){
   debugLog("uARM error:%s\n", err_str);
}

static void	uArmSetFaultAddr(struct ArmCpu* cpu, UInt32 adr, UInt8 faultStatus){
   debugLog("uARM set fault addr:0x%08X, status:0x%02X\n", adr, faultStatus);
}
