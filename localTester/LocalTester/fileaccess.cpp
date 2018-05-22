#include <QtGlobal>
#include <QFile>

#include <new>
#include <string>
#include <stdint.h>

#include "fileaccess.h"

uint8_t* getFileBuffer(QString filePath, uint64_t& size, uint32_t& error){
   QFile file(filePath);
   QByteArray tempArray;
   uint8_t* rawData = NULL;

   if(filePath == ""){
      size = 0;
      error = FILE_EMPTY_PATH;
      return NULL;
   }

   if(!file.exists()){
      size = 0;
      error = FILE_DOESNT_EXIST;
      return NULL;
   }

   if(!file.open(QIODevice::ReadOnly)){
      size = 0;
      error = FILE_PROTECTED;
      return NULL;
   }

   tempArray = file.readAll();
   file.close();

   rawData = new uint8_t[tempArray.size()];
   memcpy(rawData, tempArray.data(), tempArray.size());
   size = tempArray.size();
   error = FILE_ERR_NONE;
   return rawData;
}

uint32_t setFileBuffer(QString filePath, uint8_t* data, size_t size){
   QFile file(filePath);
   size_t bytesWritten;

   if(filePath == "")
      return FILE_EMPTY_PATH;

   if(!file.open(QIODevice::WriteOnly))
      return FILE_PROTECTED;

   bytesWritten = file.write((const char*)data, size);

   file.close();

   if(bytesWritten != size)
      return FILE_UNFINISHED;

   return FILE_ERR_NONE;
}

bool validFilePath(QString path){
#if defined(Q_OS_WIN)
   if(path.length() < 3 || !path[0].isLetter() || path[1] != ':' || (path[2] != '/' && path[2] != '\\'))
      return false;
#else
   if(path.length() < 1 || path[0] != '/')
      return false;
#endif
   for(int count = 0; count < path.length(); count++){
      if(!path[count].isLetterOrNumber() && !path[count].isPunct())
         return false;
   }
   return true;
}
