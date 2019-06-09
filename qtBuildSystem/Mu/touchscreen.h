#pragma once

#include <QWidget>
#include <QPaintEvent>
#include <QMouseEvent>

class TouchScreen : public QWidget{
   Q_OBJECT
   
private:
   float rangeSwap(float newRange, float oldRange, float value);

public:
   explicit TouchScreen(QWidget* parent = nullptr);
   ~TouchScreen();
   
protected:
   void paintEvent(QPaintEvent* ev);
   void mousePressEvent(QMouseEvent* ev);
   void mouseMoveEvent(QMouseEvent* ev);
   void mouseReleaseEvent(QMouseEvent* ev);
};
