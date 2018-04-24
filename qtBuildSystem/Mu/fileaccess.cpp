#include <QtGlobal>

#include <new>
#include <string>
#include <stdint.h>
#include <ctype.h>
#include <sys/stat.h>

#include "fileaccess.h"

uint8_t* getFileBuffer(std::string filePath, size_t& size, uint32_t& error){
   uint8_t* rawData = NULL;
   if(filePath != ""){
      struct stat st;
      int doesntExist = stat(filePath.c_str(), &st);
      if(doesntExist == 0 && st.st_size){
         FILE* dataFile = fopen(filePath.c_str(), "rb");
         if(dataFile != NULL){
            rawData = new (std::nothrow) uint8_t[st.st_size];
            if(rawData){
               size_t bytesRead = fread(rawData, 1, st.st_size, dataFile);
               if(bytesRead == (size_t)st.st_size){
                  size = bytesRead;
                  error = FRONTEND_ERR_NONE;
               }
               else{
                  error = FRONTEND_FILE_UNFINISHED;
               }
            }
            else{
               error = FRONTEND_OUT_OF_MEMORY;
            }
            fclose(dataFile);
         }
         else{
            error = FRONTEND_FILE_PROTECTED;
         }
      }
      else{
         error = FRONTEND_FILE_DOESNT_EXIST;
      }
   }
   else{
      error = FRONTEND_FILE_EMPTY_PATH;
   }

   if(error != FRONTEND_ERR_NONE){
      if(rawData)
         delete[] rawData;
      rawData = NULL;
      size = 0;
   }

   return rawData;
}

uint32_t setFileBuffer(std::string filePath, uint8_t* data, size_t size){
   uint32_t error = FRONTEND_ERR_NONE;
   if(filePath != ""){
      FILE* dataFile = fopen(filePath.c_str(), "wb");
      if(dataFile != NULL){
         size_t bytesWritten = fwrite(data, 1, size, dataFile);
         if(bytesWritten != size)
            error = FRONTEND_FILE_UNFINISHED;
         fclose(dataFile);
      }
      else{
         error = FRONTEND_FILE_PROTECTED;
      }
   }
   else{
      error = FRONTEND_FILE_EMPTY_PATH;
   }
   return error;
}

bool validFilePath(std::string path){
#ifdef Q_OS_WIN
   if(path.length() < 3 || !isalpha(path[0]) || path[1] != ':' || (path[2] != '/' && path[2] != '\\'))
      return false;
#else
   if(path.length() < 1 || path[0] != '/')
      return false;
#endif
   for(uint32_t count = 0; count < path.length(); count++){
      if(!isalnum(path[count]) && !ispunct(path[count]))
         return false;
   }
   return true;
}
