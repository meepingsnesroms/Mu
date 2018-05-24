#pragma once

#include <QJSEngine>
#include <QString>
#include <QObject>

#include <thread>
#include <atomic>

class TestExecutionEnviroment
{
private:
   QJSEngine* engine;
   std::thread jsThread;
   std::atomic<bool> jsRunning;
   QString lastProgramReturnValue;

   void jsThreadFunction(QString program, QString args, bool callMain);

public:
   TestExecutionEnviroment();
   ~TestExecutionEnviroment();

   void clearExecutionEnviroment();
   void installClass(QString name, QObject* jsClass);

   void execute(QString program, QString args, bool callMain);
   bool running() const{return jsRunning;}
   QString finish();
};


