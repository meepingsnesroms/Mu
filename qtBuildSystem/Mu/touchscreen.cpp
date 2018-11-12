#include <QMouseEvent>
#include <QLabel>

#include <stdint.h>

#include "touchscreen.h"
#include "mainwindow.h"
#include "emuwrapper.h"


TouchScreen::TouchScreen(QWidget* parent)
    : QLabel(parent){

}

TouchScreen::~TouchScreen(){

}

float TouchScreen::rangeSwap(float newRange, float oldRange, float value){
   return value / oldRange * newRange;
}

void TouchScreen::mousePressEvent(QMouseEvent* ev){
   if(ev->x() >= 0 && ev->x() < this->width() && ev->y() >= 0 && ev->y() < this->height()){
      EmuWrapper& emu = ((MainWindow*)(parentWidget()->parentWidget()->parentWidget()))->emu;

      emu.emuInput.touchscreenX = (float)ev->x() / this->width();
      emu.emuInput.touchscreenY = (float)ev->y() / this->height();
      emu.emuInput.touchscreenTouched = true;
   }
}

void TouchScreen::mouseMoveEvent(QMouseEvent* ev){
   if(ev->x() >= 0 && ev->x() < this->width() && ev->y() >= 0 && ev->y() < this->height()){
      EmuWrapper& emu = ((MainWindow*)(parentWidget()->parentWidget()->parentWidget()))->emu;

      emu.emuInput.touchscreenX = (float)ev->x() / (this->width() - 1);
      emu.emuInput.touchscreenY = (float)ev->y() / (this->height() - 1);
   }
}

void TouchScreen::mouseReleaseEvent(QMouseEvent* ev){
   EmuWrapper& emu = ((MainWindow*)(parentWidget()->parentWidget()->parentWidget()))->emu;

   emu.emuInput.touchscreenTouched = false;
}
