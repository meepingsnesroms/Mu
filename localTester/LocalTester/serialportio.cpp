#include <QObject>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdexcept>

#include "serialportio.h"


SerialPortIO::SerialPortIO(QString serialDeviceFilePath) : QObject(nullptr){
   deviceFilePath = serialDeviceFilePath;
   deviceFile = open(serialDeviceFilePath.toStdString().c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
   if(deviceFile < 0)
      throw std::invalid_argument("Cant open file!");
}

SerialPortIO::~SerialPortIO(){
   deviceFilePath.clear();
   close(deviceFile);
}

unsigned int SerialPortIO::bytesAvailable(){
   int bytes;
   ioctl(deviceFile, FIONREAD, &bytes);
   return bytes;
}

void SerialPortIO::flushFifo(){
   uint8_t data;
   while(bytesAvailable())
      read(deviceFile, &data, 1);
}

void SerialPortIO::transmitUint8(unsigned int data){
   uint8_t realData = data;
   write(deviceFile, &realData, 1);
}

unsigned int SerialPortIO::receiveUint8(){
   uint8_t data = 0x00;
   read(deviceFile, &data, 1);
   return data;
}
