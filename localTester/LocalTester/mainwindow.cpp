#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QStringList>
#include <QTimer>
#include <QImage>
#include <QPixmap>
#include <QFileDialog>
#include <QSslCertificate>
#include <QSslSocket>
#include <QNetworkConfiguration>

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

   serialOut = nullptr;
   userTerminal = nullptr;
   systemInterface = nullptr;

   dependencyBlob = "";
   testProgram = "";

   refreshWindow = new QTimer(this);
   connect(refreshWindow, SIGNAL(timeout()), this, SLOT(updateWindow()));
}

MainWindow::~MainWindow(){
   delete ui;
}

void MainWindow::updateWindow(){
   while(userTerminal->stringAvailableCxx())
      ui->receiveText->setText(ui->receiveText->toPlainText() + userTerminal->readStringCxx() + '\n');

   systemInterface->systemData.lock();
   if(systemInterface->framebufferRender)
      ui->framebuffer->setPixmap(QPixmap::fromImage(QImage((uchar*)systemInterface->framebufferPixels.data(), systemInterface->framebufferWidth, systemInterface->framebufferHeight, systemInterface->framebufferWidth * sizeof(uint16_t), QImage::Format_RGB16)));
   systemInterface->systemData.unlock();

   //javascript program has stopped, window updating service no longer needed
   if(!testEnv.running())
      refreshWindow->stop();
}

void MainWindow::launchJs(bool serialOverWifi){
   testEnv.clearExecutionEnviroment();

   if(serialOverWifi)
      /*wifi handler not done yet*/;
   else
      serialOut = new SerialPortIO(ui->serialPort->currentText());
   userTerminal = new UserIO();
   systemInterface = new JSSystem();

   testEnv.installClass("serialPort", serialOut);
   testEnv.installClass("userIo", userTerminal);
   testEnv.installClass("jsSystem", systemInterface);

   //start window updating service
   refreshWindow->start(16);//update the window every 16.67miliseconds = 60 * second

   testEnv.execute(dependencyBlob, "", false);
   testEnv.finish();
   testEnv.execute(testProgram, ui->sendText->text(), true);
   ui->sendText->clear();
   testEnv.finish();
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
   //ui->serialPort->setCurrentText("/dev/tty.Bluetooth-Incoming-Port");
   //testProgram = "userIo.writeStringJs(\"Wrote to terminal from js!\")";
   if(!testEnv.running() && ui->serialPort->currentText() != "")
      launchJs(false);
}

void MainWindow::on_startRemoteTesting_clicked(){
   //only run if WiFi available and IP address/website URL exists
   if(!testEnv.running() && wifiValidate(ui->relayIp->text()))
      launchJs(true);
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

void MainWindow::on_refreshSerialPorts_clicked(){
   QDir dev("/dev");
   QStringList filters;
   QStringList serialPorts;

   filters.append("tty.*");//Mac OS serial ports
   filters.append("ttyS*");//Linux serial ports
   dev.setNameFilters(filters);

   serialPorts = dev.entryList(QDir::System | QDir::Files, QDir::Name);

   ui->serialPort->clear();
   for(uint32_t port = 0; port < serialPorts.length(); port++)
      ui->serialPort->addItem("/dev/" + serialPorts[port]);
}
