#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdexcept>

#include "serialportio.h"


SerialPortIO::SerialPortIO(QString serialDeviceFilePath){
   deviceFilePath = serialDeviceFilePath;
   deviceFile = open(serialDeviceFilePath.toStdString().c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
   if(deviceFile < 0)
      throw std::invalid_argument("Cant open file!");
}

SerialPortIO::~SerialPortIO(){
   deviceFilePath.clear();
   close(deviceFile);
}

void SerialPortIO::testJsAttachment(){
   printf("C++ executed through javascript!\n");
}

void SerialPortIO::transmitUint8(uint8_t data){
   write(deviceFile, &data, 1);
}

uint8_t SerialPortIO::receiveUint8(){
   uint8_t data = 0x00;
   read(deviceFile, &data, 1);
   return data;
}
