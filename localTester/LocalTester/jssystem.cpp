#include <chrono>
#include <thread>
#include <stdint.h>

#include "jssystem.h"


JSSystem::JSSystem(QObject* parent) : QObject(parent){
   framebufferWidth = 0;
   framebufferHeight = 0;
}

void JSSystem::testJsAttachment(QString str){
   printf("C++ executed through javascript!, str:%s\n", str.toStdString().c_str());
}

void JSSystem::uSleep(uint32_t uSeconds){
   std::this_thread::sleep_for(std::chrono::microseconds(uSeconds));
}

void JSSystem::setFramebufferSize(uint32_t w, uint32_t h){
   framebufferPixels.resize(w * h);
   framebufferWidth = w;
   framebufferHeight = h;
}

void JSSystem::setFramebufferPixel(uint32_t x, uint32_t y, uint32_t color){
   framebufferPixels[y * framebufferWidth + x] = color;
}

/*
void JSSystem::renderFramebuffer(){

}
*/
