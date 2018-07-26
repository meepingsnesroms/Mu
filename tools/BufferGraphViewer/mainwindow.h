#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QByteArray>

#include <stdint.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow{
   Q_OBJECT

public:
   explicit MainWindow(QWidget* parent = nullptr);
   ~MainWindow();

private:
   void refreshOnscreenData();

private slots:
   void on_loadData_clicked();

   void on_width_valueChanged(int arg1);
   void on_interlacedSegments_valueChanged(int arg1);
   void on_segment_valueChanged(int arg1);

private:
   uint16_t*       imageData;
   QByteArray      fileData;
   Ui::MainWindow* ui;
};

#endif // MAINWINDOW_H
