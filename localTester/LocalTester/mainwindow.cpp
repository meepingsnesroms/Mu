#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QFileDialog>
#include <QSslCertificate>
#include <QSslSocket>
#include <QNetworkConfiguration>

#include <chrono>
#include <thread>
#include <stdint.h>

#include "fileaccess.h"
#include "serialportio.h"
#include "userio.h"
#include "jssystem.h"
#include "testexecutionenviroment.h"


MainWindow::MainWindow(QWidget* parent) :
   QMainWindow(parent),
   ui(new Ui::MainWindow){
   ui->setupUi(this);

   jsThreadRunning = false;

   serialOut = nullptr;
   userTerminal = nullptr;
   systemInterface = nullptr;
}

MainWindow::~MainWindow(){
   if(jsThread.joinable())
      jsThread.join();

   delete ui;
}

void MainWindow::jsThreadFunction(QString currentDependencyBlob, QString currentTestProgram, QString args){
   jsThreadRunning = true;
   testEnv.executeString(currentDependencyBlob);
   testEnv.executeProgram(currentTestProgram, args);
   jsThreadRunning = false;
}

void MainWindow::launchJsThread(bool serialOverWifi){
   js_class_t serialClass;
   js_class_t userIoClass;
   js_class_t systemClass;

   testEnv.clearExecutionEnviroment();

   if(serialOverWifi)
      /*wifi handler not done yet*/;
   else
      serialOut = new SerialPortIO(ui->serialPort->currentText());
   userTerminal = new UserIO();
   systemInterface = new JSSystem();

   serialClass.name = "serialPort";
   serialClass.jsClass = serialOut;
   userIoClass.name = "userIo";
   userIoClass.jsClass = userTerminal;
   systemClass.name = "jsSystem";
   systemClass.jsClass = systemInterface;

   testEnv.installClass(serialClass);
   testEnv.installClass(userIoClass);
   testEnv.installClass(systemClass);
   jsThread = std::thread(&MainWindow::jsThreadFunction, this, dependencyBlob, testProgram, ui->serialPort->currentText());
   ui->sendText->clear();
}

void MainWindow::waitForJsThread(bool desiredState){
   while(jsThreadRunning != desiredState)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

bool MainWindow::wifiValidate(QString location){
   //returns true if WiFi connected, SSL cert selected and connection succeeded
   QNetworkConfiguration network;
   if(network.state() & QNetworkConfiguration::Active && location != "" && sslCertPath != ""){


   }

   return false;
}

void MainWindow::on_pickTestProgram_clicked(){
   uint64_t size;
   uint32_t error;
   uint8_t* data;
   QString programPath = QFileDialog::getOpenFileName(this, "Select Test Program", QDir::root().path(), 0);
   data = getFileBuffer(programPath, size, error);
   if(error == FILE_ERR_NONE)
      testProgram = QString::fromLatin1((const char*)data, size);
   else
      testProgram = "";
}

void MainWindow::on_startLocalTesting_clicked(){
   /*
   if(!testProgramAlreadyRunning){
      //only run if serial port available
      if(ui->serialPort->currentText() != ""){
         js_class_t serialClass;
         js_class_t userIoClass;
         js_class_t systemClass;
         serialClass.name = "serialPort";
         serialClass.jsClass = new SerialPortIO(ui->serialPort->currentText());
         userIoClass.name = "userIo";
         userIoClass.jsClass = new UserIO();
         systemClass.name = "jsSystem";
         systemClass.jsClass = new JSSystem();
         serialOut = serialClass.jsClass;
         userTerminal = userIoClass.jsClass;
         systemInterface = systemClass.jsClass;

         testEnv.clearExecutionEnviroment();
         testEnv.installClass(serialClass);
         testEnv.installClass(userIoClass);
         testEnv.executeString(dependencyBlob);
         testEnv.executeProgram(testProgram, ui->sendText->text());
         ui->sendText->clear();

         //these objects get destroyed properly when javascript exits, trying to free them will cause a use after free execption
         serialOut = nullptr;
         userTerminal = nullptr;
         systemInterface = nullptr;
      }
   }
   */
   if(!jsThreadRunning && ui->serialPort->currentText() != "")
      launchJsThread(false);
}

void MainWindow::on_startRemoteTesting_clicked(){
   //only run if WiFi available and IP address/website URL exists
   if(!jsThreadRunning && wifiValidate(ui->relayIp->text()))
      launchJsThread(true);
}

void MainWindow::on_startTestProxy_clicked(){
   if(!jsThreadRunning){

   }
}

void MainWindow::on_pickDependencyBlob_clicked(){
   uint64_t size;
   uint32_t error;
   uint8_t* data;
   QString blobPath = QFileDialog::getOpenFileName(this, "Select Dependency Blob", QDir::root().path(), 0);
   data = getFileBuffer(blobPath, size, error);
   if(error == FILE_ERR_NONE)
      dependencyBlob = QString::fromLatin1((const char*)data, size);
   else
      dependencyBlob = "";
}

void MainWindow::on_pickSslCertificate_clicked(){
   sslCertPath = QFileDialog::getOpenFileName(this, "Select SSL Certificate", QDir::root().path(), 0);
}
