#pragma once

#include <QPixmap>
#include <QString>
#include <QByteArray>

#include <thread>
#include <atomic>
#include <vector>
#include <stdint.h>

#include "src/emulator.h"

class EmuWrapper{
private:
   EmuWrapper(const EmuWrapper&) = delete;//non construction copyable
   EmuWrapper& operator=(const EmuWrapper&) = delete;//non copyable

   bool              emuInited;
   std::thread       emuThread;
   std::atomic<bool> emuThreadJoin;
   std::atomic<bool> emuRunning;
   std::atomic<bool> emuPaused;
   uint16_t          emuVideoWidth;
   uint16_t          emuVideoHeight;
   std::atomic<bool> emuNewFrameReady;
   QString           emuRamFilePath;
   QString           emuSdCardFilePath;

   void emuThreadRun();

public:
   input_t emuInput;

   EmuWrapper();
   ~EmuWrapper();

   uint32_t init(const QString& romPath, const QString& bootloaderPath = "", const QString& ramPath = "", const QString& sdCardPath = "", uint32_t features = FEATURE_ACCURATE);
   void exit();
   void pause();
   void resume();
   uint32_t saveState(const QString& path);
   uint32_t loadState(const QString& path);
   bool isInited() const{return emuInited;}
   bool isRunning() const{return emuRunning;}
   bool isPaused() const{return emuPaused;}

   uint32_t installApplication(QString path);

   std::vector<QString>& getDebugStrings();
   std::vector<uint64_t>& getDuplicateCallCount();
   std::vector<uint32_t> getCpuRegisters();

   uint16_t screenWidth() const{return emuVideoWidth;}
   uint16_t screenHeight() const{return emuVideoHeight;}
   bool newFrameReady() const{return emuNewFrameReady;}
   void frameHandled(){emuNewFrameReady = false;}

   //calling these while newFrameReady() == false is undefined behavior, the other thread may be writing to them
   const QPixmap getFramebuffer(){return QPixmap::fromImage(QImage((uchar*)(emuVideoWidth == 320 ? palmExtendedFramebuffer : palmFramebuffer), emuVideoWidth, emuVideoHeight, emuVideoWidth * sizeof(uint16_t), QImage::Format_RGB16));}
   const int16_t* getAudioSamples() const{return palmAudio;}
   bool getPowerButtonLed() const{return palmMisc.powerButtonLed;}

   uint64_t getEmulatorMemory(uint32_t address, uint8_t size);
};
