#pragma once

#include <QObject>

class SerialPortIO : public QObject
{
   Q_OBJECT

private:
   QString deviceFilePath;
   int     deviceFile;

public:
   SerialPortIO(QString serialDeviceFilePath);
   ~SerialPortIO();

   Q_INVOKABLE unsigned int bytesAvailable();
   Q_INVOKABLE void flushFifo();
   Q_INVOKABLE void transmitUint8(unsigned int data);
   Q_INVOKABLE unsigned int receiveUint8();

signals:

public slots:
};
