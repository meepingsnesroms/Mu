#include <stdint.h>

#include "touchscreen.h"
#include "mainwindow.h"


static inline float rangeSwap(float newRange, float oldRange, float value){
   return value / oldRange * newRange;
}

TouchScreen::TouchScreen(QWidget* parent)
    : QLabel(parent){

}

TouchScreen::~TouchScreen(){

}

void TouchScreen::mousePressEvent(QMouseEvent* ev){
   frontendInput.touchscreenX = (uint16_t)rangeSwap(screenWidth, this->width(), ev->x());
   frontendInput.touchscreenY = (uint16_t)rangeSwap(screenHeight, this->height(), ev->y());
   frontendInput.touchscreenTouched = true;
}
void TouchScreen::mouseMoveEvent(QMouseEvent* ev){
   frontendInput.touchscreenX = (uint16_t)rangeSwap(screenWidth, this->width(), ev->x());
   frontendInput.touchscreenY = (uint16_t)rangeSwap(screenHeight, this->height(), ev->y());
}

void TouchScreen::mouseReleaseEvent(QMouseEvent* ev){
   frontendInput.touchscreenTouched = false;
}
