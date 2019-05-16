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
#include "../../src/emulator.h"

extern "C"{
#include "../../src/flx68000.h"
#if defined(EMU_SUPPORT_PALM_OS5)
#include "../../src/pxa255/pxa255.h"
#endif
#include "../../src/debug/sandbox.h"
}


static bool alreadyExists = false;//there can only be one of this class since it wrappers C code

static std::vector<QString>  debugStrings;
static std::vector<uint64_t> duplicateCallCount;
uint32_t                     frontendDebugStringSize;
char*                        frontendDebugString;


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
   emuNewFrameReady = false;

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
         if(!emuNewFrameReady){
            palmInput = emuInput;
            emulatorRunFrame();
            emuNewFrameReady = true;
         }
      }
      else{
         emuPaused = true;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(3));
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

      if(romFile.open(QFile::ReadOnly | QFile::ExistingOnly)){
         romData = romFile.readAll();
         romFile.close();
      }
      else{
         return EMU_ERROR_INVALID_PARAMETER;
      }

      if(bootloaderPath != ""){
         if(bootloaderFile.open(QFile::ReadOnly | QFile::ExistingOnly)){
            bootloaderData = bootloaderFile.readAll();
            bootloaderFile.close();
         }
         else{
            return EMU_ERROR_INVALID_PARAMETER;
         }
      }

      error = emulatorInit((uint8_t*)romData.data(), romData.size(), (uint8_t*)bootloaderData.data(), bootloaderData.size(), features);
      if(error == EMU_ERROR_NONE){
         QTime now = QTime::currentTime();

         emulatorSetRtc(QDate::currentDate().day(), now.hour(), now.minute(), now.second());

         if(ramPath != ""){
            QFile ramFile(ramPath);

            if(ramFile.exists()){
               if(ramFile.open(QFile::ReadOnly | QFile::ExistingOnly)){
                  QByteArray ramData;

                  ramData = ramFile.readAll();
                  ramFile.close();

                  //only copy in data if its the correct size, the file will be overwritten on exit with the devices RAM either way
                  //(changing RAM size requires a factory reset for now and always will when going from 128mb back to 16mb)
                  if(ramData.size() == emulatorGetRamSize())
                     emulatorLoadRam((uint8_t*)ramData.data(), ramData.size());
               }
            }
         }

         if(sdCardPath != ""){
            QFile sdCardFile(sdCardPath);

            if(sdCardFile.exists()){
               if(sdCardFile.open(QFile::ReadOnly | QFile::ExistingOnly)){
                  QByteArray sdCardData;

                  sdCardData = sdCardFile.readAll();
                  sdCardFile.close();

                  emulatorInsertSdCard((uint8_t*)sdCardData.data(), sdCardData.size(), NULL);
               }
            }
         }

         emuInput = palmInput;
         emuRamFilePath = ramPath;
         emuSdCardFilePath = sdCardPath;

         emuThreadJoin = false;
         emuInited = true;
         emuRunning = true;
         emuPaused = false;
         emuNewFrameReady = false;
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
         uint32_t emuRamSize = emulatorGetRamSize();
         uint8_t* emuRamData = new uint8_t[emuRamSize];

         emulatorSaveRam(emuRamData, emuRamSize);

         //save out RAM before exit
         if(ramFile.open(QFile::WriteOnly | QFile::Truncate)){
            ramFile.write((const char*)emuRamData, emuRamSize);
            ramFile.close();
         }

         delete[] emuRamData;
      }
      if(emuSdCardFilePath != ""){
         uint32_t emuSdCardSize = emulatorGetSdCardSize();
         uint8_t* emuSdCardData = new uint8_t[emuSdCardSize];

         if(emulatorGetSdCardData(emuSdCardData, emuSdCardSize) == EMU_ERROR_NONE){
            QFile sdCardFile(emuSdCardFilePath);

            //save out SD card before exit
            if(sdCardFile.open(QFile::WriteOnly | QFile::Truncate)){
               sdCardFile.write((const char*)emuSdCardData, emuSdCardSize);
               sdCardFile.close();
            }
         }
      }
      emulatorExit();
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

void EmuWrapper::reset(bool hard){
   if(emuInited){
      bool wasPaused = isPaused();

      if(!wasPaused)
         pause();

      if(hard)
         emulatorHardReset();
      else
         emulatorSoftReset();

      if(!wasPaused)
         resume();
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
      uint32_t stateSize = emulatorGetStateSize();
      uint8_t* stateData = new uint8_t[stateSize];

      emulatorSaveState(stateData, stateSize);//no need to check for errors since the buffer is always the right size
      stateFile.write((const char*)stateData, stateSize);
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

      stateDataBuffer = stateFile.readAll();
      stateFile.close();

      if(emulatorLoadState((uint8_t*)stateDataBuffer.data(), stateDataBuffer.size()))
         error = EMU_ERROR_NONE;
   }

   if(!wasPaused)
      resume();

   return error;
}

void EmuWrapper::setPenValue(float x, float y, bool touched){
   emuInput.touchscreenX = x;
   emuInput.touchscreenY = y;
   emuInput.touchscreenTouched = touched;
}

void EmuWrapper::setKeyValue(uint8_t key, bool pressed){
   switch(key){
      case BUTTON_UP:
         emuInput.buttonUp = pressed;
         break;

      case BUTTON_DOWN:
         emuInput.buttonDown = pressed;
         break;

      case BUTTON_CALENDAR:
         emuInput.buttonCalendar = pressed;
         break;

      case BUTTON_ADDRESS:
         emuInput.buttonAddress = pressed;
         break;

      case BUTTON_TODO:
         emuInput.buttonTodo = pressed;
         break;

      case BUTTON_NOTES:
         emuInput.buttonNotes = pressed;
         break;

      case BUTTON_POWER:
         emuInput.buttonPower = pressed;
         break;

      default:
         break;
   }
}

uint32_t EmuWrapper::debugInstallApplication(const QString& path){
   bool wasPaused = isPaused();
   uint32_t error = EMU_ERROR_INVALID_PARAMETER;
   QFile appFile(path);

   if(!wasPaused)
      pause();

   if(appFile.open(QFile::ReadOnly)){
      QByteArray appDataBuffer = appFile.readAll();
      uintptr_t values[2];

      values[0] = (uintptr_t)appDataBuffer.data();
      values[1] = appDataBuffer.size();

      appFile.close();
      error = sandboxCommand(SANDBOX_CMD_DEBUG_INSTALL_APP, values);
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

QString EmuWrapper::getCpuRegisterString(){
   QString regString = "";

#if defined(EMU_SUPPORT_PALM_OS5)
   if(palmEmulatingTungstenC){
      for(uint8_t regs = 0; regs < 16; regs++)
         regString += QString::asprintf("R%d:0x%08X\n", regs, pxa255GetRegister(regs));
      regString.resize(regString.size() - 1);//remove extra '\n'
   }
   else{
#endif
      for(uint8_t dRegs = 0; dRegs < 8; dRegs++)
         regString += QString::asprintf("D%d:0x%08X\n", dRegs, flx68000GetRegister(dRegs));
      for(uint8_t aRegs = 0; aRegs < 8; aRegs++)
         regString += QString::asprintf("A%d:0x%08X\n", aRegs, flx68000GetRegister(8 + aRegs));
      regString += QString::asprintf("SP:0x%08X\n", flx68000GetRegister(15));
      regString += QString::asprintf("PC:0x%08X\n", flx68000GetRegister(16));
      regString += QString::asprintf("SR:0x%04X", flx68000GetRegister(17));
#if defined(EMU_SUPPORT_PALM_OS5)
   }
#endif

   return regString;
}

uint64_t EmuWrapper::getEmulatorMemory(uint32_t address, uint8_t size){
   return flx68000ReadArbitraryMemory(address, size);
}
