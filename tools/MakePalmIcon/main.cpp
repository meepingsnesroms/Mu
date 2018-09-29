#include <QCoreApplication>
#include <QSvgRenderer>
#include <QPainter>
#include <QImage>
#include <QString>

#include <stdio.h>


void renderPalmIcon(const QString& svg, const QString& filePath, int size, int bitsPerPixel, bool color){
   QImage canvas(size, size, QImage::Format_RGB16);
   QPainter painter(&canvas);
   QSvgRenderer svgRenderer(svg);

   svgRenderer.render(&painter);

}

void convertToPalmIcons(const QString& svg, const QString& outputDirectory){
   renderPalmIcon(svg, outputDirectory + "tAIN.bin", 16, 1, false);//1bpp, greyscale
   renderPalmIcon(svg, outputDirectory + "tAIN.bin", 16, 1, false);//2bpp, greyscale
   renderPalmIcon(svg, outputDirectory + "tAIN.bin", 16, 1, false);//4bpp, greyscale
   renderPalmIcon(svg, outputDirectory + "tAIN.bin", 16, 1, false);//8bpp, greyscale
   renderPalmIcon(svg, outputDirectory + "tAIN.bin", 16, 1, true);//8bpp, color
   renderPalmIcon(svg, outputDirectory + "tAIN.bin", 16, 1, true);//16bpp, color
}

int main(int argc, char* argv[]){
   //QCoreApplication a(argc, argv);
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

   //return a.exec();
   return 0;
}
