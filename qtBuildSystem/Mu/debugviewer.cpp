#include "debugviewer.h"
#include "ui_debugviewer.h"

#include <QString>
#include <QDir>

#include <stdint.h>

#include "mainwindow.h"
#include "fileaccess.h"

extern "C" {
#include "src/m68k/m68k.h"
#include "src/hardwareRegisters.h"
#include "src/memoryAccess.h"
}


#define INVALID_NUMBER (int64_t)0x8000000000000000//the biggest negative 64bit number


int64_t getEmulatorMemorySafe(uint32_t address, uint8_t size){
   //until SPI and UART destructive reads are implemented all reads to mapped addresses are safe
   if(bankType[START_BANK(address)] != CHIP_NONE){
      uint16_t m68kSr = m68k_get_reg(NULL, M68K_REG_SR);
      m68k_set_reg(M68K_REG_SR, 0x2000);//prevent privilege violations
      switch(size){

         case 8:
            return m68k_read_memory_8(address);

         case 16:
            return m68k_read_memory_16(address);

         case 32:
            return m68k_read_memory_32(address);
      }
      m68k_set_reg(M68K_REG_SR, m68kSr);
   }
   return INVALID_NUMBER;
}


DebugViewer::DebugViewer(QWidget* parent) :
   QDialog(parent),
   ui(new Ui::DebugViewer){
   ui->setupUi(this);

   bitsPerEntry = 8;
   debugRadioButtonHandler();

#if !defined(EMU_DEBUG) || !defined(EMU_CUSTOM_DEBUG_LOG_HANDLER)
   ui->debugPrintDebugLogs->hide();
   ui->debugEraseDebugLogs->hide();
#endif
}

DebugViewer::~DebugViewer(){
   delete ui;
}

int64_t DebugViewer::numberFromString(QString str, bool negativeAllowed){
   int64_t value;
   bool validNumber;

   if(str.length() > 2 && (str[0] == "0") && (str[1].toLower() == "x")){
      //hex number
      str.remove(0, 2);
      value = str.toLongLong(&validNumber, 16);
   }
   else{
      //non hex number
      value = str.toLongLong(&validNumber, 10);
   }

   if(!validNumber || (!negativeAllowed && value < 0))
      return INVALID_NUMBER;
   return value;
}

QString DebugViewer::stringFromNumber(int64_t number, bool hex, uint32_t forcedZeros){
   if(hex){
      QString hexString;
      hexString += QString::number(number, 16).toUpper();
      while(hexString.length() < (int)forcedZeros)hexString.push_front("0");
      hexString.push_front("0x");
      return hexString;
   }

   return QString::number(number, 10);
}

void DebugViewer::debugRadioButtonHandler(){
   switch(bitsPerEntry){

      case 8:
         ui->debug8Bit->setDown(true);
         ui->debug16Bit->setDown(false);
         ui->debug32Bit->setDown(false);
         break;

      case 16:
         ui->debug8Bit->setDown(false);
         ui->debug16Bit->setDown(true);
         ui->debug32Bit->setDown(false);
         break;

      case 32:
         ui->debug8Bit->setDown(false);
         ui->debug16Bit->setDown(false);
         ui->debug32Bit->setDown(true);
         break;
   }
}

void DebugViewer::on_debugGetHexValues_clicked(){
   int64_t address = numberFromString(ui->debugAddress->text(), false/*negative allowed*/);
   int64_t length = numberFromString(ui->debugLength->text(), false/*negative allowed*/);
   uint8_t bits = bitsPerEntry;

   ui->debugValueList->clear();

   if(address != INVALID_NUMBER && length != INVALID_NUMBER && length != 0 && address + bits / 8 * length - 1 <= 0xFFFFFFFF){
      for(int64_t count = 0; count < length; count++){
         int64_t data = getEmulatorMemorySafe(address, bits);
         QString value;
         value += stringFromNumber(address, true, 8);
         value += ":";
         if(data != INVALID_NUMBER)
            value += stringFromNumber(data, true, bits / 8 * 2);
         else
            value += "Unsafe Access";
         ui->debugValueList->addItem(value);
         address += bits / 8;
      }
   }
   else{
      ui->debugValueList->addItem("Invalid Parameters");
   }
}

void DebugViewer::on_debug8Bit_clicked(){
    bitsPerEntry = 8;
    debugRadioButtonHandler();
}

void DebugViewer::on_debug16Bit_clicked(){
   bitsPerEntry = 16;
   debugRadioButtonHandler();
}

void DebugViewer::on_debug32Bit_clicked(){
   bitsPerEntry = 32;
   debugRadioButtonHandler();
}

void DebugViewer::on_debugDump_clicked(){
   QString fileOut;
   std::string fileData;//if a QString is used a '\0' will be appended to every character
   QString fileName = ui->debugFilePath->text();
   QString filePath = settings.value("resourceDirectory", "").toString() + "/debugDumps";
   QDir location = filePath;

   if(!location.exists())
      location.mkpath(".");

   for(uint64_t index = 0; index < ui->debugValueList->count(); index++){
      fileOut += ui->debugValueList->item(index)->text();
      fileOut += '\n';
   }

   fileData = fileOut.toStdString();
   setFileBuffer(filePath + "/" + fileName, (uint8_t*)fileData.c_str(), fileData.length());
}

void DebugViewer::on_debugShowRegisters_clicked(){
   ui->debugValueList->clear();
   for(uint8_t aRegs = M68K_REG_A0; aRegs <= M68K_REG_A7; aRegs++)
      ui->debugValueList->addItem("A" + stringFromNumber(aRegs - M68K_REG_A0, false, 0) + ":" + stringFromNumber(m68k_get_reg(NULL, (m68k_register_t)aRegs), true, 8));
   for(uint8_t dRegs = M68K_REG_D0; dRegs <= M68K_REG_D7; dRegs++)
      ui->debugValueList->addItem("D" + stringFromNumber(dRegs - M68K_REG_D0, false, 0) + ":" + stringFromNumber(m68k_get_reg(NULL, (m68k_register_t)dRegs), true, 8));
   ui->debugValueList->addItem("SP:" + stringFromNumber(m68k_get_reg(NULL, M68K_REG_SP), true, 8));
   ui->debugValueList->addItem("PC:" + stringFromNumber(m68k_get_reg(NULL, M68K_REG_PC), true, 8));
   ui->debugValueList->addItem("SR:" + stringFromNumber(m68k_get_reg(NULL, M68K_REG_SR), true, 4));
}

void DebugViewer::on_debugPrintDebugLogs_clicked(){
#if defined(EMU_DEBUG) && defined(EMU_CUSTOM_DEBUG_LOG_HANDLER)
   int64_t length = numberFromString(ui->debugLength->text(), true/*negative allowed*/);

   ui->debugValueList->clear();
   if(length != INVALID_NUMBER && qAbs(length) < debugStrings.size()){
      if(length < 0){
         for(uint64_t stringNum = debugStrings.size() + length; stringNum < debugStrings.size(); stringNum++)
            ui->debugValueList->addItem(QString::fromStdString(debugStrings[stringNum] + "(printed " + std::to_string(duplicateCallCount[stringNum]) + " times)"));
      }
      else{
         for(uint64_t stringNum = 0; stringNum < length; stringNum++)
            ui->debugValueList->addItem(QString::fromStdString(debugStrings[stringNum] + "(printed " + std::to_string(duplicateCallCount[stringNum]) + " times)"));
      }
   }
   else{
      for(uint64_t stringNum = 0; stringNum < debugStrings.size(); stringNum++)
         ui->debugValueList->addItem(QString::fromStdString(debugStrings[stringNum] + "(printed " + std::to_string(duplicateCallCount[stringNum]) + " times)"));
   }
#endif
}

void DebugViewer::on_debugEraseDebugLogs_clicked(){
#if defined(EMU_DEBUG) && defined(EMU_CUSTOM_DEBUG_LOG_HANDLER)
   debugStrings.clear();
   duplicateCallCount.clear();
#endif
}
