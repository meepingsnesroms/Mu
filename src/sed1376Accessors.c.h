//data access
static uint32_t handlePanelDataSwaps(uint32_t address){
   //word swap
   if(sed1376Registers[SPECIAL_EFFECT] & 0x80)
      address ^= 0x00000002;
   //byte swap
   if(sed1376Registers[SPECIAL_EFFECT] & 0x40)
      address ^= 0x00000001;
   return address;
}

//color conversion
static uint16_t makeRgb16FromSed666(uint8_t r, uint8_t g, uint8_t b){
   //the Palm m515 display controller -> LCD color lines are swapped for red and blue, so blue is put at the top
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
   return lutMonochromeValue(sed1376Framebuffer[handlePanelDataSwaps(screenStartAddress + y * lineSize + x / 8)] >> (7 - x % 8) & 0x01);
}
static uint16_t get2BppMonochrome(uint16_t x, uint16_t y){
   return lutMonochromeValue(sed1376Framebuffer[handlePanelDataSwaps(screenStartAddress + y * lineSize + x / 4)] >> (6 - x % 4 * 2) & 0x03);
}
static uint16_t get4BppMonochrome(uint16_t x, uint16_t y){
   return lutMonochromeValue(sed1376Framebuffer[handlePanelDataSwaps(screenStartAddress + y * lineSize + x / 2)] >> (4 - x % 2 * 4) & 0x0F);
}
static uint16_t get8BppMonochrome(uint16_t x, uint16_t y){
   return lutMonochromeValue(sed1376Framebuffer[handlePanelDataSwaps(screenStartAddress + y * lineSize + x)]);
}
static uint16_t get16BppMonochrome(uint16_t x, uint16_t y){
   uint16_t pixelValue = sed1376Framebuffer[handlePanelDataSwaps(screenStartAddress + (y * lineSize + x) * 2)] << 8 | sed1376Framebuffer[handlePanelDataSwaps(screenStartAddress + (y * lineSize + x) * 2 + 1)];
   return makeRgb16FromGreenComponent(pixelValue);
}

//color
static uint16_t get1BppColor(uint16_t x, uint16_t y){
   return sed1376OutputLut[sed1376Framebuffer[handlePanelDataSwaps(screenStartAddress + y * lineSize + x / 8)] >> (7 - x % 8) & 0x01];
}
static uint16_t get2BppColor(uint16_t x, uint16_t y){
   return sed1376OutputLut[sed1376Framebuffer[handlePanelDataSwaps(screenStartAddress + y * lineSize + x / 4)] >> (6 - x % 4 * 2) & 0x03];
}
static uint16_t get4BppColor(uint16_t x, uint16_t y){
   return sed1376OutputLut[sed1376Framebuffer[handlePanelDataSwaps(screenStartAddress + y * lineSize + x / 2)] >> (4 - x % 2 * 4) & 0x0F];
}
static uint16_t get8BppColor(uint16_t x, uint16_t y){
   return sed1376OutputLut[sed1376Framebuffer[handlePanelDataSwaps(screenStartAddress + y * lineSize + x)]];
}
static uint16_t get16BppColor(uint16_t x, uint16_t y){
   return sed1376Framebuffer[handlePanelDataSwaps(screenStartAddress + (y * lineSize + x) * 2)] << 8 | sed1376Framebuffer[handlePanelDataSwaps(screenStartAddress + (y * lineSize + x) * 2 + 1)];
}

static void selectRenderer(bool color, uint8_t bpp){
   renderPixel = NULL;
   if(color){
      switch(bpp){
         case 1:
            renderPixel = get1BppColor;
            break;

         case 2:
            renderPixel = get2BppColor;
            break;

         case 4:
            renderPixel = get4BppColor;
            break;

         case 8:
            renderPixel = get8BppColor;
            break;

         case 16:
            renderPixel = get16BppColor;
            break;
      }
   }
   else{
      switch(bpp){
         case 1:
            renderPixel = get1BppMonochrome;
            break;

         case 2:
            renderPixel = get2BppMonochrome;
            break;

         case 4:
            renderPixel = get4BppMonochrome;
            break;

         case 8:
            renderPixel = get8BppMonochrome;
            break;

         case 16:
            renderPixel = get16BppMonochrome;
            break;
      }
   }
}

//updaters
static void updateLcdStatus(){
   bool backlightEnabled = sed1376Registers[GPIO_CONT_0] & sed1376Registers[GPIO_CONF_0] & 0x10;

   palmMisc.lcdOn = sed1376Registers[GPIO_CONT_0] & sed1376Registers[GPIO_CONF_0] & 0x20;
   palmMisc.backlightLevel = backlightEnabled ? (1 + backlightAmplifierState()) : 0;
}
