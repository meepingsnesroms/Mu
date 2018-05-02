#include "hexviewer.h"
#include "ui_hexviewer.h"

#include <QString>
#include <QDir>

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


HexViewer::HexViewer(QWidget *parent) :
   QDialog(parent),
   ui(new Ui::HexViewer){
   ui->setupUi(this);

   bitsPerEntry = 8;
   hexRadioButtonHandler();

#if !defined(EMU_DEBUG) || !defined(EMU_CUSTOM_DEBUG_LOG_HANDLER)
   ui->hexPrintDebugLogs->hide();
   ui->hexEraseDebugLogs->hide();
#endif
}

HexViewer::~HexViewer(){
   delete ui;
}

int64_t HexViewer::numberFromString(QString str, bool negativeAllowed){
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

QString HexViewer::stringFromNumber(int64_t number, bool hex, uint32_t forcedZeros){
   if(hex){
      QString hexString;
      hexString += QString::number(number, 16).toUpper();
      while(hexString.length() < (int)forcedZeros)hexString.push_front("0");
      hexString.push_front("0x");
      return hexString;
   }

   return QString::number(number, 10);
}

void HexViewer::hexRadioButtonHandler(){
   switch(bitsPerEntry){

      case 8:
         ui->hex8Bit->setDown(true);
         ui->hex16Bit->setDown(false);
         ui->hex32Bit->setDown(false);
         break;

      case 16:
         ui->hex8Bit->setDown(false);
         ui->hex16Bit->setDown(true);
         ui->hex32Bit->setDown(false);
         break;

      case 32:
         ui->hex8Bit->setDown(false);
         ui->hex16Bit->setDown(false);
         ui->hex32Bit->setDown(true);
         break;
   }
}

void HexViewer::on_hexUpdate_clicked(){
   int64_t address = numberFromString(ui->hexAddress->text(), false/*negative allowed*/);
   int64_t length = numberFromString(ui->hexLength->text(), false/*negative allowed*/);
   uint8_t bits = bitsPerEntry;

   ui->hexValueList->clear();

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
         ui->hexValueList->addItem(value);
         address += bits / 8;
      }
   }
   else{
      ui->hexValueList->addItem("Invalid Parameters");
   }
}

void HexViewer::on_hex8Bit_clicked(){
    bitsPerEntry = 8;
    hexRadioButtonHandler();
}

void HexViewer::on_hex16Bit_clicked(){
   bitsPerEntry = 16;
   hexRadioButtonHandler();
}

void HexViewer::on_hex32Bit_clicked(){
   bitsPerEntry = 32;
   hexRadioButtonHandler();
}

void HexViewer::on_hexDump_clicked(){
   int64_t address = numberFromString(ui->hexAddress->text(), false/*negative allowed*/);
   int64_t length = numberFromString(ui->hexLength->text(), false/*negative allowed*/);
   uint8_t bits = bitsPerEntry;
   QString fileName = ui->hexFilePath->text();
   QString filePath = settings.value("resourceDirectory", "").toString() + "/hexDumps";
   QDir location = filePath;

   if(!location.exists())
      location.mkpath(".");

   if(validFilePath(filePath + "/" + fileName) && address != INVALID_NUMBER && length != INVALID_NUMBER && length != 0 && address + bits / 8 * length - 1 <= 0xFFFFFFFF){
      length *= bits / 8;
      uint8_t* dumpBuffer = new uint8_t[length];
      for(int64_t count = 0; count < length; count++)
         dumpBuffer[count] = getEmulatorMemorySafe(address + count, 8);
      setFileBuffer(filePath + "/" + fileName, dumpBuffer, length);
      delete[] dumpBuffer;
   }
}

void HexViewer::on_hexShowRegisters_clicked(){
   ui->hexValueList->clear();
   for(uint8_t aRegs = M68K_REG_A0; aRegs <= M68K_REG_A7; aRegs++)
      ui->hexValueList->addItem("A" + stringFromNumber(aRegs - M68K_REG_A0, false, 0) + ":" + stringFromNumber(m68k_get_reg(NULL, (m68k_register_t)aRegs), true, 8));
   for(uint8_t dRegs = M68K_REG_D0; dRegs <= M68K_REG_D7; dRegs++)
      ui->hexValueList->addItem("D" + stringFromNumber(dRegs - M68K_REG_D0, false, 0) + ":" + stringFromNumber(m68k_get_reg(NULL, (m68k_register_t)dRegs), true, 8));
   ui->hexValueList->addItem("SP:" + stringFromNumber(m68k_get_reg(NULL, M68K_REG_SP), true, 8));
   ui->hexValueList->addItem("PC:" + stringFromNumber(m68k_get_reg(NULL, M68K_REG_PC), true, 8));
   ui->hexValueList->addItem("SR:" + stringFromNumber(m68k_get_reg(NULL, M68K_REG_SR), true, 4));
}

void HexViewer::on_hexPrintDebugLogs_clicked(){
#if defined(EMU_DEBUG) && defined(EMU_CUSTOM_DEBUG_LOG_HANDLER)
   int64_t length = numberFromString(ui->hexLength->text(), true/*negative allowed*/);

   ui->hexValueList->clear();
   if(length != INVALID_NUMBER && qAbs(length) < debugStrings.size()){
      if(length < 0){
         for(uint64_t stringNum = debugStrings.size() + length; stringNum < debugStrings.size(); stringNum++)
            ui->hexValueList->addItem(QString::fromStdString(debugStrings[stringNum] + "(printed " + std::to_string(duplicateCallCount[stringNum]) + " times)"));
      }
      else{
         for(uint64_t stringNum = 0; stringNum < length; stringNum++)
            ui->hexValueList->addItem(QString::fromStdString(debugStrings[stringNum] + "(printed " + std::to_string(duplicateCallCount[stringNum]) + " times)"));
      }
   }
   else{
      for(uint64_t stringNum = 0; stringNum < debugStrings.size(); stringNum++)
         ui->hexValueList->addItem(QString::fromStdString(debugStrings[stringNum] + "(printed " + std::to_string(duplicateCallCount[stringNum]) + " times)"));
   }
#endif
}

void HexViewer::on_hexEraseDebugLogs_clicked(){
#if defined(EMU_DEBUG) && defined(EMU_CUSTOM_DEBUG_LOG_HANDLER)
   debugStrings.clear();
   duplicateCallCount.clear();
#endif
}
