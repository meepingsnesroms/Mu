#include <QJSEngine>
#include <QJSValue>
#include <QString>

#include <thread>
#include <chrono>
#include <stdint.h>

#include "testexecutionenviroment.h"


TestExecutionEnviroment::TestExecutionEnviroment(){
   jsRunning = false;
   lastProgramReturnValue = "";
   engine = new QJSEngine();
}

TestExecutionEnviroment::~TestExecutionEnviroment(){
   finish();
   engine->collectGarbage();
   delete engine;//deleting the engine frees all QObject classes passed to it automaticly
}

void TestExecutionEnviroment::jsThreadFunction(QString program, QString args, bool callMain){
   if(callMain){
      QJSValue jsGlobal = engine->globalObject();
      jsGlobal.setProperty("__programArgs", QJSValue(args));
      engine->evaluate(program);
      lastProgramReturnValue = engine->evaluate("main(__programArgs);").toString();
   }
   else{
      engine->evaluate(program);
   }
   jsRunning = false;
}

void TestExecutionEnviroment::clearExecutionEnviroment(){
   finish();
   engine->collectGarbage();
   delete engine;//deleting the engine frees all QObject classes passed to it automaticly
   engine = new QJSEngine();
}

void TestExecutionEnviroment::installClass(QString name, QObject* jsClass){
   QJSValue jsGlobal = engine->globalObject();
   jsGlobal.setProperty(name, engine->newQObject(jsClass));
}

void TestExecutionEnviroment::execute(QString program, QString args, bool callMain){
   if(!jsRunning){
      jsRunning = true;
      jsThread = std::thread(&TestExecutionEnviroment::jsThreadFunction, this, program, args, callMain);
   }
}

QString TestExecutionEnviroment::finish(){
   while(jsRunning)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
   if(jsThread.joinable())
      jsThread.join();

   return lastProgramReturnValue;
}
