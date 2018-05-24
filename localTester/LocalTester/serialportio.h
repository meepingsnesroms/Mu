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

   Q_INVOKABLE void transmitUint8(uint8_t data);
   Q_INVOKABLE uint8_t receiveUint8();

signals:

public slots:
};
