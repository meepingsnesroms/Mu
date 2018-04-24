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

static QImage video;
static QTimer* refreshDisplay;
static HexViewer* emuStateBrowser;
static QSettings settings;
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
   emulatorExit();
   emuMutex.unlock();
   delete ui;
}

void MainWindow::popupErrorDialog(std::string error){
   QMessageBox::information(this, "Mu", QString::fromStdString(error), QMessageBox::Ok);
}

bool MainWindow::eventFilter(QObject *object, QEvent *event){
   if(std::string(object->metaObject()->className()) == "QPushButton" && event->type() == QEvent::Resize){
      QPushButton* button = (QPushButton*)object;
      button->setIconSize(QSize(button->size().width() / 1.7, button->size().height() / 1.7));
   }

   return QMainWindow::eventFilter(object, event);
}

void MainWindow::loadRom(){
   std::string rom = settings.value("romPath", "").toString().toStdString();
   if(rom == ""){
      //keep the selector open until a valid file is picked
      while(settings.value("romPath", "").toString().toStdString() == "")
         selectRom();
   }

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

   if(error != FRONTEND_ERR_NONE){
      popupErrorDialog("Could not open ROM file");
   }
}

void MainWindow::selectRom(){
   std::string rom = QFileDialog::getOpenFileName(this, "Palm OS ROM (palmos41-en-m515.rom)", QDir::root().path(), 0).toStdString();
   uint32_t error;
   size_t size;
   uint8_t* romData = getFileBuffer(rom, size, error);
   if(romData){
      //valid file
      settings.setValue("romPath", QString::fromStdString(rom));
      delete[] romData;
   }


   if(error != FRONTEND_ERR_NONE){
      popupErrorDialog("Could not open ROM file");
   }
}

void MainWindow::on_install_pressed(){
   std::string app = QFileDialog::getOpenFileName(this, "Open Prc/Pdb/Pqa", QDir::root().path(), 0).toStdString();
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

void MainWindow::on_display_destroyed(){
   //do nothing
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
   emuOn = false;

   emuStateBrowser->exec();

   emuMutex.unlock();
}
