#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QFile>
#include <QStringList>
#include <QTimer>
#include <QImage>
#include <QPixmap>
#include <QFileDialog>
#include <QHostInfo>
#include <QNetworkConfiguration>
#include <QIntValidator>

#include <stdint.h>

#include "serialportio.h"
#include "userio.h"
#include "jssystem.h"
#include "testexecutionenviroment.h"


MainWindow::MainWindow(QWidget* parent) :
   QMainWindow(parent),
   ui(new Ui::MainWindow){
   ui->setupUi(this);

   ui->relayPort->setValidator(new QIntValidator(0, 65535, this));

   serialOut = nullptr;
   userTerminal = nullptr;
   systemInterface = nullptr;

   dependencyBlob = "";
   testProgram = "";

   //temporary
   testEnv.setErrorPath("/Users/Hoppy/Desktop/projects/PalmEmuRedo/libretro-palmm515emu/localTester/jsError.log");

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
   if(!testEnv.running()){
      ui->receiveText->setText(ui->receiveText->toPlainText() + "PROGRAM FINISHED!" + '\n');
      refreshWindow->stop();
   }
}

void MainWindow::launchJs(bool serialOverWifi){
   testEnv.finish();
   testEnv.clearExecutionEnviroment();
   ui->framebuffer->clear();

   systemInterface = new JSSystem();
   testEnv.installClass("jsSystem", systemInterface);
   userTerminal = new UserIO();
   testEnv.installClass("userIo", userTerminal);
   if(serialOverWifi){
      wifiSerialOut = new SerialOverWifi(ui->relayIp->text(), ui->relayPort->text().toUInt(), QByteArray());
      testEnv.installClass("serialPort", wifiSerialOut);
   }
   else{
      serialOut = new SerialPortIO(ui->serialPort->currentText());
      testEnv.installClass("serialPort", serialOut);
   }

   testEnv.execute(dependencyBlob, "", false);
   testEnv.finish();
   testEnv.execute(testProgram, ui->sendText->text(), true);
   ui->sendText->clear();
   refreshWindow->start(16);//update the window every 16.67miliseconds = 60 * second
}

bool MainWindow::wifiValidate(){
   //returns IP address if passed an IP address or website name, WiFi connected, SSL cert selected and connection succeeded otherwise returns an empty string
   QString location = ui->relayIp->text();
   QString port = ui->relayPort->text();
   QNetworkConfiguration network;
   if(network.state() & QNetworkConfiguration::Active && location != "" && port != "" && sslCertPath != ""){
      QHostInfo info = QHostInfo::fromName(location);
      if(info.addresses().length() > 0)
         return true;
   }

   return false;
}

void MainWindow::on_pickTestProgram_clicked(){
   QString lastTestProgramPath = appSettings.value("lastTestProgramPath", QDir::root().path()).toString();
   QString programPath = QFileDialog::getOpenFileName(this, "Select Test Program", lastTestProgramPath, 0);
   QFile testProgramData(programPath);

   if(testProgramData.open(QFile::ReadOnly)){
      testProgram = testProgramData.readAll();
      testProgramData.close();
   }
   else{
      testProgram = "";
   }
}

void MainWindow::on_startLocalTesting_clicked(){
   if(!testEnv.running() && ui->serialPort->currentText() != "")
      launchJs(false);
}

void MainWindow::on_startRemoteTesting_clicked(){
   //only run if WiFi available and IP address/website URL exists
   if(!testEnv.running() && wifiValidate())
      launchJs(true);
}

void MainWindow::on_startTestProxy_clicked(){

}

void MainWindow::on_pickDependencyBlob_clicked(){
   QString lastDependencyBlobPath = appSettings.value("lastDependencyBlobPath", QDir::root().path()).toString();
   QString blobPath = QFileDialog::getOpenFileName(this, "Select Dependency Blob", lastDependencyBlobPath, 0);
   QFile dependencyBlobData(blobPath);

   if(dependencyBlobData.open(QFile::ReadOnly)){
      dependencyBlob = dependencyBlobData.readAll();
      dependencyBlobData.close();
   }
   else{
      dependencyBlob = "";
   }
}

void MainWindow::on_pickSslCertificate_clicked(){
   QString lastSslCertPath = appSettings.value("lastSslCertPath", QDir::root().path()).toString();

   sslCertPath = QFileDialog::getOpenFileName(this, "Select SSL Certificate", lastSslCertPath, 0);
   appSettings.setValue("lastSslCertPath", sslCertPath);
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

void MainWindow::on_sendText_returnPressed(){
   if(testEnv.running()){
      userTerminal->writeStringCxx(ui->sendText->text());
      ui->sendText->clear();
   }
}

void MainWindow::on_clearTerminal_clicked(){
   ui->receiveText->clear();
}
