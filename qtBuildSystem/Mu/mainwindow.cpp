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

#include <chrono>
#include <thread>
#include <atomic>
#include <stdint.h>

#include "debugviewer.h"
#include "fileaccess.h"


MainWindow::MainWindow(QWidget* parent) :
   QMainWindow(parent),
   ui(new Ui::MainWindow){
   ui->setupUi(this);

   emuDebugger = new DebugViewer(this);
   refreshDisplay = new QTimer(this);

   ui->calender->installEventFilter(this);
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
   ui->ctrlBtn->installEventFilter(this);
   ui->debugger->installEventFilter(this);

   ui->ctrlBtn->setIcon(QIcon(":/buttons/images/play.png"));

#if defined(Q_OS_ANDROID)
   if(settings.value("resourceDirectory", "").toString() == "")
      settings.setValue("resourceDirectory", "/sdcard/Mu");
#elif defined(Q_OS_IOS)
   if(settings.value("resourceDirectory", "").toString() == "")
      settings.setValue("resourceDirectory", "/var/mobile/Media");
#else
   if(settings.value("resourceDirectory", "").toString() == "")
      settings.setValue("resourceDirectory", QDir::homePath() + "/Mu");
#endif

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
   ui->debugger->hide();
#endif

   connect(refreshDisplay, SIGNAL(timeout()), this, SLOT(updateDisplay()));
   refreshDisplay->start(16);//update display every 16.67miliseconds = 60 * second
}

MainWindow::~MainWindow(){
   delete ui;
}

void MainWindow::popupErrorDialog(QString error){
   QMessageBox::critical(this, "Mu", error, QMessageBox::Ok);
}

void MainWindow::popupInformationDialog(QString info){
   QMessageBox::information(this, "Mu", info, QMessageBox::Ok);
}

bool MainWindow::eventFilter(QObject *object, QEvent *event){
   if(QString(object->metaObject()->className()) == "QPushButton" && event->type() == QEvent::Resize){
      QPushButton* button = (QPushButton*)object;
      button->setIconSize(QSize(button->size().width() / 1.7, button->size().height() / 1.7));
   }

   return QMainWindow::eventFilter(object, event);
}

void MainWindow::selectHomePath(){
   QString dir = QFileDialog::getOpenFileName(this, "New Home Directory(\"~/Mu\" is default)", QDir::root().path(), 0);
   settings.setValue("resourceDirectory", dir);
}

void MainWindow::on_install_pressed(){
   QString app = QFileDialog::getOpenFileName(this, "Open *.prc/pdb/pqa", QDir::root().path(), 0);
   uint32_t error = emu.installApplication(app);
   if(error != FILE_ERR_NONE)
      popupErrorDialog("Could not install app");
}

//display
void MainWindow::updateDisplay(){
   if(emu.newFrameReady()){
      ui->display->setPixmap(emu.getFramebuffer().scaled(QSize(ui->display->size().width() * 0.95, ui->display->size().height() * 0.95), Qt::KeepAspectRatio, Qt::SmoothTransformation));
      ui->display->update();
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

void MainWindow::on_calender_pressed(){
   emu.emuInput.buttonCalender = true;
}

void MainWindow::on_calender_released(){
   emu.emuInput.buttonCalender = false;
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

//emu control
void MainWindow::on_ctrlBtn_clicked(){
   if(!emu.isInited()){
      uint32_t error = emu.init(settings.value("resourceDirectory", "").toString() + "/palmos41-en-m515.rom", settings.value("resourceDirectory", "").toString() + "/bootloader-en-m515.rom", FEATURE_ACCURATE);
      if(error == EMU_ERROR_NONE){
         ui->calender->setEnabled(true);
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
}

void MainWindow::on_debugger_clicked(){
   if(emu.isRunning()){
      emu.pause();
      ui->ctrlBtn->setIcon(QIcon(":/buttons/images/play.png"));
      emuDebugger->exec();
   }
   else{
      popupInformationDialog("Cant open debugger, emulator not running.");
   }
}

void MainWindow::on_screenshot_clicked(){
   uint64_t screenshotNumber = settings.value("screenshotNum", 0).toLongLong();
   QString path = settings.value("resourceDirectory", "").toString();
   QDir location = path + "/screenshots";

   if(!location.exists())
      location.mkpath(".");

   ui->display->pixmap()->save(path + "/screenshots/" + "screenshot" + QString::number(screenshotNumber, 10) + ".png", nullptr, 100);
   screenshotNumber++;
   settings.setValue("screenshotNum", screenshotNumber);
}
