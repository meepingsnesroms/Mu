#include <QString>
#include <QPixmap>
#include <QImage>
#include <QFile>
#include <QByteArray>
#include <QDateTime>
#include <QDate>
#include <QTime>

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
   else
      newDebugString.append("MISSING \"\\n\"");


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
   emuVideoWidth = 0;
   emuVideoHeight = 0;
   emuNewFrameReady = false;
   emuDoubleBufferVideo = nullptr;

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
            memcpy(emuDoubleBufferVideo, emuVideoWidth == 320 ? palmExtendedFramebuffer : palmFramebuffer, emuVideoWidth * emuVideoHeight * sizeof(uint16_t));
            memcpy(emuDoubleBufferAudio, palmAudio, AUDIO_SAMPLES);
            emuNewFrameReady = true;
         }
      }
      else{
         emuPaused = true;
      }

      std::this_thread::sleep_for(std::chrono::microseconds(16666));
   }
}

uint32_t EmuWrapper::init(const QString& romPath, const QString& bootloaderPath, const QString& ramPath, const QString& sdCardPath, uint32_t features){
   if(!emuRunning && !emuInited){
      //start emu
      uint32_t error;
      QFile romFile(romPath);
      QFile bootloaderFile(bootloaderPath);
      QByteArray romData;
      QByteArray bootloaderData;
      buffer_t romBuff;
      buffer_t bootloaderBuff;

      if(romFile.open(QFile::ReadOnly | QFile::ExistingOnly)){
         romData = romFile.readAll();
         romFile.close();

         romBuff.data = (uint8_t*)romData.data();
         romBuff.size = romData.size();
      }
      else{
         return EMU_ERROR_INVALID_PARAMETER;
      }

      if(bootloaderPath != ""){
         if(bootloaderFile.open(QFile::ReadOnly | QFile::ExistingOnly)){
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
         QTime now = QTime::currentTime();

         emulatorSetRtc(QDate::currentDate().day(), now.hour(), now.minute(), now.second());

         if(ramPath != ""){
            QFile ramFile(ramPath);

            if(ramFile.exists()){
               if(ramFile.open(QFile::ReadOnly | QFile::ExistingOnly)){
                  QByteArray ramData;
                  buffer_t emuRam = emulatorGetRamBuffer();

                  ramData = ramFile.readAll();
                  ramFile.close();

                  //only copy in data if its the correct size, the file will be overwritten on exit with the devices RAM either way
                  //(changing RAM size requires a factory reset for now and always will when going from 128mb back to 16mb)
                  if(ramData.size() == emuRam.size)
                     memcpy(emuRam.data, ramData.data(), emuRam.size);
               }
            }
         }

         if(sdCardPath != ""){
            QFile sdCardFile(sdCardPath);

            if(sdCardFile.exists()){
               if(sdCardFile.open(QFile::ReadOnly | QFile::ExistingOnly)){
                  QByteArray sdCardData;
                  buffer_t newSdCard;

                  sdCardData = sdCardFile.readAll();
                  sdCardFile.close();

                  newSdCard.data = (uint8_t*)sdCardData.data();
                  newSdCard.size = sdCardData.size();

                  emulatorInsertSdCard(newSdCard);
               }
            }
         }

         if(features & FEATURE_320x320){
            emuVideoWidth = 320;
            emuVideoHeight = 320 + 120;
         }
         else{
            emuVideoWidth = 160;
            emuVideoHeight = 160 + 60;
         }

         emuInput = palmInput;
         emuRamFilePath = ramPath;
         emuSdCardFilePath = sdCardPath;

         emuThreadJoin = false;
         emuInited = true;
         emuRunning = true;
         emuPaused = false;
         emuNewFrameReady = false;
         emuDoubleBufferVideo = new uint16_t[emuVideoWidth * emuVideoHeight];
         emuDoubleBufferAudio = new int16_t[CRYSTAL_FREQUENCY * AUDIO_DUTY_CYCLE_SIZE * 2/*channels*/];
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
      if(emuRamFilePath != ""){
         QFile ramFile(emuRamFilePath);
         buffer_t emuRam = emulatorGetRamBuffer();

         //save out RAM before exit
         if(ramFile.open(QFile::WriteOnly | QFile::Truncate)){
            ramFile.write((const char*)emuRam.data, emuRam.size);
            ramFile.close();
         }
      }
      if(emuSdCardFilePath != ""){
         buffer_t emuSdCard = emulatorGetSdCardBuffer();

         if(emuSdCard.data){
            QFile sdCardFile(emuSdCardFilePath);

            //save out SD card before exit
            if(sdCardFile.open(QFile::WriteOnly | QFile::Truncate)){
               sdCardFile.write((const char*)emuSdCard.data, emuSdCard.size);
               sdCardFile.close();
            }
         }
      }
      emulatorExit();
      delete[] emuDoubleBufferVideo;
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

uint32_t EmuWrapper::saveState(const QString& path){
   bool wasPaused = isPaused();
   uint32_t error = EMU_ERROR_INVALID_PARAMETER;
   QFile stateFile(path);

   if(!wasPaused)
      pause();

   //save here
   if(stateFile.open(QFile::WriteOnly)){
      buffer_t stateData;

      stateData.size = emulatorGetStateSize();
      stateData.data = new uint8_t[stateData.size];

      emulatorSaveState(stateData);//no need to check for errors since the buffer is always the right size
      stateFile.write((const char*)stateData.data, stateData.size);
      stateFile.close();

      error = EMU_ERROR_NONE;
   }

   if(!wasPaused)
      resume();

   return error;
}

uint32_t EmuWrapper::loadState(const QString& path){
   bool wasPaused = isPaused();
   uint32_t error = EMU_ERROR_INVALID_PARAMETER;
   QFile stateFile(path);

   if(!wasPaused)
      pause();

   if(stateFile.open(QFile::ReadOnly)){
      QByteArray stateDataBuffer;
      buffer_t stateData;

      stateDataBuffer = stateFile.readAll();
      stateFile.close();
      stateData.data = (uint8_t*)stateDataBuffer.data();
      stateData.size = stateDataBuffer.size();

      if(emulatorLoadState(stateData))
         error = EMU_ERROR_NONE;
   }

   if(!wasPaused)
      resume();

   return error;
}

uint32_t EmuWrapper::installApplication(QString path){
   bool wasPaused = isPaused();
   uint32_t error = EMU_ERROR_INVALID_PARAMETER;
   QFile appFile(path);

   if(!wasPaused)
      pause();

   if(appFile.open(QFile::ReadOnly)){
      QByteArray appDataBuffer;
      buffer_t appData;

      appDataBuffer = appFile.readAll();
      appFile.close();
      appData.data = (uint8_t*)appDataBuffer.data();
      appData.size = appDataBuffer.size();
      error = emulatorInstallPrcPdb(appData);
   }

   if(!wasPaused)
      resume();

   return error;
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
