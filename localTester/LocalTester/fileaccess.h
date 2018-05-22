#pragma once

#include <QString>
#include <stdint.h>

enum{
   FILE_ERR_NONE,
   FILE_DOESNT_EXIST,
   FILE_EMPTY_PATH,
   FILE_UNFINISHED,
   FILE_MODIFYED,
   FILE_INVALID_TYPE,
   FILE_PROTECTED,
   FILE_OUT_OF_MEMORY
};

uint8_t* getFileBuffer(QString filePath, uint64_t& size, uint32_t& error);
uint32_t setFileBuffer(QString filePath, uint8_t* data, size_t size);
bool validFilePath(QString path);
