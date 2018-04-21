//data access
static inline uint32_t handlePanelDataSwaps(uint32_t address){
   if(sed1376Registers[SPECIAL_EFFECT] & 0x80){
      //word swap
      address ^= 0x00000002;
   }
   if(sed1376Registers[SPECIAL_EFFECT] & 0x40){
      //byte swap
      address ^= 0x00000001;
   }
   return address;
}

//color conversion
static inline uint16_t makeRgb16FromRgb666(uint8_t r, uint8_t g, uint8_t b){
   uint16_t color = r << 10 & 0xF800;
   color |= g << 5 & 0x07E0;
   color |= b >> 1 & 0x008F;
   return color;
}
static inline void makeRgb666FromRgb16(uint16_t color, uint8_t* r, uint8_t* g, uint8_t* b){
   *r = color >> 10 & 0x3E;
   *g = color >> 5 & 0x3F;
   *b = color << 1 & 0x3E;
}
static inline uint16_t lutMonochromeValue(uint8_t lutIndex){
   return makeRgb16FromRgb666(sed1376GLut[lutIndex], sed1376GLut[lutIndex], sed1376GLut[lutIndex]);
}

//monochrome, 0 degrees rotation
static inline uint16_t get1BppMonochrome0Degrees(uint32_t screenStartAddress, uint16_t lineSize, uint16_t x, uint16_t y){
   return lutMonochromeValue(sed1376Framebuffer[screenStartAddress + y * lineSize + x / 8] >> (7 - x % 8) & 0x01);
}
static inline uint16_t get2BppMonochrome0Degrees(uint32_t screenStartAddress, uint16_t lineSize, uint16_t x, uint16_t y){
   return lutMonochromeValue(sed1376Framebuffer[screenStartAddress + y * lineSize + x / 4] >> (6 - x % 4 * 2) & 0x03);
}
static inline uint16_t get4BppMonochrome0Degrees(uint32_t screenStartAddress, uint16_t lineSize, uint16_t x, uint16_t y){
   return lutMonochromeValue(sed1376Framebuffer[screenStartAddress + y * lineSize + x / 2] >> (4 - x % 2 * 4) & 0x0F);
}
static inline uint16_t get8BppMonochrome0Degrees(uint32_t screenStartAddress, uint16_t lineSize, uint16_t x, uint16_t y){
   return lutMonochromeValue(sed1376Framebuffer[screenStartAddress + y * lineSize + x]);
}
static inline uint16_t get16BppMonochrome0Degrees(uint32_t screenStartAddress, uint16_t lineSize, uint16_t x, uint16_t y){
   uint16_t pixelValue = (sed1376Framebuffer[screenStartAddress + (y * lineSize + x) * 2] << 8 | sed1376Framebuffer[screenStartAddress + (y * lineSize + x) * 2 + 1]) >> 5 & 0x3F;
   return makeRgb16FromRgb666(pixelValue, pixelValue, pixelValue);
}

//color,  0 degrees rotation
static inline uint16_t get1BppColor0Degrees(uint32_t screenStartAddress, uint16_t lineSize, uint16_t x, uint16_t y){
   return sed1376OutputLut[sed1376Framebuffer[screenStartAddress + y * lineSize + x / 8] >> (7 - x % 8) & 0x01];
}
static inline uint16_t get2BppColor0Degrees(uint32_t screenStartAddress, uint16_t lineSize, uint16_t x, uint16_t y){
   return sed1376OutputLut[sed1376Framebuffer[screenStartAddress + y * lineSize + x / 4] >> (6 - x % 4 * 2) & 0x03];
}
static inline uint16_t get4BppColor0Degrees(uint32_t screenStartAddress, uint16_t lineSize, uint16_t x, uint16_t y){
   return sed1376OutputLut[sed1376Framebuffer[screenStartAddress + y * lineSize + x / 2] >> (4 - x % 2 * 4) & 0x0F];
}
static inline uint16_t get8BppColor0Degrees(uint32_t screenStartAddress, uint16_t lineSize, uint16_t x, uint16_t y){
   return sed1376OutputLut[sed1376Framebuffer[screenStartAddress + y * lineSize + x]];
}
static inline uint16_t get16BppColor0Degrees(uint32_t screenStartAddress, uint16_t lineSize, uint16_t x, uint16_t y){
   return sed1376Framebuffer[screenStartAddress + (y * lineSize + x) * 2] << 8 | sed1376Framebuffer[screenStartAddress + (y * lineSize + x) * 2 + 1];
}
