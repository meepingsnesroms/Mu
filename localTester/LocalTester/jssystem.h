#pragma once

#include <QObject>
#include <QString>
#include <QJSValue>

#include <mutex>
#include <vector>
#include <stdint.h>

class JSSystem : public QObject
{
   Q_OBJECT

public:
   std::mutex            systemData;
   bool                  framebufferRender;
   uint16_t              framebufferWidth;
   uint16_t              framebufferHeight;
   std::vector<uint16_t> framebufferPixels;

   explicit JSSystem(QObject* parent = nullptr);

   Q_INVOKABLE void testJsAttachment(QString str);
   Q_INVOKABLE void uSleep(uint32_t uSeconds);
   Q_INVOKABLE void setFramebufferSize(uint32_t w, uint32_t h);
   Q_INVOKABLE void setFramebuffer(QJSValue framebuffer);

signals:

public slots:
};
