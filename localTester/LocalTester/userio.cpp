#include "userio.h"

UserIO::UserIO(QObject* parent) : QObject(parent){

}

void UserIO::setRefreshHandler(void (*newRefreshHandler)()){
   refreshHandler = newRefreshHandler;
}

bool UserIO::stringAvailableJs(){
   return !cxxStrings.empty();
}

QString UserIO::readStringJs(){
   if(!cxxStrings.empty()){
      QString currentString  = cxxStrings[0];
      cxxStrings.erase(cxxStrings.begin() + 0);
      return currentString;
   }

   return "";
}

void UserIO::writeStringJs(QString data){
   jsStrings.push_back(data);
   if(refreshHandler != nullptr)
      refreshHandler();
}

bool UserIO::stringAvailableCxx(){
   return !jsStrings.empty();
}

QString UserIO::readStringCxx(){
   if(!jsStrings.empty()){
      QString currentString  = jsStrings[0];
      jsStrings.erase(jsStrings.begin() + 0);
      return currentString;
   }

   return "";
}

void UserIO::writeStringCxx(QString data){
   cxxStrings.push_back(data);
}

void UserIO::resetStrings(){
   jsStrings.clear();
   cxxStrings.clear();
}
