#include <QObject>
#include <QString>
#include <QByteArray>

#include <stdint.h>
#include <stdexcept>

#include "serialoverwifi.h"


SerialOverWifi::SerialOverWifi(QString location, uint16_t port, QByteArray certificate) : QObject(nullptr){
   throw std::invalid_argument("Cant open connection!");
}

SerialOverWifi::~SerialOverWifi(){

}

unsigned int SerialOverWifi::bytesAvailable(){
   int bytes = 0;

   return bytes;
}

void SerialOverWifi::flushFifo(){

}

void SerialOverWifi::transmitUint8(unsigned int data){

}

unsigned int SerialOverWifi::receiveUint8(){
   uint8_t data = 0x00;

   return data;
}
