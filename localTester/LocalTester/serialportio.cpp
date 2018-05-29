#include <QObject>

#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdexcept>

#include "serialportio.h"


SerialPortIO::SerialPortIO(QString serialDeviceFilePath) : QObject(nullptr){
   deviceFilePath = serialDeviceFilePath;
   deviceFile = open(serialDeviceFilePath.toStdString().c_str(), O_RDWR | O_NOCTTY);
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

void SerialPortIO::transmitUint8(unsigned int data){
   uint8_t realData = data;
   send(deviceFile, &realData, 1, MSG_DONTWAIT);
}

unsigned int SerialPortIO::receiveUint8(){
   uint8_t data = 0x00;
   recv(deviceFile, &data, 1, MSG_DONTWAIT);
   return data;
}
