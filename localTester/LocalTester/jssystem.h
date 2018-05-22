#pragma once

#include <QObject>
#include <QString>

#include <vector>
#include <stdint.h>

class JSSystem : public QObject
{
   Q_OBJECT

private:
   uint16_t framebufferWidth;
   uint16_t framebufferHeight;
   std::vector<uint16_t> framebufferPixels;
   //void (*updateFramebuffer)();

public:
   explicit JSSystem(QObject* parent = nullptr);

   //void setFramebufferUpdater(void (*newFramebufferUpdater)());

   Q_INVOKABLE void testJsAttachment(QString str);
   Q_INVOKABLE void uSleep(uint32_t uSeconds);
   Q_INVOKABLE void setFramebufferSize(uint32_t w, uint32_t h);
   Q_INVOKABLE void setFramebufferPixel(uint32_t x, uint32_t y, uint32_t color);

signals:

public slots:
};
