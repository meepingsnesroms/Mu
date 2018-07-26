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
   //I got lucky RGB16(bbbbbggggggrrrrr) actually has the low end as red and the high end as blue, automatic thermal coloration for free
   int interlacedSegments = ui->interlacedSegments->value();
   int activeSegment = ui->segment->value();
   int width = ui->width->value();
   int height = fileData.length() / 2 / interlacedSegments / width;
   int imageIndex = activeSegment;
   uint16_t* oldImage = imageData;

   imageData = new uint16_t[width * height];

   for(int y = 0; y < height; y++){
      for(int x = 0; x < width; x++){
         //deinterlace the data
         imageData[x + y * width] = fileData[imageIndex * 2] << 8 | fileData[imageIndex * 2 + 1];
         imageIndex += interlacedSegments;
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
