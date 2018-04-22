#include "touchscreen.h"
#include "mainwindow.h"
#include "src/emulator.h"

float rangeSwap(float newRange, float oldRange, float value){
   return value / oldRange * newRange;
}

TouchScreen::TouchScreen(QWidget* parent)
    : QLabel(parent){

}

TouchScreen::~TouchScreen(){

}

void TouchScreen::mousePressEvent(QMouseEvent* ev){
   palmInput.touchscreenX = (uint16_t)rangeSwap(screenWidth, this->width(), ev->x());
   palmInput.touchscreenY = (uint16_t)rangeSwap(screenHeight, this->height(), ev->y());
	palmInput.touchscreenTouched = true;
}
void TouchScreen::mouseMoveEvent(QMouseEvent* ev){
   palmInput.touchscreenX = (uint16_t)rangeSwap(screenWidth, this->width(), ev->x());
   palmInput.touchscreenY = (uint16_t)rangeSwap(screenHeight, this->height(), ev->y());
}

void TouchScreen::mouseReleaseEvent(QMouseEvent* ev){
   palmInput.touchscreenTouched = false;
}
