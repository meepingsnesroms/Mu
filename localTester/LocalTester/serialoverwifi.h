#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>

class SerialOverWifi : public QObject
{
   Q_OBJECT

private:
   //fdf

public:
   SerialOverWifi(QString location, uint16_t port, QByteArray certificate);
   ~SerialOverWifi();

   Q_INVOKABLE unsigned int bytesAvailable();
   Q_INVOKABLE void flushFifo();
   Q_INVOKABLE void transmitUint8(unsigned int data);
   Q_INVOKABLE unsigned int receiveUint8();

signals:

public slots:
};
