#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QFileDialog>
#include <QTimer>
#include <QTouchEvent>
#include <QMessageBox>
#include <QSettings>
#include <QFont>
#include <QKeyEvent>

#include <atomic>
#include <mutex>
#include <stdint.h>

#include "hexviewer.h"
#include "fileaccess.h"
#include "src/emulator.h"


uint32_t screenWidth;
uint32_t screenHeight;
QSettings settings;

static QImage video;
static QTimer* refreshDisplay;
static HexViewer* emuStateBrowser;
std::mutex emuMutex;
static std::atomic<bool> emuOn;
static std::atomic<bool> emuInited;
static uint8_t romBuffer[ROM_SIZE];


MainWindow::MainWindow(QWidget* parent) :
   QMainWindow(parent),
   ui(new Ui::MainWindow){
   ui->setupUi(this);

   emuStateBrowser = new HexViewer(this);
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

   ui->ctrlBtn->setText("Start");

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

   emuOn = false;
   emuInited = false;
   screenWidth = 160;
   screenHeight = 160 + 60;

   loadRom();

#if !defined(FRONTEND_DEBUG)
   ui->hexViewer->hide();
#endif

   connect(refreshDisplay, SIGNAL(timeout()), this, SLOT(updateDisplay()));
   refreshDisplay->start(16);//update display every 16.67miliseconds = 60 * second
}

MainWindow::~MainWindow(){
   emuMutex.lock();
   if(emuInited)
      emulatorExit();
   emuMutex.unlock();
   delete ui;
}

void MainWindow::popupErrorDialog(QString error){
   QMessageBox::information(this, "Mu", error, QMessageBox::Ok);
}

bool MainWindow::eventFilter(QObject *object, QEvent *event){
   if(QString(object->metaObject()->className()) == "QPushButton" && event->type() == QEvent::Resize){
      QPushButton* button = (QPushButton*)object;
      button->setIconSize(QSize(button->size().width() / 1.7, button->size().height() / 1.7));
   }

   return QMainWindow::eventFilter(object, event);
}

void MainWindow::loadRom(){
   QString rom = settings.value("resourceDirectory", "").toString() + "/palmos41-en-m515.rom";
   uint32_t error;
   size_t size;
   uint8_t* romData = getFileBuffer(rom, size, error);
   if(romData){
      size_t romSize = size < ROM_SIZE ? size : ROM_SIZE;
      memcpy(romBuffer, romData, romSize);
      if(romSize < ROM_SIZE)
         memset(romBuffer + romSize, 0x00, ROM_SIZE - romSize);
      delete[] romData;
   }

   if(error != FRONTEND_ERR_NONE)
      popupErrorDialog("Could not open ROM file");
   ui->ctrlBtn->setEnabled(error == FRONTEND_ERR_NONE);
}

void MainWindow::selectHomePath(){
   QString dir = QFileDialog::getOpenFileName(this, "New Home Directory \"~/Mu\" is default", QDir::root().path(), 0);
   settings.setValue("resourceDirectory", dir);
}

void MainWindow::on_install_pressed(){
   QString app = QFileDialog::getOpenFileName(this, "Open Prc/Pdb/Pqa", QDir::root().path(), 0);
   uint32_t error;
   size_t size;
   uint8_t* appData = getFileBuffer(app, size, error);
   if(appData){
      error = emulatorInstallPrcPdb(appData, size);
      delete[] appData;
   }

   if(error != FRONTEND_ERR_NONE)
      popupErrorDialog("Could not install app");
}

//display
void MainWindow::updateDisplay(){
   if(emuOn){
      if(emuMutex.try_lock()){
#if defined(FRONTEND_DEBUG) && defined(EMU_DEBUG)
         if(emulateUntilDebugEventOrFrameEnd()){
            //debug event occured
            emuOn = false;
            ui->ctrlBtn->setText("Resume");
         }
#else
         emulateFrame();
#endif

         video = QImage((unsigned char*)palmFramebuffer, screenWidth, screenHeight, QImage::Format_RGB16);//16 bit
         ui->display->setPixmap(QPixmap::fromImage(video).scaled(ui->display->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
         ui->display->update();
         emuMutex.unlock();
      }
   }
}

//palm buttons
void MainWindow::on_power_pressed(){
   palmInput.buttonPower = true;
}

void MainWindow::on_power_released(){
   palmInput.buttonPower = false;
}

void MainWindow::on_calender_pressed(){
   palmInput.buttonCalender = true;
}

void MainWindow::on_calender_released(){
   palmInput.buttonCalender = false;
}

void MainWindow::on_addressBook_pressed(){
   palmInput.buttonAddress = true;
}

void MainWindow::on_addressBook_released(){
   palmInput.buttonAddress = false;
}

void MainWindow::on_todo_pressed(){
   palmInput.buttonTodo = true;
}

void MainWindow::on_todo_released(){
   palmInput.buttonTodo = false;
}

void MainWindow::on_notes_pressed(){
   palmInput.buttonNotes = true;
}

void MainWindow::on_notes_released(){
   palmInput.buttonNotes = false;
}

//ui buttons
void MainWindow::on_mainLeft_clicked(){
   ui->controlPanel->setCurrentWidget(ui->controlPanel->widget(2));
}

void MainWindow::on_mainRight_clicked(){
   ui->controlPanel->setCurrentWidget(ui->controlPanel->widget(1));
}

void MainWindow::on_settingsLeft_clicked(){
   ui->controlPanel->setCurrentWidget(ui->controlPanel->widget(0));
}

void MainWindow::on_settingsRight_clicked(){
   ui->controlPanel->setCurrentWidget(ui->controlPanel->widget(2));
}

void MainWindow::on_joyLeft_clicked(){
   ui->controlPanel->setCurrentWidget(ui->controlPanel->widget(1));
}

void MainWindow::on_joyRight_clicked(){
   ui->controlPanel->setCurrentWidget(ui->controlPanel->widget(0));
}

void MainWindow::on_settings_clicked(){
   //setprocess(SETUP);
}

//emu control
void MainWindow::on_ctrlBtn_clicked(){
   emuMutex.lock();
   if(!emuOn && !emuInited){
      //start emu
      emulatorInit(romBuffer, NULL/*bootloader*/, ACCURATE);
      emuInited = true;
      emuOn = true;
      ui->ctrlBtn->setText("Pause");
   }
   else if(emuOn){
      emuOn = false;
      ui->ctrlBtn->setText("Resume");
   }
   else if(!emuOn){
      emuOn = true;
      ui->ctrlBtn->setText("Pause");
   }
   emuMutex.unlock();
}

void MainWindow::on_hexViewer_clicked(){
   emuMutex.lock();
   if(emuOn){
      emuOn = false;
      ui->ctrlBtn->setText("Resume");
   }

   emuStateBrowser->exec();

   emuMutex.unlock();
}

void MainWindow::on_screenshot_clicked(){
   uint64_t screenshotNumber = settings.value("screenshotNum", 0).toLongLong();
   QString path = settings.value("resourceDirectory", "").toString();
   QDir location = path + "/screenshots";

   if(!location.exists())
      location.mkpath(".");

   video.save(path + "/screenshots/" + "screenshot" + QString::number(screenshotNumber, 10) + ".png", NULL, 100);
   screenshotNumber++;
   settings.setValue("screenshotNum", screenshotNumber);
}
