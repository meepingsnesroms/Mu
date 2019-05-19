#include <QString>
#include <QVector>
#include <QPixmap>
#include <QImage>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QByteArray>
#include <QDateTime>
#include <QDate>
#include <QTime>

#include <new>
#include <chrono>
#include <thread>
#include <string>
#include <stdint.h>

#include "emuwrapper.h"
#include "../../src/emulator.h"
#include "../../src/fileLauncher/launcher.h"

extern "C"{
#include "../../src/flx68000.h"
#if defined(EMU_SUPPORT_PALM_OS5)
#include "../../src/pxa255/pxa255.h"
#endif
#include "../../src/debug/sandbox.h"
}


#define MAX_LOG_ENTRY_LENGTH 200


static bool alreadyExists = false;//there can only be one of this class since it wrappers C code

static QVector<QString>  debugStrings;
static QVector<uint64_t> duplicateCallCount;
uint32_t                 frontendDebugStringSize;
char*                    frontendDebugString;


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

   frontendDebugString = new char[MAX_LOG_ENTRY_LENGTH];
   frontendDebugStringSize = MAX_LOG_ENTRY_LENGTH;
}

EmuWrapper::~EmuWrapper(){
   if(emuInited)
      exit();

   delete[] frontendDebugString;
   frontendDebugStringSize = 0;
   debugStrings.clear();
   duplicateCallCount.clear();

   //allow creating a new emu class after the old one is closed
   alreadyExists = false;
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

void EmuWrapper::writeOutSaves(){
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
}

uint32_t EmuWrapper::init(const QString& romPath, const QString& bootloaderPath, const QString& ramPath, const QString& sdCardPath, uint32_t features){
   if(!emuRunning && !emuInited){
      //start emu
      uint32_t error;
      QFile romFile(romPath);
      QFile bootloaderFile(bootloaderPath);

      if(!romFile.open(QFile::ReadOnly | QFile::ExistingOnly))
         return EMU_ERROR_INVALID_PARAMETER;

      if(bootloaderPath != ""){
         if(!bootloaderFile.open(QFile::ReadOnly | QFile::ExistingOnly))
            return EMU_ERROR_INVALID_PARAMETER;
      }

      error = emulatorInit((uint8_t*)romFile.readAll().data(), romFile.size(), (uint8_t*)bootloaderFile.readAll().data(), bootloaderFile.size(), features);
      if(error == EMU_ERROR_NONE){
         QTime now = QTime::currentTime();

         emulatorSetRtc(QDate::currentDate().day(), now.hour(), now.minute(), now.second());

         if(ramPath != ""){
            QFile ramFile(ramPath);

            if(ramFile.open(QFile::ReadOnly | QFile::ExistingOnly)){
               emulatorLoadRam((uint8_t*)ramFile.readAll().data(), ramFile.size());
               ramFile.close();
            }
         }

         if(sdCardPath != ""){
            QFile sdCardFile(sdCardPath);

            if(sdCardFile.open(QFile::ReadOnly | QFile::ExistingOnly)){
               emulatorInsertSdCard((uint8_t*)sdCardFile.readAll().data(), sdCardFile.size(), NULL);
               sdCardFile.close();
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

      romFile.close();
      if(bootloaderPath != "")
         bootloaderFile.close();
   }

   return EMU_ERROR_NONE;
}

void EmuWrapper::exit(){
   emuThreadJoin = true;
   emuRunning = false;
   if(emuThread.joinable())
      emuThread.join();
   if(emuInited){
      writeOutSaves();
      emulatorDeinit();
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

uint32_t EmuWrapper::bootFromFileOrDirectory(const QString& mainPath){
   bool wasPaused = isPaused();
   uint32_t error = EMU_ERROR_NONE;
   QFileInfo pathInfo(mainPath);
   QStringList paths;
   QVector<QByteArray> fileDataBuffers;
   QVector<QByteArray> fileInfoBuffers;
   launcher_file_t* files;
   QFile ramFile(mainPath + ".ram");
   QFile sdCardFile(mainPath + ".sd.img");
   bool hasSaveRam;
   bool hasSaveSdCard;
   int newestBootableFile = -1;

   if(!wasPaused)
      pause();

   if(pathInfo.isDir()){
      //get Palm file list
      QDir dirInfo(mainPath);
      QStringList filters;

      filters += "*.prc";
      filters += "*.pdb";
      filters += "*.pqa";
      filters += "*.img";

      paths = dirInfo.entryList(filters);

      //make paths full direct paths
      for(int index = 0; index < paths.length(); index++)
         paths[index].prepend(mainPath + "/");
   }
   else if(pathInfo.exists()){
      //use single file
      paths = QStringList(mainPath);
   }
   else{
      //error
      error = EMU_ERROR_INVALID_PARAMETER;
      goto errorOccurred;
   }

   fileDataBuffers.resize(paths.length());
   fileInfoBuffers.resize(paths.length());
   files = new launcher_file_t[paths.length()];

   memset(files, 0x00, sizeof(launcher_file_t) * paths.length());

   for(int index = 0; index < paths.length(); index++){
      QFile appFile(paths[index]);

      if(appFile.open(QFile::ReadOnly | QFile::ExistingOnly)){
         QString suffix = QFileInfo(paths[index]).suffix().toLower();

         fileDataBuffers[index] = appFile.readAll();
         appFile.close();

         files[index].fileData = (uint8_t*)fileDataBuffers[index].data();
         files[index].fileSize = fileDataBuffers[index].size();

         if(suffix == "prc"){
            files[index].type = LAUNCHER_FILE_TYPE_PRC;
            newestBootableFile = index;
         }
         else if(suffix == "pdb"){
            files[index].type = LAUNCHER_FILE_TYPE_PDB;
         }
         else if(suffix == "pqa"){
            files[index].type = LAUNCHER_FILE_TYPE_PQA;
            newestBootableFile = index;
         }
         else if(suffix == "img"){
            QFile infoFile(paths[index].remove(paths[index].size() - 3, 3) + "info");//swap "img" for "info"

            if(infoFile.open(QFile::ReadOnly | QFile::ExistingOnly)){
               fileInfoBuffers[index] = infoFile.readAll();
               files[index].infoData = (uint8_t*)fileInfoBuffers[index].data();
               files[index].infoSize = fileInfoBuffers[index].size();
               infoFile.close();
            }
            files[index].type = LAUNCHER_FILE_TYPE_IMG;
            newestBootableFile = index;
         }
         else{
            error = EMU_ERROR_INVALID_PARAMETER;
            goto errorOccurred;
         }
      }
      else{
         error = EMU_ERROR_RESOURCE_LOCKED;
         goto errorOccurred;
      }
   }

   if(newestBootableFile != -1){
      files[newestBootableFile].boot = true;
   }
   else{
      error = EMU_ERROR_INVALID_PARAMETER;
      goto errorOccurred;
   }

   //save the current data for the last program launched, or the standard device image if none where launched
   writeOutSaves();

   //its OK if these fail, the buffer will just be NULL, 0 if they do
   hasSaveRam = ramFile.open(QFile::ReadOnly | QFile::ExistingOnly);
   hasSaveSdCard = sdCardFile.open(QFile::ReadOnly | QFile::ExistingOnly);

   error = launcherLaunch(files, paths.length(), hasSaveRam ? (uint8_t*)ramFile.readAll().data() : NULL, hasSaveRam ? ramFile.size() : 0, hasSaveSdCard ? (uint8_t*)sdCardFile.readAll().data() : NULL, hasSaveSdCard ? sdCardFile.size() : 0);
   if(error != EMU_ERROR_NONE)
      goto errorOccurred;

   //its OK if these fail
   if(hasSaveRam)
      ramFile.close();
   if(hasSaveSdCard)
      sdCardFile.close();

   //everything worked, set output save files
   emuRamFilePath = mainPath + ".ram";
   emuSdCardFilePath = mainPath + ".sd.img";

   //need this goto because the emulator must be released before returning
   errorOccurred:

   if(!wasPaused)
      resume();

   return error;
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

   if(stateFile.open(QFile::ReadOnly | QFile::ExistingOnly)){
      if(emulatorLoadState((uint8_t*)stateFile.readAll().data(), stateFile.size()))
         error = EMU_ERROR_NONE;
      stateFile.close();

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

QVector<QString>& EmuWrapper::debugGetLogEntrys(){
   return debugStrings;
}

QVector<uint64_t>& EmuWrapper::debugGetDuplicateLogEntryCount(){
   return duplicateCallCount;
}

QString EmuWrapper::debugGetCpuRegisterString(){
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

uint32_t EmuWrapper::debugInstallApplication(const QString& path){
   bool wasPaused = isPaused();
   uint32_t error = EMU_ERROR_INVALID_PARAMETER;
   QFile appFile(path);

   if(!wasPaused)
      pause();

   if(appFile.open(QFile::ReadOnly | QFile::ExistingOnly)){
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

uint64_t EmuWrapper::debugGetEmulatorMemory(uint32_t address, uint8_t size){
   return flx68000ReadArbitraryMemory(address, size);
}
