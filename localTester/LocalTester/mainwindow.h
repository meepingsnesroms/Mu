#pragma once

#include <QMainWindow>
#include <QSslSocket>
#include <QString>

#include <atomic>
#include <thread>

#include "serialportio.h"
#include "userio.h"
#include "jssystem.h"
#include "testexecutionenviroment.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
   Q_OBJECT

public:
   explicit MainWindow(QWidget* parent = 0);
   ~MainWindow();

private slots:
   void jsThreadFunction(QString currentDependencyBlob, QString currentTestProgram, QString args);
   void launchJsThread(bool serialOverWifi);
   void waitForJsThread(bool desiredState);

   bool wifiValidate(QString location);

   void on_pickTestProgram_clicked();
   void on_startLocalTesting_clicked();
   void on_startRemoteTesting_clicked();
   void on_startTestProxy_clicked();
   void on_pickDependencyBlob_clicked();
   void on_pickSslCertificate_clicked();

private:
   std::thread jsThread;
   std::atomic<bool> jsThreadRunning;
   SerialPortIO* serialOut;
   UserIO* userTerminal;
   JSSystem* systemInterface;
   TestExecutionEnviroment testEnv;

   QString testProgram;
   QString dependencyBlob;
   QString sslCertPath;
   QSslSocket wifiConnection;
   Ui::MainWindow* ui;
};
