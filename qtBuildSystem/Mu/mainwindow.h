#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QSettings>

#include "emuwrapper.h"
#include "debugviewer.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
   Q_OBJECT

public:
   EmuWrapper emu;
   QSettings  settings;

   explicit MainWindow(QWidget* parent = 0);
   ~MainWindow();

private slots:
   void popupErrorDialog(QString error);
   void popupInformationDialog(QString info);
   bool eventFilter(QObject* object, QEvent* event);
   void selectHomePath();
   void on_install_pressed();

   //display
   void updateDisplay();

   //palm buttons
   void on_power_pressed();
   void on_power_released();
   void on_calendar_pressed();
   void on_calendar_released();
   void on_addressBook_pressed();
   void on_addressBook_released();
   void on_todo_pressed();
   void on_todo_released();
   void on_notes_pressed();
   void on_notes_released();
   void on_up_pressed();
   void on_up_released();
   void on_down_pressed();
   void on_down_released();
   void on_left_pressed();
   void on_left_released();
   void on_right_pressed();
   void on_right_released();

   void on_ctrlBtn_clicked();

   void on_debugger_clicked();

   void on_screenshot_clicked();

private:
   DebugViewer*    emuDebugger;
   QTimer*         refreshDisplay;
   Ui::MainWindow* ui;
};
