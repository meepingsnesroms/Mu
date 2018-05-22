#pragma once

#include <QJSEngine>
#include <QString>
#include <QObject>

typedef struct{
   QString name;
   QObject* jsClass;
}js_class_t;

class TestExecutionEnviroment
{
private:
   QJSEngine* engine;
   std::vector<js_class_t> customClasses;

public:
   TestExecutionEnviroment();
   ~TestExecutionEnviroment();

   void clearExecutionEnviroment();
   void installClass(js_class_t jsClass);

   QString executeProgram(QString code, QString args);
   QString executeString(QString code);
};


