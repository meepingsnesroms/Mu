#include <QJSEngine>
#include <QString>

#include <vector>
#include <stdint.h>

#include "testexecutionenviroment.h"


TestExecutionEnviroment::TestExecutionEnviroment(){
   engine = new QJSEngine();
}

TestExecutionEnviroment::~TestExecutionEnviroment(){
   engine->collectGarbage();
   delete engine;//deleting the engine frees all QObject classes passed to it automaticly
   customClasses.clear();
}

void TestExecutionEnviroment::clearExecutionEnviroment(){
   engine->collectGarbage();
   delete engine;//deleting the engine frees all QObject classes passed to it automaticly
   engine = new QJSEngine();
   customClasses.clear();
}

void TestExecutionEnviroment::installClass(js_class_t jsClass){
   QJSValue jsGlobal = engine->globalObject();
   QJSValue convertedClass = engine->newQObject(jsClass.jsClass);
   jsGlobal.setProperty(jsClass.name, convertedClass);
   customClasses.push_back(jsClass);
}

QString TestExecutionEnviroment::executeProgram(QString code, QString args){
   QJSValue jsGlobal = engine->globalObject();
   jsGlobal.setProperty("__programArgs", QJSValue(args));
   engine->evaluate(code);

   return engine->evaluate("main(__programArgs);").toString();
}

QString TestExecutionEnviroment::executeString(QString code){
   return engine->evaluate(code).toString();
}
