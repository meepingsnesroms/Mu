#include <stdint.h>

#include "touchscreen.h"
#include "mainwindow.h"
#include "emuwrapper.h"


TouchScreen::TouchScreen(QWidget* parent)
    : QLabel(parent){

}

TouchScreen::~TouchScreen(){

}

double TouchScreen::rangeSwap(double newRange, double oldRange, double value){
   return value / oldRange * newRange;
}

void TouchScreen::mousePressEvent(QMouseEvent* ev){
   if(ev->x() >= 0 && ev->x() < this->width() && ev->y() >= 0 && ev->y() < this->height()){
      EmuWrapper& emu = ((MainWindow*)(parentWidget()->parentWidget()->parentWidget()))->emu;

      emu.emuInput.touchscreenX = (uint16_t)rangeSwap(emu.screenWidth(), this->width(), ev->x());
      emu.emuInput.touchscreenY = (uint16_t)rangeSwap(emu.screenHeight(), this->height(), ev->y());
      emu.emuInput.touchscreenTouched = true;
   }
}

void TouchScreen::mouseMoveEvent(QMouseEvent* ev){
   if(ev->x() >= 0 && ev->x() < this->width() && ev->y() >= 0 && ev->y() < this->height()){
      EmuWrapper& emu = ((MainWindow*)(parentWidget()->parentWidget()->parentWidget()))->emu;

      emu.emuInput.touchscreenX = (uint16_t)rangeSwap(emu.screenWidth(), this->width(), ev->x());
      emu.emuInput.touchscreenY = (uint16_t)rangeSwap(emu.screenHeight(), this->height(), ev->y());
   }
}

void TouchScreen::mouseReleaseEvent(QMouseEvent* ev){
   EmuWrapper& emu = ((MainWindow*)(parentWidget()->parentWidget()->parentWidget()))->emu;

   emu.emuInput.touchscreenTouched = false;
}
