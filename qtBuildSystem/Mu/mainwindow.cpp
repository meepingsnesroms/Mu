#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QFileDialog>
#include <QTimer>
#include <QTouchEvent>
#include <QMessageBox>
#include <QSettings>
#include <QFont>
#include <QIcon>
#include <QKeyEvent>
#include <QGraphicsScene>
#include <QPixmap>

#include <stdint.h>

#include "debugviewer.h"


MainWindow::MainWindow(QWidget* parent) :
   QMainWindow(parent),
   ui(new Ui::MainWindow){
   ui->setupUi(this);

   stateManager = new StateManager(this);
   emuDebugger = new DebugViewer(this);
   refreshDisplay = new QTimer(this);

   //this makes the display window and button icons resize properly
   ui->centralWidget->installEventFilter(this);
   ui->centralWidget->setObjectName("centralWidget");
   //ui->displayContainer->installEventFilter(this);
   //ui->displayContainer->setObjectName("displayContainer");

   ui->calendar->installEventFilter(this);
   ui->addressBook->installEventFilter(this);
   ui->todo->installEventFilter(this);
   ui->notes->installEventFilter(this);

   ui->up->installEventFilter(this);
   ui->down->installEventFilter(this);
   ui->left->installEventFilter(this);
   ui->right->installEventFilter(this);
   ui->center->installEventFilter(this);

   ui->power->installEventFilter(this);

   ui->screenshot->installEventFilter(this);
   //ui->install->installEventFilter(this);//enable when install gets an icon
   //ui->stateManager->installEventFilter(this);//enable when stateManager gets an icon
   ui->debugger->installEventFilter(this);

   ui->ctrlBtn->installEventFilter(this);

   //ui->ctrlBtn->setIcon(QIcon(":/buttons/images/play.png"));


   QString resourceDirPath = settings.value("resourceDirectory", "").toString();

   //use default path if path not set
   if(resourceDirPath == ""){
#if defined(Q_OS_ANDROID)
      resourceDirPath = "/sdcard/Mu";
#elif defined(Q_OS_IOS)
      resourceDirPath = "/var/mobile/Media/Mu";
#else
      resourceDirPath = QDir::homePath() + "/Mu";
#endif
      settings.setValue("resourceDirectory", resourceDirPath);
   }

   //create directory tree, in case someone deleted it since the emu was last run
   createHomeDirectoryTree(resourceDirPath);


#if !defined(EMU_DEBUG) || defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
   ui->debugger->hide();
#endif

   connect(refreshDisplay, SIGNAL(timeout()), this, SLOT(updateDisplay()));
   refreshDisplay->start(16);//update display every 16.67miliseconds = 60 * second
}

MainWindow::~MainWindow(){
   delete ui;
}

void MainWindow::createHomeDirectoryTree(QString path){
   QDir homeDir(path);

   //creates directorys if not present, does nothing if they exist already
   homeDir.mkpath(".");
   homeDir.mkpath("./saveStates");
   homeDir.mkpath("./screenshots");
   homeDir.mkpath("./debugDumps");
}

void MainWindow::popupErrorDialog(QString error){
   QMessageBox::critical(this, "Mu", error, QMessageBox::Ok);
}

void MainWindow::popupInformationDialog(QString info){
   QMessageBox::information(this, "Mu", info, QMessageBox::Ok);
}

bool MainWindow::eventFilter(QObject *object, QEvent *event){
   if(event->type() == QEvent::Resize){
      if(QString(object->metaObject()->className()) == "QPushButton"){
         QPushButton* button = (QPushButton*)object;

         button->setIconSize(QSize(button->size().width() / 1.7, button->size().height() / 1.7));
      }

      if(object->objectName() == "centralWidget"){
         double smallestRatio;

         //update displayContainer first, make the display occupy the top 3/4 of the screen
         ui->displayContainer->setFixedHeight(ui->centralWidget->height() * 0.75);

         smallestRatio = qMin(ui->displayContainer->size().width() * 0.98 / 3.0 , ui->displayContainer->size().height() * 0.98 / 4.0);
         //the 0.98 above allows the display to shrink, without it the displayContainer couldent shrink because of the fixed size of the display

         //set new size
         ui->display->setFixedSize(smallestRatio * 3.0, smallestRatio * 4.0);

         //scale framebuffer to new size and refresh
         if(emu.isInited()){
            ui->display->setPixmap(emu.getFramebuffer().scaled(QSize(ui->display->size().width(), ui->display->size().height()), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            ui->display->update();
         }
      }
   }

   return QMainWindow::eventFilter(object, event);
}

void MainWindow::selectHomePath(){
   QString homeDirPath = QFileDialog::getOpenFileName(this, "New Home Directory(\"~/Mu\" is default)", QDir::root().path(), nullptr);

   createHomeDirectoryTree(homeDirPath);
   settings.setValue("resourceDirectory", homeDirPath);
}

void MainWindow::on_install_pressed(){
   QString app = QFileDialog::getOpenFileName(this, "Open *.prc/pdb/pqa", QDir::root().path(), nullptr);
   uint32_t error = emu.installApplication(app);

   if(error != EMU_ERROR_NONE)
      popupErrorDialog("Could not install app");
}

//display
void MainWindow::updateDisplay(){
   if(emu.newFrameReady()){
      ui->display->setPixmap(emu.getFramebuffer().scaled(ui->display->size().width(), ui->display->size().height(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
      ui->display->update();

      ui->powerButtonLed->setStyleSheet(emu.getPowerButtonLed() ? "background: lime" : "");
   }

   if(emu.debugEventOccured()){
      emu.clearDebugEvent();
      ui->ctrlBtn->setIcon(QIcon(":/buttons/images/play.png"));
      emuDebugger->exec();
   }
}

//palm buttons
void MainWindow::on_power_pressed(){
   emu.emuInput.buttonPower = true;
}

void MainWindow::on_power_released(){
   emu.emuInput.buttonPower = false;
}

void MainWindow::on_calendar_pressed(){
   emu.emuInput.buttonCalendar = true;
}

void MainWindow::on_calendar_released(){
   emu.emuInput.buttonCalendar = false;
}

void MainWindow::on_addressBook_pressed(){
   emu.emuInput.buttonAddress = true;
}

void MainWindow::on_addressBook_released(){
   emu.emuInput.buttonAddress = false;
}

void MainWindow::on_todo_pressed(){
   emu.emuInput.buttonTodo = true;
}

void MainWindow::on_todo_released(){
   emu.emuInput.buttonTodo = false;
}

void MainWindow::on_notes_pressed(){
   emu.emuInput.buttonNotes = true;
}

void MainWindow::on_notes_released(){
   emu.emuInput.buttonNotes = false;
}

void MainWindow::on_up_pressed(){
    emu.emuInput.buttonUp = true;
}

void MainWindow::on_up_released(){
    emu.emuInput.buttonUp = false;
}

void MainWindow::on_down_pressed(){
    emu.emuInput.buttonDown = true;
}

void MainWindow::on_down_released(){
    emu.emuInput.buttonDown = false;
}

void MainWindow::on_left_pressed(){
    emu.emuInput.buttonLeft = true;
}

void MainWindow::on_left_released(){
    emu.emuInput.buttonLeft = false;
}

void MainWindow::on_right_pressed(){
    emu.emuInput.buttonRight = true;
}

void MainWindow::on_right_released(){
    emu.emuInput.buttonRight = false;
}

//emu control
void MainWindow::on_ctrlBtn_clicked(){
   if(!emu.isInited()){
      QString systemDirectory = settings.value("resourceDirectory", "").toString();
      uint32_t error = emu.init(systemDirectory + "/palmos41-en-m515.rom", systemDirectory + "/bootloader-en-m515.rom", systemDirectory + "/userdata-en-m515.ram", FEATURE_DEBUG);
      if(error == EMU_ERROR_NONE){
         ui->calendar->setEnabled(true);
         ui->addressBook->setEnabled(true);
         ui->todo->setEnabled(true);
         ui->notes->setEnabled(true);

         ui->up->setEnabled(true);
         ui->down->setEnabled(true);

         ui->power->setEnabled(true);

         /*
         //if FEATURE_EXT_KEYS enabled add OS 5 buttons
         ui->left->setEnabled(true);
         ui->right->setEnabled(true);
         ui->center->setEnabled(true);
         */

         ui->screenshot->setEnabled(true);
         ui->install->setEnabled(true);
         ui->stateManager->setEnabled(true);
         ui->debugger->setEnabled(true);

         ui->ctrlBtn->setIcon(QIcon(":/buttons/images/pause.png"));
      }
      else{
         popupErrorDialog("Emu error:" + QString::number(error) + ", cant run!");
      }
   }
   else if(emu.isRunning()){
      emu.pause();
      ui->ctrlBtn->setIcon(QIcon(":/buttons/images/play.png"));
   }
   else if(emu.isPaused()){
      emu.resume();
      ui->ctrlBtn->setIcon(QIcon(":/buttons/images/pause.png"));
   }

   ui->ctrlBtn->repaint();//Qt 5.11 broke icon changes on click, this is a patch
}

void MainWindow::on_debugger_clicked(){
   if(emu.isInited()){
      emu.pause();
      ui->ctrlBtn->setIcon(QIcon(":/buttons/images/play.png"));
      ui->ctrlBtn->repaint();//Qt 5.11 broke icon changes on click, this is a patch
      emuDebugger->exec();
   }
}

void MainWindow::on_screenshot_clicked(){
   if(emu.isInited()){
      qlonglong screenshotNumber = settings.value("screenshotNum", 0).toLongLong();
      QString screenshotPath = settings.value("resourceDirectory", "").toString() + "/screenshots/screenshot" + QString::number(screenshotNumber, 10) + ".png";

      emu.getFramebuffer().save(screenshotPath, "PNG", 100);
      screenshotNumber++;
      settings.setValue("screenshotNum", screenshotNumber);
   }
}

void MainWindow::on_stateManager_clicked(){
   if(emu.isInited()){
      bool wasPaused = emu.isPaused();

      if(!wasPaused)
         emu.pause();

      stateManager->updateStateList();
      stateManager->exec();

      if(!wasPaused)
         emu.resume();
   }
}
