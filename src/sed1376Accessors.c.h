//data access
static uint32_t handlePanelDataSwaps(uint32_t address){
#if !defined(EMU_NO_SAFETY)
   //word swap
   if(sed1376Registers[SPECIAL_EFFECT] & 0x80)
      address ^= 0x00000002;
#endif
   //byte swap, used in 16 bpp mode
   if(sed1376Registers[SPECIAL_EFFECT] & 0x40)
      address ^= 0x00000001;
   return address;
}

//color conversion
static uint16_t makeRgb16FromSed666(uint8_t r, uint8_t g, uint8_t b){
   uint16_t color = r >> 2 << 10 & 0xF800;
   color |= g >> 2 << 5 & 0x07E0;
   color |= b >> 2 >> 1 & 0x001F;
   return color;
}
static uint16_t makeRgb16FromGreenComponent(uint16_t g){
   uint16_t color = g;
   color |= g >> 6;
   color |= g << 5 & 0xF1;
   return color;
}
static uint16_t lutMonochromeValue(uint8_t lutIndex){
   return makeRgb16FromSed666(sed1376GLut[lutIndex], sed1376GLut[lutIndex], sed1376GLut[lutIndex]);
}

//monochrome
static uint16_t get1BppMonochrome(uint16_t x, uint16_t y){
   return lutMonochromeValue(sed1376Ram[handlePanelDataSwaps(sed1376ScreenStartAddress + y * sed1376LineSize + x / 8)] >> (7 - x % 8) & 0x01);
}
static uint16_t get2BppMonochrome(uint16_t x, uint16_t y){
   return lutMonochromeValue(sed1376Ram[handlePanelDataSwaps(sed1376ScreenStartAddress + y * sed1376LineSize + x / 4)] >> (6 - x % 4 * 2) & 0x03);
}
static uint16_t get4BppMonochrome(uint16_t x, uint16_t y){
   return lutMonochromeValue(sed1376Ram[handlePanelDataSwaps(sed1376ScreenStartAddress + y * sed1376LineSize + x / 2)] >> (4 - x % 2 * 4) & 0x0F);
}
static uint16_t get8BppMonochrome(uint16_t x, uint16_t y){
   return lutMonochromeValue(sed1376Ram[handlePanelDataSwaps(sed1376ScreenStartAddress + y * sed1376LineSize + x)]);
}
static uint16_t get16BppMonochrome(uint16_t x, uint16_t y){
   uint16_t pixelValue = sed1376Ram[handlePanelDataSwaps(sed1376ScreenStartAddress + (y * sed1376LineSize + x) * 2)] << 8 | sed1376Ram[handlePanelDataSwaps(sed1376ScreenStartAddress + (y * sed1376LineSize + x) * 2 + 1)];
   return makeRgb16FromGreenComponent(pixelValue);
}

//color
static uint16_t get1BppColor(uint16_t x, uint16_t y){
   return sed1376OutputLut[sed1376Ram[handlePanelDataSwaps(sed1376ScreenStartAddress + y * sed1376LineSize + x / 8)] >> (7 - x % 8) & 0x01];
}
static uint16_t get2BppColor(uint16_t x, uint16_t y){
   return sed1376OutputLut[sed1376Ram[handlePanelDataSwaps(sed1376ScreenStartAddress + y * sed1376LineSize + x / 4)] >> (6 - x % 4 * 2) & 0x03];
}
static uint16_t get4BppColor(uint16_t x, uint16_t y){
   return sed1376OutputLut[sed1376Ram[handlePanelDataSwaps(sed1376ScreenStartAddress + y * sed1376LineSize + x / 2)] >> (4 - x % 2 * 4) & 0x0F];
}
static uint16_t get8BppColor(uint16_t x, uint16_t y){
   return sed1376OutputLut[sed1376Ram[handlePanelDataSwaps(sed1376ScreenStartAddress + y * sed1376LineSize + x)]];
}
static uint16_t get16BppColor(uint16_t x, uint16_t y){
   //this format is little endian, to use big endian data sed1376Registers[SPECIAL_EFFECT] & 0x40 must be set
   return sed1376Ram[handlePanelDataSwaps(sed1376ScreenStartAddress + (y * sed1376LineSize + x * 2) + 1)] << 8 | sed1376Ram[handlePanelDataSwaps(sed1376ScreenStartAddress + (y * sed1376LineSize + x * 2))];
}

static void selectRenderer(bool color, uint8_t bpp){
   sed1376RenderPixel = NULL;
   if(color){
      switch(bpp){
         case 1:
            sed1376RenderPixel = get1BppColor;
            break;

         case 2:
            sed1376RenderPixel = get2BppColor;
            break;

         case 4:
            sed1376RenderPixel = get4BppColor;
            break;

         case 8:
            sed1376RenderPixel = get8BppColor;
            break;

         case 16:
            sed1376RenderPixel = get16BppColor;
            break;

         default:
            debugLog("SED1376 invalid color bpp:%d\n", bpp);
            break;
      }
   }
   else{
      switch(bpp){
         case 1:
            sed1376RenderPixel = get1BppMonochrome;
            break;

         case 2:
            sed1376RenderPixel = get2BppMonochrome;
            break;

         case 4:
            sed1376RenderPixel = get4BppMonochrome;
            break;

         case 8:
            sed1376RenderPixel = get8BppMonochrome;
            break;

         case 16:
            sed1376RenderPixel = get16BppMonochrome;
            break;

         default:
            debugLog("SED1376 invalid grayscale bpp:%d\n", bpp);
            break;
      }
   }
}
