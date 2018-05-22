#pragma once

#include <QMainWindow>
#include <QSslSocket>
#include <QString>

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
   bool wifiValidate(QString location);

   void on_pickTestProgram_clicked();
   void on_startLocalTesting_clicked();
   void on_startRemoteTesting_clicked();
   void on_startTestProxy_clicked();
   void on_pickDependencyBlob_clicked();
   void on_pickSslCertificate_clicked();

private:
   QString testProgram;
   QString dependencyBlob;
   QString sslCertPath;
   QSslSocket wifiConnection;
   TestExecutionEnviroment testEnv;
   Ui::MainWindow* ui;
};
