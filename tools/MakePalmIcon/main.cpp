#include <QCoreApplication>
#include <QSvgRenderer>
#include <QPainter>
#include <QImage>
#include <QFile>
#include <QString>

#include <stdio.h>
#include <stdint.h>


#define MAX_PALM_BITMAP_SIZE 0xFFFF


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

   return rowBytes;
}

static inline uint16_t getBitmapFlags(uint8_t version){

}

static inline uint16_t getNextDepthOffset(int16_t width, int16_t height, uint8_t bitsPerPixel){

}

uint32_t renderPalmIcon(const QString& svg, uint8_t* output, int16_t width, int16_t height, uint8_t bitsPerPixel, bool color){
   QImage canvas(width, height, QImage::Format_RGB16);
   QPainter painter(&canvas);
   QSvgRenderer svgRenderer(svg);
   uint32_t offset = 0;

   svgRenderer.render(&painter);

   //write a Palm bitmap struct
   uint8_t bitmapVersion = color ? 2 : 1;

   writeBe16(output + offset, width);//width
   offset += sizeof(uint16_t);
   writeBe16(output + offset, height);//height
   offset += sizeof(uint16_t);
   writeBe16(output + offset, getRowBytes(width, bitsPerPixel));//rowBytes
   offset += sizeof(uint16_t);
   writeBe16(output + offset, getBitmapFlags(bitmapVersion));//bitmapFlags
   offset += sizeof(uint16_t);
   output[offset] = bitsPerPixel;//pixelSize
   offset += sizeof(uint8_t);
   output[offset] = bitmapVersion;//version
   offset += sizeof(uint8_t);
   writeBe16(output + offset, getNextDepthOffset(width, height, bitsPerPixel));//nextDepthOffset
   offset += sizeof(uint16_t);
   if(bitmapVersion > 1){
      output[offset] = 0x00;//transparentIndex, fixme
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

   return offset;
}

void convertToPalmIcons(const QString& svg, const QString& outputDirectory){
   QFile taib03E8File(outputDirectory + "/tAIB03E8.bin");
   QFile taib03E9File(outputDirectory + "/tAIB03E9.bin");
   uint8_t* taib03E8 = new uint8_t[MAX_PALM_BITMAP_SIZE * 6];
   uint8_t* taib03E9 = new uint8_t[MAX_PALM_BITMAP_SIZE * 6];
   uint32_t taib03E8Offset = 0;
   uint32_t taib03E9Offset = 0;

   taib03E8Offset += renderPalmIcon(svg, taib03E8 + taib03E8Offset, 32, 32, 1, false);//1bpp, greyscale
   taib03E8Offset += renderPalmIcon(svg, taib03E8 + taib03E8Offset, 32, 32, 2, false);//2bpp, greyscale
   taib03E8Offset += renderPalmIcon(svg, taib03E8 + taib03E8Offset, 32, 32, 4, false);//4bpp, greyscale
   taib03E8Offset += renderPalmIcon(svg, taib03E8 + taib03E8Offset, 32, 32, 8, false);//8bpp, greyscale
   taib03E8Offset += renderPalmIcon(svg, taib03E8 + taib03E8Offset, 32, 32, 8, true);//8bpp, color
   taib03E8Offset += renderPalmIcon(svg, taib03E8 + taib03E8Offset, 32, 32, 16, true);//16bpp, color

   taib03E9Offset += renderPalmIcon(svg, taib03E9 + taib03E9Offset, 15, 9, 1, false);//1bpp, greyscale
   taib03E9Offset += renderPalmIcon(svg, taib03E9 + taib03E9Offset, 15, 9, 2, false);//2bpp, greyscale
   taib03E9Offset += renderPalmIcon(svg, taib03E9 + taib03E9Offset, 15, 9, 4, false);//4bpp, greyscale
   taib03E9Offset += renderPalmIcon(svg, taib03E9 + taib03E9Offset, 15, 9, 8, false);//8bpp, greyscale
   taib03E9Offset += renderPalmIcon(svg, taib03E9 + taib03E9Offset, 15, 9, 8, true);//8bpp, color
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
      printf("Fleas have pet rabbits!\n");
   }

   return 0;
}
