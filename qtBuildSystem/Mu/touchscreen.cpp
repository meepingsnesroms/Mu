#include "touchscreen.h"

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>

#include <stdint.h>

#include "mainwindow.h"
#include "emuwrapper.h"


TouchScreen::TouchScreen(QWidget* parent)
    : QWidget(parent){

}

TouchScreen::~TouchScreen(){

}

float TouchScreen::rangeSwap(float newRange, float oldRange, float value){
   return value / oldRange * newRange;
}

void TouchScreen::paintEvent(QPaintEvent* ev){
   EmuWrapper& emu = ((MainWindow*)(parentWidget()->parentWidget()->parentWidget()))->emu;

   if(emu.isInited()){
      QPainter painter(this);
      QImage image = emu.getFramebufferImage().scaled(painter.window().size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

      painter.drawImage((painter.window().width() - image.width()) / 2, (painter.window().height() - image.height()) / 2, image);
   }
}

void TouchScreen::mousePressEvent(QMouseEvent* ev){
   if(ev->x() >= 0 && ev->x() < this->width() && ev->y() >= 0 && ev->y() < this->height()){
      EmuWrapper& emu = ((MainWindow*)(parentWidget()->parentWidget()->parentWidget()))->emu;

      emu.setPenValue((float)ev->x() / (this->width() - 1), (float)ev->y() / (this->height() - 1), true);
   }
}

void TouchScreen::mouseMoveEvent(QMouseEvent* ev){
   if(ev->x() >= 0 && ev->x() < this->width() && ev->y() >= 0 && ev->y() < this->height()){
      EmuWrapper& emu = ((MainWindow*)(parentWidget()->parentWidget()->parentWidget()))->emu;

      emu.setPenValue((float)ev->x() / (this->width() - 1), (float)ev->y() / (this->height() - 1), true);
   }
}

void TouchScreen::mouseReleaseEvent(QMouseEvent* ev){
   EmuWrapper& emu = ((MainWindow*)(parentWidget()->parentWidget()->parentWidget()))->emu;

   emu.setPenValue(0.0, 0.0, false);
}
