#include "touchscreen.h"
#include "mainwindow.h"
#include "emulator.h"


float rangeSwap(float r1Start, float r1End, float r2Start, float r2End, float r2Val){
	float r1Size = r1End - r1Start;
	float r2Size = r2End - r2Start;

	float XTo1 = r2Size / r1Size;

	return r2Val * XTo1 + r2Start;
}

TouchScreen::TouchScreen(QWidget* parent)
    : QLabel(parent){

}

TouchScreen::~TouchScreen(){

}

void TouchScreen::mousePressEvent(QMouseEvent *ev){
	int x = (int)rangeSwap(0,this->width(),0,settings.screen_width,ev->x());
	int y = (int)rangeSwap(0,this->height(),0,settings.screen_height,ev->y());
	emu_sendtouch(x,y,true);
}
void TouchScreen::mouseMoveEvent(QMouseEvent *ev){
	int x = (int)rangeSwap(0,this->width(),0,settings.screen_width,ev->x());
	int y = (int)rangeSwap(0,this->height(),0,settings.screen_height,ev->y());
	emu_sendtouch(x,y,true);
}

void TouchScreen::mouseReleaseEvent(QMouseEvent *ev){
	int x = (int)rangeSwap(0,this->width(),0,settings.screen_width,ev->x());
	int y = (int)rangeSwap(0,this->height(),0,settings.screen_height,ev->y());
	emu_sendtouch(x,y,false);
}

/*
void TouchScreen::touchPressEvent(QTouchEvent *ev){

}
*/
