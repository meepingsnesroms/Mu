#pragma once

#include <QLabel>
#include <QMouseEvent>
#include <QTouchEvent>

class TouchScreen : public QLabel{
   Q_OBJECT
   
private:
   float rangeSwap(float newRange, float oldRange, float value);

public:
   explicit TouchScreen(QWidget* parent = nullptr);
   ~TouchScreen();
   
protected:
   void mousePressEvent(QMouseEvent* ev);
   void mouseMoveEvent(QMouseEvent* ev);
   void mouseReleaseEvent(QMouseEvent* ev);
};
