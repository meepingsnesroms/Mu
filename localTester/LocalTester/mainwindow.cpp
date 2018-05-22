#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QFileDialog>
#include <QSslCertificate>
#include <QSslSocket>
#include <QNetworkConfiguration>

#include "fileaccess.h"
#include "serialportio.h"
#include "userio.h"
#include "testexecutionenviroment.h"


MainWindow::MainWindow(QWidget* parent) :
   QMainWindow(parent),
   ui(new Ui::MainWindow){
   ui->setupUi(this);
}

MainWindow::~MainWindow(){
   delete ui;
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
   //only run if serial port available
   if(ui->serialPort->currentText() != ""){
      js_class_t serialClass;
      js_class_t userIoClass;
      serialClass.name = "serialPort";
      serialClass.jsClass = new SerialPortIO(ui->serialPort->currentText());
      serialClass.name = "userIo";
      serialClass.jsClass = new UserIO();

      testEnv.clearExecutionEnviroment();
      testEnv.installClass(serialClass);
      testEnv.installClass(userIoClass);
      testEnv.executeString(dependencyBlob);
      testEnv.executeProgram(testProgram, ui->sendText->text());
      ui->sendText->clear();
   }
}

void MainWindow::on_startRemoteTesting_clicked(){
   //only run if WiFi available and IP address/website URL exists
   if(wifiValidate(ui->relayIp->text())){
      /*
      js_class_t serialClass;
      serialClass.name = "serialPort";
      serialClass.jsClass = new SerialPortIO(ui->serialPort->currentText());
      testEnv.clearExecutionEnviroment();
      testEnv.installClass(serialClass);
      testEnv.executeString(dependencyBlob);
      testEnv.executeProgram(testProgram, ui->sendText->text());
      ui->sendText->clear();
      */
   }
}

void MainWindow::on_startTestProxy_clicked(){

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
