#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QString>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QByteArray>
#include <QPixmap>
#include <QImage>

#include <stdint.h>


uint16_t getEmWaveHeatMapColor(uint16_t data){
   //stolen from http://www.andrewnoske.com/wiki/Code_-_heatmaps_and_color_gradients
   const int NUM_COLORS = 8;
   const float color[NUM_COLORS][3] = {{0,0,0}, {1,0,0}, {1,1,0}, {0,1,0}, {0,1,1}, {0,0,1}, {0,0,1}, {1,1,1}};//{r,g,b}
   float value = (float)data / 0xFFFF;
   float red, green, blue;
   int idx1;        // |-- Our desired color will be between these two indexes in "color".
   int idx2;        // |
   float fractBetween = 0;  // Fraction between "idx1" and "idx2" where our value is.

   if(value <= 0){
      // accounts for an input <=0
      idx1 = idx2 = 0;
   }
   else if(value >= 1){
      // accounts for an input >=0
      idx1 = idx2 = NUM_COLORS-1;
   }
   else{
      value = value * (NUM_COLORS - 1);    // Will multiply value by NUM_COLORS.
      idx1  = (int)value;                // Our desired color will be after this index.
      idx2  = idx1 + 1;                  // ... and before this index (inclusive).
      fractBetween = value - (float)idx1;// Distance between the two indexes (0-1).
   }

   red   = (color[idx2][0] - color[idx1][0]) * fractBetween + color[idx1][0];
   green = (color[idx2][1] - color[idx1][1]) * fractBetween + color[idx1][1];
   blue  = (color[idx2][2] - color[idx1][2]) * fractBetween + color[idx1][2];

   return (uint16_t)(red * 0x001F) << 11 | (uint16_t)(green * 0x003F) << 5 | (uint16_t)(blue * 0x001F);
}


MainWindow::MainWindow(QWidget* parent) :
   QMainWindow(parent),
   ui(new Ui::MainWindow){
   ui->setupUi(this);

   imageData = nullptr;
}

MainWindow::~MainWindow(){
   if(imageData)
      delete[] imageData;
   delete ui;
}

void MainWindow::refreshOnscreenData(){
   int interlacedSegments = ui->interlacedSegments->value();
   int activeSegment = ui->segment->value();
   int width = ui->width->value();
   int height = fileData.length() / sizeof(uint16_t) / interlacedSegments / width;
   int imageIndex = activeSegment * sizeof(uint16_t);
   uint16_t* oldImage = imageData;

   imageData = new uint16_t[width * height];

   for(int y = 0; y < height; y++){
      for(int x = 0; x < width; x++){
         //deinterlace the data
         imageData[x + y * width] = getEmWaveHeatMapColor((uint8_t)fileData[imageIndex] << 8 | (uint8_t)fileData[imageIndex + 1]);
         imageIndex += interlacedSegments * sizeof(uint16_t);
      }
   }

   //swap out image buffer and delete the old one, the image buffer must remain persistant while the corrisponding QPixmap is displayed
   ui->framebuffer->setPixmap(QPixmap::fromImage(QImage((uchar*)imageData, width, height, width * sizeof(uint16_t), QImage::Format_RGB16)));
   delete[] oldImage;
}

void MainWindow::on_loadData_clicked(){
   QString dataPath = QFileDialog::getOpenFileName(this, "Open *.bin", QDir::root().path(), nullptr);
   QFile dataFile(dataPath);

   if(dataFile.open(QFile::ReadOnly)){
      fileData = dataFile.readAll();
      dataFile.close();
      refreshOnscreenData();
   }
}

void MainWindow::on_width_valueChanged(int arg1){
   refreshOnscreenData();
}

void MainWindow::on_interlacedSegments_valueChanged(int arg1){
   ui->segment->setMaximum(arg1 - 1);
   refreshOnscreenData();
}

void MainWindow::on_segment_valueChanged(int arg1){
   refreshOnscreenData();
}
