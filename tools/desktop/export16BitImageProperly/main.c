#include <stdio.h>

#include IMPORT_HEADER


#define ENTRYS_PER_LINE 10


static unsigned short makeRgb16FromRgb24(unsigned char r, unsigned char g, unsigned char b){
   unsigned short color16;
   color16 = r << 8 & 0xF800;
   color16 |= g << 3 & 0x07E0;
   color16 |= b >> 3 & 0x001F;
   return color16;
}

int main(int argc, const char* argv[]){
   printf("#include <stdint.h>\n");
   printf("\n");
   printf("\n");
   printf("const uint16_t %s[%d * %d] = {\n", argv[1], width, height);
   printf("   ");
   
   int triggerNextLine = 0;
   unsigned char* pixelData = header_data;
   unsigned char pixel[3];
   for(int count = 0; count < width * height; count++){
      HEADER_PIXEL(header_data, pixel);
      
      printf("0x%04X,", makeRgb16FromRgb24(pixel[0], pixel[1], pixel[2]));
      
      triggerNextLine++;
      if(triggerNextLine >= ENTRYS_PER_LINE){
         printf("\n   ");
         triggerNextLine = 0;
      }
   }
   
   printf("\n};");
   printf("\n");
   
   return 0;
}
