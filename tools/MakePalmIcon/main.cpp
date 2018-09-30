#include <QCoreApplication>
#include <QSvgRenderer>
#include <QPainter>
#include <QImage>
#include <QColor>
#include <QRectF>
#include <QFile>
#include <QString>

#include <stdio.h>
#include <stdint.h>
#include <string.h>


#define BITMAP_HEADER_SIZE 16
#define MAX_PALM_BITMAP_SIZE (0xFFFF * 4)


static inline void writeBe16(uint8_t* data, uint16_t value){
#if Q_BYTE_ORDER != Q_BIG_ENDIAN
   value = value >> 8 | value << 8;
#endif
   ((uint16_t*)data)[0] = value;
}

static inline uint16_t getRowBytes(int16_t width, uint8_t bitsPerPixel){
   uint16_t rowBytes = width * bitsPerPixel / 8;

   //add 1 byte at the end for leftover bits
   if(width * bitsPerPixel % 8 > 0)
      rowBytes += 1;

   //Palm OS crashes from 16 bit accesses on odd addresses, so this is likely the same
   if(rowBytes & 1)
      rowBytes += 1;

   return rowBytes;
}

static inline uint16_t getNextDepthOffset(int16_t width, int16_t height, uint8_t bitsPerPixel){
   uint32_t nextDepthOffset = 0;

   nextDepthOffset += BITMAP_HEADER_SIZE;
   //nextDepthOffset += 0xFF;//custom palette is not supported
   nextDepthOffset += getRowBytes(width, bitsPerPixel) * height;

   return nextDepthOffset / 4;
}

void renderTo1Bit(uint8_t* data, const QImage& image){
   uint16_t rowBytes = getRowBytes(image.width(), 1);

   memset(data, 0x00, rowBytes * image.height());
   for(int16_t y = 0; y < image.height(); y++){
      for(int16_t x = 0; x < image.width(); x++){
         QColor pixel = QColor(image.pixel(x, y));
         bool pixelAveraged = (pixel.redF() + pixel.greenF() + pixel.blueF()) / 3.0 > 0.5;

         if(pixelAveraged)
            data[y * rowBytes + x / 8] |= 1 << 7 - x % 8;
      }
   }
}

void renderTo2Bits(uint8_t* data, const QImage& image){
   uint16_t rowBytes = getRowBytes(image.width(), 2);

   memset(data, 0x00, rowBytes * image.height());
   for(int16_t y = 0; y < image.height(); y++){
      for(int16_t x = 0; x < image.width(); x++){
         QColor pixel = QColor(image.pixel(x, y));
         uint8_t pixelAveraged = (pixel.redF() + pixel.greenF() + pixel.blueF()) / 3.0 * 4.0;

         data[y * rowBytes + x / 4] |= pixelAveraged << 6 - x % 4 * 2;
      }
   }
}

void renderTo4Bits(uint8_t* data, const QImage& image){
   uint16_t rowBytes = getRowBytes(image.width(), 4);

   memset(data, 0x00, rowBytes * image.height());
   for(int16_t y = 0; y < image.height(); y++){
      for(int16_t x = 0; x < image.width(); x++){
         QColor pixel = QColor(image.pixel(x, y));
         uint8_t pixelAveraged = (pixel.redF() + pixel.greenF() + pixel.blueF()) / 3.0 * 16.0;

         data[y * rowBytes + x / 2] |= pixelAveraged << (x % 2 ? 0 : 4);
      }
   }
}

void renderTo8Bits(uint8_t* data, const QImage& image){
   uint16_t rowBytes = getRowBytes(image.width(), 8);

   memset(data, 0x00, rowBytes * image.height());
   for(int16_t y = 0; y < image.height(); y++){
      for(int16_t x = 0; x < image.width(); x++){
         QColor pixel = QColor(image.pixel(x, y));

         //may need to dump the LUT from a Palm ROM
         data[y * rowBytes + x] = (pixel.red() >> 5 & 0x07) << 5 | (pixel.green() >> 5 & 0x07) << 2 | (pixel.blue() >> 6 & 0x03);
      }
   }
}

void renderTo16Bits(uint8_t* data, const QImage& image){
   uint16_t rowBytes = getRowBytes(image.width(), 16);

   memset(data, 0x00, rowBytes * image.height());
   for(int16_t y = 0; y < image.height(); y++){
      for(int16_t x = 0; x < image.width(); x++){
         QColor pixel = QColor(image.pixel(x, y));
         uint16_t pixel16 = (pixel.red() >> 3 & 0x001F) << 11 | (pixel.green() >> 2 & 0x003F) << 5 | (pixel.blue() >> 3 & 0x001F);

         writeBe16(data + y * rowBytes + x * 2, pixel16);
      }
   }
}

uint32_t renderPalmIcon(const QString& svg, uint8_t* output, int16_t width, int16_t height, uint8_t bitsPerPixel, bool lastBitmap){
   QImage canvas(width, height, QImage::Format_RGB16);
   QPainter painter(&canvas);
   QSvgRenderer svgRenderer(svg);
   uint32_t offset = 0;

   //clear buffer to white and render
   for(int16_t count = 0; count < height; count++)
      memset(canvas.scanLine(count), 0xFF, width * sizeof(uint16_t));
   svgRenderer.render(&painter);

   //write a Palm bitmap struct
   uint8_t bitmapVersion = bitsPerPixel > 4 ? 2 : 1;

   writeBe16(output + offset, width);//width
   offset += sizeof(int16_t);
   writeBe16(output + offset, height);//height
   offset += sizeof(int16_t);
   writeBe16(output + offset, getRowBytes(width, bitsPerPixel));//rowBytes
   offset += sizeof(uint16_t);
   writeBe16(output + offset, bitsPerPixel == 16 ? 0x0400 : 0x0000);//bitmapFlags, only directColor is implemented, not implemented, prevents using transparency and color tables
   offset += sizeof(uint16_t);
   output[offset] = bitsPerPixel;//pixelSize
   offset += sizeof(uint8_t);
   output[offset] = bitmapVersion;//version
   offset += sizeof(uint8_t);
   writeBe16(output + offset, lastBitmap ? 0 : getNextDepthOffset(width, height, bitsPerPixel));//nextDepthOffset
   offset += sizeof(uint16_t);
   if(bitmapVersion > 1){
      output[offset] = 0xFF;//transparentIndex, white is transparent, not doing this causes issues on Palm OS5+
      offset += sizeof(uint8_t);
      output[offset] = 0xFF;//compressionType
      offset += sizeof(uint8_t);
   }
   else{
      output[offset] = 0x00;//reserved
      offset += sizeof(uint8_t);
      output[offset] = 0x00;//reserved
      offset += sizeof(uint8_t);
   }
   output[offset] = 0x00;//reserved
   offset += sizeof(uint8_t);
   output[offset] = 0x00;//reserved
   offset += sizeof(uint8_t);
   //0xFF look up table goes here when active, custom LUT is currently unsupported
   switch(bitsPerPixel){
      case 1:
         renderTo1Bit(output + offset, canvas);
         break;

      case 2:
         renderTo2Bits(output + offset, canvas);
         break;

      case 4:
         renderTo4Bits(output + offset, canvas);
         break;

      case 8:
         renderTo8Bits(output + offset, canvas);
         break;

      case 16:
         renderTo16Bits(output + offset, canvas);
         break;
   }
   offset += getRowBytes(width, bitsPerPixel) * height;

   return offset;
}

void convertToPalmIcons(const QString& svg, const QString& outputDirectory){
   QFile taib03E8File(outputDirectory + "/tAIB03E8.bin");
   QFile taib03E9File(outputDirectory + "/tAIB03E9.bin");
   uint8_t* taib03E8 = new uint8_t[MAX_PALM_BITMAP_SIZE * 5];
   uint8_t* taib03E9 = new uint8_t[MAX_PALM_BITMAP_SIZE * 5];
   uint32_t taib03E8Offset = 0;
   uint32_t taib03E9Offset = 0;

   taib03E8Offset += renderPalmIcon(svg, taib03E8 + taib03E8Offset, 32, 32, 1, false);//1bpp, greyscale
   taib03E8Offset += renderPalmIcon(svg, taib03E8 + taib03E8Offset, 32, 32, 2, false);//2bpp, greyscale
   taib03E8Offset += renderPalmIcon(svg, taib03E8 + taib03E8Offset, 32, 32, 4, false);//4bpp, greyscale
   taib03E8Offset += renderPalmIcon(svg, taib03E8 + taib03E8Offset, 32, 32, 8, false);//8bpp, color
   taib03E8Offset += renderPalmIcon(svg, taib03E8 + taib03E8Offset, 32, 32, 16, true);//16bpp, color

   taib03E9Offset += renderPalmIcon(svg, taib03E9 + taib03E9Offset, 15, 9, 1, false);//1bpp, greyscale
   taib03E9Offset += renderPalmIcon(svg, taib03E9 + taib03E9Offset, 15, 9, 2, false);//2bpp, greyscale
   taib03E9Offset += renderPalmIcon(svg, taib03E9 + taib03E9Offset, 15, 9, 4, false);//4bpp, greyscale
   taib03E9Offset += renderPalmIcon(svg, taib03E9 + taib03E9Offset, 15, 9, 8, false);//8bpp, color
   taib03E9Offset += renderPalmIcon(svg, taib03E9 + taib03E9Offset, 15, 9, 16, true);//16bpp, color

   if(taib03E8File.open(QFile::WriteOnly | QFile::Truncate)){
      taib03E8File.write((const char*)taib03E8, taib03E8Offset);
      taib03E8File.close();
   }
   if(taib03E9File.open(QFile::WriteOnly | QFile::Truncate)){
      taib03E9File.write((const char*)taib03E9, taib03E9Offset);
      taib03E9File.close();
   }
}

int main(int argc, char* argv[]){
   if(argc == 3){
      //render .svg to all sizes of Palm OS icon with the proper names for prc-tools
      convertToPalmIcons(QString(argv[1]), QString(argv[2]));
   }
   else{
      //invalid parameters
      printf("MakePalmIcon v1.0\n");
      printf("A replacement for the pilrc bitmap converter, which seems to be broken on 64 bit systems.\n");
      printf("Format:\"/path/to/image.svg\" \"/path/to/palm/application/directory\"\n");
      printf("Fleas have pet rabbits!\n");
   }

   return 0;
}
