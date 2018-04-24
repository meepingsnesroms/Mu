#pragma once

#include <string>
#include <stdint.h>

enum{
   FRONTEND_ERR_NONE,
   FRONTEND_FILE_DOESNT_EXIST,
   FRONTEND_FILE_EMPTY_PATH,
   FRONTEND_FILE_UNFINISHED,
   FRONTEND_FILE_MODIFYED,
   FRONTEND_FILE_INVALID_TYPE,
   FRONTEND_FILE_PROTECTED,
   FRONTEND_OUT_OF_MEMORY
};

uint8_t* getFileBuffer(std::string filePath, size_t& size, uint32_t& error);
uint32_t setFileBuffer(std::string filePath, uint8_t* data, size_t size);
bool validFilePath(std::string path);
