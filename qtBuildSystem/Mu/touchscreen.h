#pragma once

#include <QLabel>
#include <QMouseEvent>
#include <QTouchEvent>

class TouchScreen : public QLabel{
   Q_OBJECT
   
private:
   double rangeSwap(double newRange, double oldRange, double value);

public:
   explicit TouchScreen(QWidget* parent = nullptr);
   ~TouchScreen();
   
protected:
   void mousePressEvent(QMouseEvent* ev);
   void mouseMoveEvent(QMouseEvent* ev);
   void mouseReleaseEvent(QMouseEvent* ev);
};
