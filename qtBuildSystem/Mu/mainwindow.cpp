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
#include <QObject>
#include <QEvent>
#include <QKeyEvent>
#include <QGraphicsScene>
#include <QPixmap>
#include <QAudioOutput>
#include <QAudioFormat>

#include <stdint.h>

#include "debugviewer.h"
#include "statemanager.h"


/*
#if defined(Q_OS_ANDROID)
#include <android/log.h>
#endif

void printLastRun(){
   static int64_t lastTime = 0;
   struct timespec tms;

   clock_gettime(CLOCK_REALTIME, &tms);

   int64_t micros = tms.tv_sec * 1000000;
   micros += tms.tv_nsec / 1000;
   if(tms.tv_nsec % 1000 >= 500)
       micros++;

#if defined(Q_OS_ANDROID)
   __android_log_print(ANDROID_LOG_DEBUG, "Mu", "Microseconds:%lld\n", micros - lastTime);
#else
   printf("Microseconds:%lld\n", micros - lastTime);
#endif

   lastTime = micros;
}
*/


MainWindow::MainWindow(QWidget* parent) :
   QMainWindow(parent),
   ui(new Ui::MainWindow){
   QAudioFormat format;
   QString resourceDirPath;

   //audio output
   format.setSampleRate(AUDIO_SAMPLE_RATE);
   format.setChannelCount(2);
   format.setSampleSize(16);
   format.setCodec("audio/pcm");
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
   format.setByteOrder(QAudioFormat::BigEndian);
#else
   format.setByteOrder(QAudioFormat::LittleEndian);
#endif
   format.setSampleType(QAudioFormat::SignedInt);

   //submodules
   settings = new QSettings(QDir::homePath() + "/MuCfg.txt", QSettings::IniFormat);//settings is public, create it first
   stateManager = new StateManager(this);
   emuDebugger = new DebugViewer(this);
   refreshDisplay = new QTimer(this);
   audioDevice = new QAudioOutput(format, this);
   audioOut = audioDevice->start();

   //resource directory
   resourceDirPath = settings->value("resourceDirectory", "").toString();

   //get default path if path not set
   if(resourceDirPath == ""){
#if defined(Q_OS_ANDROID)
      resourceDirPath = "/sdcard/Mu";
#elif defined(Q_OS_IOS)
      resourceDirPath = "/var/mobile/Media/Mu";
#else
      resourceDirPath = QDir::homePath() + "/Mu";
#endif
      settings->setValue("resourceDirectory", resourceDirPath);
   }

   //create directory tree, in case someone deleted it since the emu was last run or it was never created
   createHomeDirectoryTree(resourceDirPath);

   //GUI
   ui->setupUi(this);

   //this makes the display window and button icons resize properly
   ui->centralWidget->installEventFilter(this);
   ui->centralWidget->setObjectName("centralWidget");

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

   ui->ctrlBtn->installEventFilter(this);
   ui->install->installEventFilter(this);
   ui->debugger->installEventFilter(this);
   ui->screenshot->installEventFilter(this);
   ui->stateManager->installEventFilter(this);

#if !defined(EMU_DEBUG) || defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
   ui->debugger->hide();
#endif
   connect(refreshDisplay, SIGNAL(timeout()), this, SLOT(updateDisplay()));
   refreshDisplay->start(1000 / EMU_FPS);//update display every X milliseconds
}

MainWindow::~MainWindow(){
   refreshDisplay->stop();
   delete stateManager;
   delete emuDebugger;
   delete refreshDisplay;
   delete audioDevice;
   delete settings;//settings is public, destroy it last
   delete ui;
}

bool MainWindow::eventFilter(QObject* object, QEvent* event){
   if(event->type() == QEvent::Resize){
      if(QString(object->metaObject()->className()) == "QPushButton"){
         QPushButton* button = (QPushButton*)object;

         button->setIconSize(QSize(button->size().width() / 1.7, button->size().height() / 1.7));
      }

      if(object->objectName() == "centralWidget"){
         float smallestRatio;

         //update displayContainer first, make the display occupy the top 2/3 of the screen
         ui->displayContainer->setFixedHeight(ui->centralWidget->height() * 0.66);

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

void MainWindow::createHomeDirectoryTree(const QString& path){
   QDir homeDir(path);

   //creates directorys if not present, does nothing if they exist already
   homeDir.mkpath(".");
   homeDir.mkpath("./saveStates");
   homeDir.mkpath("./screenshots");
   homeDir.mkpath("./debugDumps");
}

void MainWindow::popupErrorDialog(const QString& error){
   QMessageBox::critical(this, "Mu", error, QMessageBox::Ok);
}

void MainWindow::popupInformationDialog(const QString& info){
   QMessageBox::information(this, "Mu", info, QMessageBox::Ok);
}

void MainWindow::selectHomePath(){
   QString homeDirPath = QFileDialog::getOpenFileName(this, "New Home Directory(\"~/Mu\" is default)", QDir::root().path(), nullptr);

   createHomeDirectoryTree(homeDirPath);
   settings->setValue("resourceDirectory", homeDirPath);
}

//display
void MainWindow::updateDisplay(){
   if(emu.newFrameReady()){
      //video, this is doing bilinear filitering in software, this is why the Qt port is broken on Android, move this to a new thread if possible
      ui->display->setPixmap(emu.getFramebuffer().scaled(ui->display->size().width(), ui->display->size().height(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

      //audio
      audioOut->write((const char*)emu.getAudioSamples(), AUDIO_SAMPLES_PER_FRAME * 2/*channels*/ * sizeof(int16_t));

      //power LED
      ui->powerButtonLed->setStyleSheet(emu.getPowerButtonLed() ? "background: lime" : "");

      //allow next frame to start
      emu.frameHandled();

      //update GUI
      ui->display->repaint();
      ui->powerButtonLed->repaint();
   }

   //printLastRun();
}

//Palm buttons
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
      QString sysDir = settings->value("resourceDirectory", "").toString();
      uint32_t error = emu.init(sysDir + "/palmos41-en-m515.rom", QFile(sysDir + "/bootloader-en-m515.rom").exists() ? sysDir + "/bootloader-en-m515.rom" : "", sysDir + "/userdata-en-m515.ram", sysDir + "/sd-en-m515.img", FEATURE_CUSTOM_FB | FEATURE_DEBUG);
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

         ui->ctrlBtn->setIcon(QIcon(":/buttons/images/pause.svg"));
      }
      else{
         popupErrorDialog("Emu error:" + QString::number(error) + ", cant run!");
      }
   }
   else if(emu.isRunning()){
      emu.pause();
      ui->ctrlBtn->setIcon(QIcon(":/buttons/images/play.svg"));
   }
   else if(emu.isPaused()){
      emu.resume();
      ui->ctrlBtn->setIcon(QIcon(":/buttons/images/pause.svg"));
   }

   ui->ctrlBtn->repaint();
}

void MainWindow::on_install_clicked(){
   if(emu.isInited()){
      QString app = QFileDialog::getOpenFileName(this, "Open *.prc/pdb/pqa", QDir::root().path(), nullptr);

      if(app != ""){
         uint32_t error = emu.installApplication(app);

         if(error != EMU_ERROR_NONE)
            popupErrorDialog("Could not install app, Error:" + QString::number(error));
      }
   }
}

void MainWindow::on_debugger_clicked(){
   if(emu.isInited()){
      emu.pause();
      ui->ctrlBtn->setIcon(QIcon(":/buttons/images/play.svg"));
      ui->ctrlBtn->repaint();
      emuDebugger->exec();
   }
}

void MainWindow::on_screenshot_clicked(){
   if(emu.isInited()){
      qlonglong screenshotNumber = settings->value("screenshotNum", 0).toLongLong();
      QString screenshotPath = settings->value("resourceDirectory", "").toString() + "/screenshots/screenshot" + QString::number(screenshotNumber, 10) + ".png";

      emu.getFramebuffer().save(screenshotPath, "PNG", 100);
      screenshotNumber++;
      settings->setValue("screenshotNum", screenshotNumber);
   }
}

void MainWindow::on_stateManager_clicked(){
   if(emu.isInited()){
      bool wasPaused = emu.isPaused();

      if(!wasPaused)
         emu.pause();

      stateManager->exec();

      if(!wasPaused)
         emu.resume();
   }
}
