#pragma once

#include <QMainWindow>
#include <QSettings>
#include <stdint.h>

#include "src/emulator.h"

extern uint32_t  screenWidth;
extern uint32_t  screenHeight;
extern input_t   frontendInput;
extern QSettings settings;

#if defined(EMU_DEBUG) && defined(EMU_CUSTOM_DEBUG_LOG_HANDLER)
#include <vector>
#include <string>


extern std::vector<std::string> debugStrings;
extern std::vector<uint64_t>    duplicateCallCount;
#endif

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
   Q_OBJECT

public:
   explicit MainWindow(QWidget *parent = 0);
   ~MainWindow();

private slots:
   void popupErrorDialog(QString error);
   void popupInformationDialog(QString info);
   bool eventFilter(QObject *object, QEvent *event);
   void selectHomePath();
   void on_install_pressed();

   //display
   void updateDisplay();

   //palm buttons
   void on_power_pressed();
   void on_power_released();
   void on_calender_pressed();
   void on_calender_released();
   void on_addressBook_pressed();
   void on_addressBook_released();
   void on_todo_pressed();
   void on_todo_released();
   void on_notes_pressed();
   void on_notes_released();

   void on_ctrlBtn_clicked();

   void on_debugger_clicked();

   void on_screenshot_clicked();

private:
   Ui::MainWindow *ui;
};
