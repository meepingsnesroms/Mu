#include <QCoreApplication>
#include <QSvgRenderer>
#include <QPainter>
#include <QImage>
#include <QColor>
#include <QRectF>
#include <QFile>
#include <QFileInfo>
#include <QString>

#include <stdio.h>
#include <stdint.h>
#include <string.h>


#define BITMAP_HEADER_SIZE 16
#define BITMAP_DIRECT_INFO_TYPE_SIZE 8
#define MAX_PALM_BITMAP_SIZE (0xFFFF * 4)


static const uint8_t palmPalette8Bpp[256][3] = {
  { 255, 255, 255 }, { 255, 204, 255 }, { 255, 153, 255 }, { 255, 102, 255 },
  { 255,  51, 255 }, { 255,   0, 255 }, { 255, 255, 204 }, { 255, 204, 204 },
  { 255, 153, 204 }, { 255, 102, 204 }, { 255,  51, 204 }, { 255,   0, 204 },
  { 255, 255, 153 }, { 255, 204, 153 }, { 255, 153, 153 }, { 255, 102, 153 },
  { 255,  51, 153 }, { 255,   0, 153 }, { 204, 255, 255 }, { 204, 204, 255 },
  { 204, 153, 255 }, { 204, 102, 255 }, { 204,  51, 255 }, { 204,   0, 255 },
  { 204, 255, 204 }, { 204, 204, 204 }, { 204, 153, 204 }, { 204, 102, 204 },
  { 204,  51, 204 }, { 204,   0, 204 }, { 204, 255, 153 }, { 204, 204, 153 },
  { 204, 153, 153 }, { 204, 102, 153 }, { 204,  51, 153 }, { 204,   0, 153 },
  { 153, 255, 255 }, { 153, 204, 255 }, { 153, 153, 255 }, { 153, 102, 255 },
  { 153,  51, 255 }, { 153,   0, 255 }, { 153, 255, 204 }, { 153, 204, 204 },
  { 153, 153, 204 }, { 153, 102, 204 }, { 153,  51, 204 }, { 153,   0, 204 },
  { 153, 255, 153 }, { 153, 204, 153 }, { 153, 153, 153 }, { 153, 102, 153 },
  { 153,  51, 153 }, { 153,   0, 153 }, { 102, 255, 255 }, { 102, 204, 255 },
  { 102, 153, 255 }, { 102, 102, 255 }, { 102,  51, 255 }, { 102,   0, 255 },
  { 102, 255, 204 }, { 102, 204, 204 }, { 102, 153, 204 }, { 102, 102, 204 },
  { 102,  51, 204 }, { 102,   0, 204 }, { 102, 255, 153 }, { 102, 204, 153 },
  { 102, 153, 153 }, { 102, 102, 153 }, { 102,  51, 153 }, { 102,   0, 153 },
  {  51, 255, 255 }, {  51, 204, 255 }, {  51, 153, 255 }, {  51, 102, 255 },
  {  51,  51, 255 }, {  51,   0, 255 }, {  51, 255, 204 }, {  51, 204, 204 },
  {  51, 153, 204 }, {  51, 102, 204 }, {  51,  51, 204 }, {  51,   0, 204 },
  {  51, 255, 153 }, {  51, 204, 153 }, {  51, 153, 153 }, {  51, 102, 153 },
  {  51,  51, 153 }, {  51,   0, 153 }, {   0, 255, 255 }, {   0, 204, 255 },
  {   0, 153, 255 }, {   0, 102, 255 }, {   0,  51, 255 }, {   0,   0, 255 },
  {   0, 255, 204 }, {   0, 204, 204 }, {   0, 153, 204 }, {   0, 102, 204 },
  {   0,  51, 204 }, {   0,   0, 204 }, {   0, 255, 153 }, {   0, 204, 153 },
  {   0, 153, 153 }, {   0, 102, 153 }, {   0,  51, 153 }, {   0,   0, 153 },
  { 255, 255, 102 }, { 255, 204, 102 }, { 255, 153, 102 }, { 255, 102, 102 },
  { 255,  51, 102 }, { 255,   0, 102 }, { 255, 255,  51 }, { 255, 204,  51 },
  { 255, 153,  51 }, { 255, 102,  51 }, { 255,  51,  51 }, { 255,   0,  51 },
  { 255, 255,   0 }, { 255, 204,   0 }, { 255, 153,   0 }, { 255, 102,   0 },
  { 255,  51,   0 }, { 255,   0,   0 }, { 204, 255, 102 }, { 204, 204, 102 },
  { 204, 153, 102 }, { 204, 102, 102 }, { 204,  51, 102 }, { 204,   0, 102 },
  { 204, 255,  51 }, { 204, 204,  51 }, { 204, 153,  51 }, { 204, 102,  51 },
  { 204,  51,  51 }, { 204,   0,  51 }, { 204, 255,   0 }, { 204, 204,   0 },
  { 204, 153,   0 }, { 204, 102,   0 }, { 204,  51,   0 }, { 204,   0,   0 },
  { 153, 255, 102 }, { 153, 204, 102 }, { 153, 153, 102 }, { 153, 102, 102 },
  { 153,  51, 102 }, { 153,   0, 102 }, { 153, 255,  51 }, { 153, 204,  51 },
  { 153, 153,  51 }, { 153, 102,  51 }, { 153,  51,  51 }, { 153,   0,  51 },
  { 153, 255,   0 }, { 153, 204,   0 }, { 153, 153,   0 }, { 153, 102,   0 },
  { 153,  51,   0 }, { 153,   0,   0 }, { 102, 255, 102 }, { 102, 204, 102 },
  { 102, 153, 102 }, { 102, 102, 102 }, { 102,  51, 102 }, { 102,   0, 102 },
  { 102, 255,  51 }, { 102, 204,  51 }, { 102, 153,  51 }, { 102, 102,  51 },
  { 102,  51,  51 }, { 102,   0,  51 }, { 102, 255,   0 }, { 102, 204,   0 },
  { 102, 153,   0 }, { 102, 102,   0 }, { 102,  51,   0 }, { 102,   0,   0 },
  {  51, 255, 102 }, {  51, 204, 102 }, {  51, 153, 102 }, {  51, 102, 102 },
  {  51,  51, 102 }, {  51,   0, 102 }, {  51, 255,  51 }, {  51, 204,  51 },
  {  51, 153,  51 }, {  51, 102,  51 }, {  51,  51,  51 }, {  51,   0,  51 },
  {  51, 255,   0 }, {  51, 204,   0 }, {  51, 153,   0 }, {  51, 102,   0 },
  {  51,  51,   0 }, {  51,   0,   0 }, {   0, 255, 102 }, {   0, 204, 102 },
  {   0, 153, 102 }, {   0, 102, 102 }, {   0,  51, 102 }, {   0,   0, 102 },
  {   0, 255,  51 }, {   0, 204,  51 }, {   0, 153,  51 }, {   0, 102,  51 },
  {   0,  51,  51 }, {   0,   0,  51 }, {   0, 255,   0 }, {   0, 204,   0 },
  {   0, 153,   0 }, {   0, 102,   0 }, {   0,  51,   0 }, {  17,  17,  17 },
  {  34,  34,  34 }, {  68,  68,  68 }, {  85,  85,  85 }, { 119, 119, 119 },
  { 136, 136, 136 }, { 170, 170, 170 }, { 187, 187, 187 }, { 221, 221, 221 },
  { 238, 238, 238 }, { 192, 192, 192 }, { 128,   0,   0 }, { 128,   0, 128 },
  {   0, 128,   0 }, {   0, 128, 128 }, {   0,   0,   0 }, {   0,   0,   0 },
  {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 },
  {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 },
  {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 },
  {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 },
  {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 },
  {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 }
};


static inline uint16_t getRgbDiff(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2){
   return qMax(r1, r2) - qMin(r1, r2) + qMax(g1, g2) - qMin(g1, g2) + qMax(b1, b2) - qMin(b1, b2);
}

static inline uint8_t getBest8BppIndex(uint8_t r, uint8_t g, uint8_t b){
   uint16_t closeness = 0xFFFF;
   uint8_t bestIndex;

   for(uint16_t count = 0; count < 256; count++){
      uint16_t thisDiff = getRgbDiff(r, g, b, palmPalette8Bpp[count][0], palmPalette8Bpp[count][1], palmPalette8Bpp[count][2]);

      if(thisDiff < closeness){
         closeness = thisDiff;
         bestIndex = count;
      }
   }

   return bestIndex;
}

static inline void writeBe16(uint8_t* data, uint16_t value){
   data[0] = value >> 8;
   data[1] = value & 0xFF;
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
   uint32_t nextDepthOffset = BITMAP_HEADER_SIZE;

   if(bitsPerPixel == 16)
      nextDepthOffset += BITMAP_DIRECT_INFO_TYPE_SIZE;
   //custom palette is not supported right now
   //if(bitsPerPixel == 8)
   //   nextDepthOffset += 0xFF;
   nextDepthOffset += getRowBytes(width, bitsPerPixel) * height;

   //must be 32 bit aligned
   if(nextDepthOffset & 3)
      nextDepthOffset = (nextDepthOffset & ~3) + 4;

   return nextDepthOffset / 4;
}

void renderTo1Bit(uint8_t* data, const QImage& image){
   uint16_t rowBytes = getRowBytes(image.width(), 1);

   memset(data, 0x00, rowBytes * image.height());
   for(int16_t y = 0; y < image.height(); y++){
      for(int16_t x = 0; x < image.width(); x++){
         QColor pixel = QColor(image.pixel(x, y));
         bool pixelAveraged = (pixel.redF() + pixel.greenF() + pixel.blueF()) / 3.0 < 0.5;

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

         if(pixel.red() == 0xFF && pixel.green() == 0xFF && pixel.blue() == 0xFF)
            data[y * rowBytes + x] = 0xFF;//transparentIndex
         else
            data[y * rowBytes + x] = getBest8BppIndex(pixel.red(), pixel.green(), pixel.blue());
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

uint32_t renderPalmBitmap(const QImage& bitmap, uint8_t* output, uint8_t bitsPerPixel, bool lastBitmap){
   /*
   QFileInfo imageSource(path);
   QImage canvas(width, height, QImage::Format_RGB32);
   QPainter painter(&canvas);
   uint32_t offset = 0;

   //clear buffer to white and render
   canvas.fill(QColor(0xFF, 0xFF, 0xFF).rgb());
   if(imageSource.isDir()){
      //use image of correct size for Palm icon
      //$DIR/icon$WIDTHx$HEIGHTx$BPP.png", doesnt actually have to be a png, just has to have that extension
      QImage icon(path + "/icon" + QString::number(width) + "x" + QString::number(height) + ".png");

      painter.drawImage(0, 0, icon);
   }
   else if(imageSource.isFile() && imageSource.suffix() == "svg"){
      //render SVG to Palm icon
      QSvgRenderer svgRenderer(path);

      svgRenderer.render(&painter);
   }
   */

   //write a Palm bitmap struct
   uint32_t offset = 0;
   uint8_t bitmapVersion = bitsPerPixel <= 4 ? 1 : 2;

   writeBe16(output + offset, bitmap.width());//width
   offset += sizeof(int16_t);
   writeBe16(output + offset, bitmap.height());//height
   offset += sizeof(int16_t);
   writeBe16(output + offset, getRowBytes(bitmap.width(), bitsPerPixel));//rowBytes
   offset += sizeof(uint16_t);
   writeBe16(output + offset, (bitsPerPixel == 8 ? 0x2000 : 0x0000) | (bitsPerPixel == 16 ? 0x0400 : 0x0000));//bitmapFlags, only hasTransparency and directColor are implemented
   offset += sizeof(uint16_t);
   if(bitmapVersion >= 1){
      output[offset] = bitsPerPixel;//pixelSize
      offset += sizeof(uint8_t);
      output[offset] = bitmapVersion;//version
      offset += sizeof(uint8_t);
      writeBe16(output + offset, lastBitmap ? 0 : getNextDepthOffset(bitmap.width(), bitmap.height(), bitsPerPixel));//nextDepthOffset
      offset += sizeof(uint16_t);
      if(bitmapVersion >= 2){
         output[offset] = 0xFF;//transparentIndex, white is transparent, not doing this causes issues on Palm OS5+
         offset += sizeof(uint8_t);
         output[offset] = 0xFF;//compressionType, none
         offset += sizeof(uint8_t);
         writeBe16(output + offset, 0x0000);//reserved
         offset += sizeof(uint16_t);
         if(bitsPerPixel == 16){
            //add BitmapDirectInfoType
            output[offset] = 0x05;//redBits
            offset += sizeof(uint8_t);
            output[offset] = 0x06;//greenBits
            offset += sizeof(uint8_t);
            output[offset] = 0x05;//blueBits
            offset += sizeof(uint8_t);
            output[offset] = 0x00;//reserved
            offset += sizeof(uint8_t);

            //RGBColorType transparentColor
            output[offset] = 0xFF;//index
            offset += sizeof(uint8_t);
            output[offset] = 0x1F;//r
            offset += sizeof(uint8_t);
            output[offset] = 0x3F;//g
            offset += sizeof(uint8_t);
            output[offset] = 0x1F;//b
            offset += sizeof(uint8_t);
         }
         //0xFF look up table goes here when active, custom LUT is currently unsupported
      }
      else{
         //bitmapV1
         writeBe16(output + offset, 0x0000);//reserved
         offset += sizeof(uint16_t);
         writeBe16(output + offset, 0x0000);//reserved
         offset += sizeof(uint16_t);
      }
   }
   else{
      //bitmapV0
      writeBe16(output + offset, 0x0000);//reserved
      offset += sizeof(uint16_t);
      writeBe16(output + offset, 0x0000);//reserved
      offset += sizeof(uint16_t);
      writeBe16(output + offset, 0x0000);//reserved
      offset += sizeof(uint16_t);
      writeBe16(output + offset, 0x0000);//reserved
      offset += sizeof(uint16_t);
   }
   switch(bitsPerPixel){
      case 1:
         renderTo1Bit(output + offset, bitmap);
         break;

      case 2:
         renderTo2Bits(output + offset, bitmap);
         break;

      case 4:
         renderTo4Bits(output + offset, bitmap);
         break;

      case 8:
         renderTo8Bits(output + offset, bitmap);
         break;

      case 16:
         renderTo16Bits(output + offset, bitmap);
         break;
   }
   offset += getRowBytes(bitmap.width(), bitsPerPixel) * bitmap.height();

   //must be 32 bit aligned to chain bitmaps
   if(!lastBitmap && offset & 3)
      offset = (offset & ~3) + 4;

   return offset;
}

QImage renderSvgToQimage(const QString& path, int16_t width, int16_t height){
   QSvgRenderer svgRenderer(path);
   QImage canvas(width, height, QImage::Format_RGB32);
   QPainter painter(&canvas);

   //clear buffer to white and render
   canvas.fill(QColor(0xFF, 0xFF, 0xFF).rgb());
   svgRenderer.render(&painter);

   return canvas;
}

QImage getIconImage(const QString& path, int16_t width, int16_t height, uint8_t bpp){
   //$DIR/icon$WIDTHx$HEIGHTx$BPP.png", doesnt actually have to be a png, just has to have that extension
   return QImage(path + "/icon" + QString::number(width) + "x" + QString::number(height) + "x" + QString::number(bpp) + ".png");
}

void convertToPalmIcons(const QString& path, const QString& outputDirectory){
   QFileInfo imagePath(path);
   QFile taib03E8File(outputDirectory + "/tAIB03E8.bin");
   QFile taib03E9File(outputDirectory + "/tAIB03E9.bin");
   uint8_t* taib03E8 = new uint8_t[MAX_PALM_BITMAP_SIZE * 5];
   uint8_t* taib03E9 = new uint8_t[MAX_PALM_BITMAP_SIZE * 5];
   uint32_t taib03E8Offset = 0;
   uint32_t taib03E9Offset = 0;

   //render folder of icons or .svg file to all sizes of Palm OS icon with the proper names for prc-tools
   if(imagePath.isDir()){
      taib03E8Offset += renderPalmBitmap(getIconImage(path, 22, 22, 1), taib03E8 + taib03E8Offset, 1, false);//1bpp, greyscale
      taib03E8Offset += renderPalmBitmap(getIconImage(path, 22, 22, 2), taib03E8 + taib03E8Offset, 2, false);//2bpp, greyscale
      taib03E8Offset += renderPalmBitmap(getIconImage(path, 22, 22, 4), taib03E8 + taib03E8Offset, 4, false);//4bpp, greyscale
      taib03E8Offset += renderPalmBitmap(getIconImage(path, 22, 22, 8), taib03E8 + taib03E8Offset, 8, false);//8bpp, color
      taib03E8Offset += renderPalmBitmap(getIconImage(path, 22, 22, 16), taib03E8 + taib03E8Offset, 16, true);//16bpp, color

      taib03E9Offset += renderPalmBitmap(getIconImage(path, 15, 9, 1), taib03E9 + taib03E9Offset, 1, false);//1bpp, greyscale
      taib03E9Offset += renderPalmBitmap(getIconImage(path, 15, 9, 2), taib03E9 + taib03E9Offset, 2, false);//2bpp, greyscale
      taib03E9Offset += renderPalmBitmap(getIconImage(path, 15, 9, 4), taib03E9 + taib03E9Offset, 4, false);//4bpp, greyscale
      taib03E9Offset += renderPalmBitmap(getIconImage(path, 15, 9, 8), taib03E9 + taib03E9Offset, 8, false);//8bpp, color
      taib03E9Offset += renderPalmBitmap(getIconImage(path, 15, 9, 16), taib03E9 + taib03E9Offset, 16, true);//16bpp, color
   }
   else if(imagePath.isFile() && imagePath.suffix() == "svg"){
      QImage svgOutputBig = renderSvgToQimage(path, 22, 22);
      QImage svgOutputLittle = renderSvgToQimage(path, 15, 9);

      taib03E8Offset += renderPalmBitmap(svgOutputBig, taib03E8 + taib03E8Offset, 1, false);//1bpp, greyscale
      taib03E8Offset += renderPalmBitmap(svgOutputBig, taib03E8 + taib03E8Offset, 2, false);//2bpp, greyscale
      taib03E8Offset += renderPalmBitmap(svgOutputBig, taib03E8 + taib03E8Offset, 4, false);//4bpp, greyscale
      taib03E8Offset += renderPalmBitmap(svgOutputBig, taib03E8 + taib03E8Offset, 8, false);//8bpp, color
      taib03E8Offset += renderPalmBitmap(svgOutputBig, taib03E8 + taib03E8Offset, 16, true);//16bpp, color

      taib03E9Offset += renderPalmBitmap(svgOutputLittle, taib03E9 + taib03E9Offset, 1, false);//1bpp, greyscale
      taib03E9Offset += renderPalmBitmap(svgOutputLittle, taib03E9 + taib03E9Offset, 2, false);//2bpp, greyscale
      taib03E9Offset += renderPalmBitmap(svgOutputLittle, taib03E9 + taib03E9Offset, 4, false);//4bpp, greyscale
      taib03E9Offset += renderPalmBitmap(svgOutputLittle, taib03E9 + taib03E9Offset, 8, false);//8bpp, color
      taib03E9Offset += renderPalmBitmap(svgOutputLittle, taib03E9 + taib03E9Offset, 16, true);//16bpp, color
   }

   if(taib03E8File.open(QFile::WriteOnly | QFile::Truncate)){
      taib03E8File.write((const char*)taib03E8, taib03E8Offset);
      taib03E8File.close();
   }
   if(taib03E9File.open(QFile::WriteOnly | QFile::Truncate)){
      taib03E9File.write((const char*)taib03E9, taib03E9Offset);
      taib03E9File.close();
   }

   delete[] taib03E8;
   delete[] taib03E9;
}

void convertToPalmBitmap(const QString& imagePath, const QString& outputPath){
   QImage image(imagePath);
   QFile outputFile(outputPath);
   uint8_t* outputBuffer = new uint8_t[MAX_PALM_BITMAP_SIZE * 5];
   uint32_t outputOffset = 0;

   outputOffset += renderPalmBitmap(image, outputBuffer + outputOffset, 1, false);//1bpp, greyscale
   outputOffset += renderPalmBitmap(image, outputBuffer + outputOffset, 2, false);//2bpp, greyscale
   outputOffset += renderPalmBitmap(image, outputBuffer + outputOffset, 4, false);//4bpp, greyscale
   outputOffset += renderPalmBitmap(image, outputBuffer + outputOffset, 8, false);//8bpp, color
   outputOffset += renderPalmBitmap(image, outputBuffer + outputOffset, 16, true);//16bpp, color

   if(outputFile.open(QFile::WriteOnly | QFile::Truncate)){
      outputFile.write((const char*)outputBuffer, outputOffset);
      outputFile.close();
   }

   delete[] outputBuffer;
}

int main(int argc, char* argv[]){
   if(argc == 4){
      QString command = argv[1];

      if(command == "icon")
         convertToPalmIcons(QString(argv[2]), QString(argv[3]));
      else if(command == "bitmap")
         convertToPalmBitmap(QString(argv[2]), QString(argv[3]));
      else
         printf("\"%s\" is not a valid command.\n", command.toStdString().c_str());
   }
   else{
      //invalid parameters
      printf("MakePalmBitmap v1.1\n");
      printf("A replacement for the pilrc bitmap converter, which seems to be broken on 64 bit systems.\n");
      printf("Args:icon \"/path/to/image.svg\" \"/path/to/palm/application/directory\"\n");
      printf("OR\n");
      printf("Args:icon \"/path/to/image/dir\" \"/path/to/palm/application/directory\"\n");
      printf("OR\n");
      printf("Args:bitmap \"/path/to/image.png\" \"/path/to/palm/application/directory/Tbmp****.bin\"\n");
      printf("Fleas have pet rabbits!\n");
   }

   return 0;
}
