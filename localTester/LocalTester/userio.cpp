#include "userio.h"

UserIO::UserIO(QObject* parent) : QObject(parent){

}

bool UserIO::stringAvailableJs(){
   bool empty;
   userData.lock();
   empty = cxxStrings.empty();
   userData.unlock();
   return !empty;
}

QString UserIO::readStringJs(){
   QString str = "";
   userData.lock();
   if(!cxxStrings.empty()){
      str = cxxStrings[0];
      cxxStrings.erase(cxxStrings.begin() + 0);
   }
   userData.unlock();
   return str;
}

void UserIO::writeStringJs(QString data){
   userData.lock();
   jsStrings.push_back(data);
   userData.unlock();
}

bool UserIO::stringAvailableCxx(){
   bool empty;
   userData.lock();
   empty = jsStrings.empty();
   userData.unlock();
   return !empty;
}

QString UserIO::readStringCxx(){
   QString str = "";
   userData.lock();
   if(!jsStrings.empty()){
      str = jsStrings[0];
      jsStrings.erase(jsStrings.begin() + 0);
   }
   userData.unlock();

   return str;
}

void UserIO::writeStringCxx(QString data){
   userData.lock();
   cxxStrings.push_back(data);
   userData.unlock();
}

void UserIO::resetStrings(){
   jsStrings.clear();
   cxxStrings.clear();
}
