#include <QObject>
#include <QJSValue>

#include <chrono>
#include <thread>
#include <stdint.h>

#include "jssystem.h"


JSSystem::JSSystem(QObject* parent) : QObject(parent){
   framebufferWidth = 0;
   framebufferHeight = 0;
   framebufferRender = false;
}

void JSSystem::testJsAttachment(QString str){
   printf("C++ executed through javascript!, str:%s\n", str.toStdString().c_str());
}

void JSSystem::uSleep(uint32_t uSeconds){
   std::this_thread::sleep_for(std::chrono::microseconds(uSeconds));
}

void JSSystem::setFramebufferSize(uint32_t w, uint32_t h){
   systemData.lock();
   framebufferPixels.resize(w * h);
   framebufferWidth = w;
   framebufferHeight = h;
   systemData.unlock();
}

void JSSystem::setFramebuffer(QJSValue framebuffer){
   systemData.lock();
   for(uint16_t y = 0; y < framebufferHeight; y++)
      for(uint16_t x = 0; x < framebufferWidth; x++)
         framebufferPixels[y * framebufferWidth + x] = framebuffer.property(y * framebufferWidth + x);
   framebufferRender = true;
   systemData.unlock();
}
