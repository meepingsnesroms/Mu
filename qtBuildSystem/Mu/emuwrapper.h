#pragma once

#include <QPixmap>
#include <QVector>
#include <QString>
#include <QByteArray>

#include <thread>
#include <atomic>
#include <stdint.h>

#include "../../src/emulator.h"

class EmuWrapper{
private:
   EmuWrapper(const EmuWrapper&) = delete;//non construction copyable
   EmuWrapper& operator=(const EmuWrapper&) = delete;//non copyable

   bool              emuInited;
   std::thread       emuThread;
   std::atomic<bool> emuThreadJoin;
   std::atomic<bool> emuRunning;
   std::atomic<bool> emuPaused;
   std::atomic<bool> emuNewFrameReady;
   QString           emuOsName;
   QString           emuRamFilePath;
   QString           emuSdCardFilePath;
   QString           emuSaveStatePath;
   input_t           emuInput;

   void emuThreadRun();
   void writeOutSaves();

public:
   enum{
      BUTTON_UP = 0,
      BUTTON_DOWN,
      BUTTON_LEFT,
      BUTTON_RIGHT,
      BUTTON_CENTER,
      BUTTON_CALENDAR,
      BUTTON_ADDRESS,
      BUTTON_TODO,
      BUTTON_NOTES,
      BUTTON_POWER,
      BUTTON_TOTAL_COUNT
   };

   EmuWrapper();
   ~EmuWrapper();

   uint32_t init(const QString& assetPath, bool useOs5, uint32_t features = FEATURE_ACCURATE, bool fastBoot = false);
   void exit();
   void pause();
   void resume();
   void reset(bool hard);
   uint32_t bootFromFileOrDirectory(const QString& mainPath);
   uint32_t installApplication(const QString& path);
   const QString& getStatePath() const{return emuSaveStatePath;}//needed for looking up state pictures in the GUI
   uint32_t saveState(const QString& name);
   uint32_t loadState(const QString& name);
   bool isInited() const{return emuInited;}
   bool isRunning() const{return emuRunning;}
   bool isPaused() const{return emuPaused;}
   void setPenValue(float x, float y, bool touched);
   void setKeyValue(uint8_t key, bool pressed);

   uint16_t screenWidth() const{return palmFramebufferWidth;}
   uint16_t screenHeight() const{return palmFramebufferHeight;}
   bool newFrameReady() const{return emuNewFrameReady;}
   void frameHandled(){emuNewFrameReady = false;}

   //calling these while newFrameReady() == false is undefined behavior, the other thread may be writing to them
   const QPixmap getFramebuffer(){return QPixmap::fromImage(QImage((uchar*)palmFramebuffer, palmFramebufferWidth, palmFramebufferHeight, palmFramebufferWidth * sizeof(uint16_t), QImage::Format_RGB16));}
   const int16_t* getAudioSamples() const{return palmAudio;}
   bool getPowerButtonLed() const{return palmMisc.powerButtonLed;}

   QVector<QString>& debugGetLogEntrys();
   QVector<uint64_t>& debugGetDuplicateLogEntryCount();
   QString debugGetCpuRegisterString();
   uint64_t debugGetEmulatorMemory(uint32_t address, uint8_t size);
};
