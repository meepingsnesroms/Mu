#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QIcon>
#include <QString>
#include <QObject>
#include <QEvent>
#include <QSettings>
#include <QAudioOutput>
#include <QIODevice>
#include <QKeyEvent>

#include "emuwrapper.h"
#include "settingsmanager.h"
#include "statemanager.h"
#include "debugviewer.h"

namespace Ui{
class MainWindow;
}

class MainWindow : public QMainWindow{
   Q_OBJECT

public:
   EmuWrapper emu;
   QSettings* settings;

   explicit MainWindow(QWidget* parent = nullptr);
   ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);

public:
   void createHomeDirectoryTree(const QString& path);

private:
   uint32_t getEmuFeatureList();
   void popupErrorDialog(const QString& error);
   void popupInformationDialog(const QString& info);
   void redraw();

private slots:
   bool eventFilter(QObject* object, QEvent* event);

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

   //frontend buttons
   void on_ctrlBtn_clicked();
   void on_install_clicked();
   void on_debugger_clicked();
   void on_screenshot_clicked();
   void on_stateManager_clicked();
   void on_reset_clicked();
   void on_settings_clicked();

private:
   SettingsManager* settingsManager;
   StateManager*    stateManager;
   DebugViewer*     emuDebugger;
   QTimer*          refreshDisplay;
   QAudioOutput*    audioDevice;
   QIODevice*       audioOut;
   Ui::MainWindow*  ui;
   int              keyForButton[EmuWrapper::BUTTON_TOTAL_COUNT];
};
