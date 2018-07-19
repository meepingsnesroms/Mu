#include <QString>
#include <QPixmap>
#include <QImage>
#include <QFile>
#include <QByteArray>

#include <new>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <stdint.h>

#include "emuwrapper.h"
#include "src/emulator.h"

extern "C"{
#include "src/m68k/m68k.h"
#include "src/memoryAccess.h"
#include "src/hardwareRegisters.h"
}


static bool alreadyExists = false;//there can only be one of this class since it wrappers C code

static std::vector<QString>  debugStrings;
static std::vector<uint64_t> duplicateCallCount;
uint32_t                     frontendDebugStringSize;
char*                        frontendDebugString;


void frontendHandleDebugClearLogs(){
   debugStrings.clear();
   duplicateCallCount.clear();
}

void frontendHandleDebugPrint(){
   QString newDebugString = frontendDebugString;

   //this debug handler doesnt need the \n
   if(newDebugString.back() == '\n')
      newDebugString.remove(newDebugString.length() - 1, 1);

   if(!debugStrings.empty() && newDebugString == debugStrings.back()){
      duplicateCallCount.back()++;
   }
   else{
      debugStrings.push_back(newDebugString);
      duplicateCallCount.push_back(1);
   }
}

EmuWrapper::EmuWrapper(){
   if(alreadyExists == true)
      throw std::bad_alloc();
   alreadyExists = true;

   emuInited = false;
   emuThreadJoin = false;
   emuRunning = false;
   emuPaused = false;
   emuDebugEvent = false;
   emuVideoWidth = 0;
   emuVideoHeight = 0;
   emuNewFrameReady = false;
   emuDoubleBuffer = NULL;

   frontendDebugString = new char[200];
   frontendDebugStringSize = 200;
}

EmuWrapper::~EmuWrapper(){
   if(emuInited)
      exit();

   delete[] frontendDebugString;
   frontendDebugStringSize = 0;
   debugStrings.clear();
   duplicateCallCount.clear();
}

void EmuWrapper::emuThreadRun(){
   while(!emuThreadJoin){
      if(emuRunning){
         emuPaused = false;
         palmInput = emuInput;
         emulateFrame();
         if(!emuNewFrameReady){
            memcpy(emuDoubleBuffer, emuVideoWidth == 320 ? palmExtendedFramebuffer : palmFramebuffer, emuVideoWidth * emuVideoHeight * sizeof(uint16_t));
            emuNewFrameReady = true;
         }
      }
      else{
         emuPaused = true;
      }

      std::this_thread::sleep_for(std::chrono::microseconds(16666));
   }
}

uint32_t EmuWrapper::init(QString romPath, QString bootloaderPath, uint32_t features){
   if(!emuRunning && !emuInited){
      //start emu
      uint32_t error;
      QFile romFile(romPath);
      QFile bootloaderFile(bootloaderPath);
      QByteArray romData;
      QByteArray bootloaderData;
      buffer_t romBuff;
      buffer_t bootloaderBuff;

      if(romFile.open(QFile::ReadOnly)){
         romData = romFile.readAll();
         romFile.close();

         romBuff.data = (uint8_t*)romData.data();
         romBuff.size = romData.size();
      }
      else{
         return EMU_ERROR_INVALID_PARAMETER;
      }

      if(bootloaderPath != "" && bootloaderFile.exists()){
         if(bootloaderFile.open(QFile::ReadOnly)){
            bootloaderData = bootloaderFile.readAll();
            bootloaderFile.close();

            bootloaderBuff.data = (uint8_t*)bootloaderData.data();
            bootloaderBuff.size = bootloaderData.size();
         }
         else{
            return EMU_ERROR_INVALID_PARAMETER;
         }
      }
      else{
         bootloaderBuff.data = NULL;
         bootloaderBuff.size = 0;
      }

      error = emulatorInit(romBuff, bootloaderBuff, features);
      if(error == EMU_ERROR_NONE){
         if(features & FEATURE_320x320){
            emuVideoWidth = 320;
            emuVideoHeight = 320 + 120;
         }
         else{
            emuVideoWidth = 160;
            emuVideoHeight = 160 + 60;
         }

         emuInput = palmInput;

         emuThreadJoin = false;
         emuInited = true;
         emuRunning = true;
         emuPaused = false;
         emuNewFrameReady = false;
         emuDoubleBuffer = new uint16_t[emuVideoWidth * emuVideoHeight];
         emuThread = std::thread(&EmuWrapper::emuThreadRun, this);
      }
      else{
         return error;
      }
   }

   return EMU_ERROR_NONE;
}

void EmuWrapper::exit(){
   emuThreadJoin = true;
   emuRunning = false;
   if(emuThread.joinable())
      emuThread.join();
   if(emuInited){
      emulatorExit();
      delete[] emuDoubleBuffer;
   }
}

void EmuWrapper::pause(){
   if(emuInited){
      emuRunning = false;
      while(!emuPaused)
         std::this_thread::sleep_for(std::chrono::milliseconds(1));
   }
}

void EmuWrapper::resume(){
   if(emuInited){
      emuRunning = true;
      while(emuPaused)
         std::this_thread::sleep_for(std::chrono::milliseconds(1));
   }
}

uint32_t EmuWrapper::installApplication(QString path){
   QFile appFile(path);

   if(appFile.open(QFile::ReadOnly)){
      QByteArray appDataBuffer;
      buffer_t appData;

      appDataBuffer = appFile.readAll();
      appFile.close();
      appData.data = (uint8_t*)appDataBuffer.data();
      appData.size = appDataBuffer.size();
      if(appData.size != 0)
         return emulatorInstallPrcPdb(appData);
   }

   return EMU_ERROR_OUT_OF_MEMORY;
}

std::vector<QString>& EmuWrapper::getDebugStrings(){
   return debugStrings;
}

std::vector<uint64_t>& EmuWrapper::getDuplicateCallCount(){
   return duplicateCallCount;
}

std::vector<uint32_t> EmuWrapper::getCpuRegisters(){
   std::vector<uint32_t> registers;
   for(uint8_t reg = M68K_REG_D0; reg <= M68K_REG_SR; reg++)
      registers.push_back(m68k_get_reg(NULL, (m68k_register_t)reg));
   return registers;
}

QPixmap EmuWrapper::getFramebuffer(){
   emuNewFrameReady = false;
   return QPixmap::fromImage(QImage((uchar*)emuDoubleBuffer, emuVideoWidth, emuVideoHeight, emuVideoWidth * sizeof(uint16_t), QImage::Format_RGB16));
}

bool EmuWrapper::getPowerButtonLed(){
   return palmMisc.powerButtonLed;
}

uint64_t EmuWrapper::getEmulatorMemory(uint32_t address, uint8_t size){
   uint64_t data = UINT64_MAX;//invalid access
   //until SPI and UART destructive reads are implemented all reads to mapped addresses are safe, SPI is now implemented, this needs to be fixed
   if(bankType[START_BANK(address)] != CHIP_NONE){
      uint16_t m68kSr = m68k_get_reg(NULL, M68K_REG_SR);
      m68k_set_reg(M68K_REG_SR, 0x2000);//prevent privilege violations
      switch(size){
         case 8:
            data = m68k_read_memory_8(address);
            break;

         case 16:
            data = m68k_read_memory_16(address);
            break;

         case 32:
            data = m68k_read_memory_32(address);
            break;
      }
      m68k_set_reg(M68K_REG_SR, m68kSr);
   }

   return data;
}
