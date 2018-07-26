#pragma once

#include <QPixmap>
#include <QString>

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
   std::atomic<bool> emuDebugEvent;
   uint16_t          emuVideoWidth;
   uint16_t          emuVideoHeight;
   std::atomic<bool> emuNewFrameReady;
   uint16_t*         emuDoubleBuffer;

   void emuThreadRun();

public:
   input_t emuInput;

   EmuWrapper();
   ~EmuWrapper();

   uint32_t init(QString romPath, QString bootloaderPath, uint32_t features);
   void exit();
   void pause();
   void resume();
   bool isInited() const{return emuInited;}
   bool isRunning() const{return emuRunning;}
   bool isPaused() const{return emuPaused;}

   uint32_t installApplication(QString path);

   std::vector<QString>& getDebugStrings();
   std::vector<uint64_t>& getDuplicateCallCount();
   std::vector<uint32_t> getCpuRegisters();
   bool debugEventOccured() const{return emuDebugEvent;}
   void clearDebugEvent(){emuDebugEvent = false;}

   uint16_t screenWidth() const{return emuVideoWidth;}
   uint16_t screenHeight() const{return emuVideoHeight;}
   bool newFrameReady() const{return emuNewFrameReady;}
   QPixmap getFramebuffer();
   bool getPowerButtonLed();

   uint64_t getEmulatorMemory(uint32_t address, uint8_t size);
};
