#include "hexviewer.h"
#include "ui_hexviewer.h"

#include <QString>

extern "C" {
#include "src/m68k/m68k.h"
#include "src/hardwareRegisters.h"
#include "src/memoryAccess.h"
}


#define INVALID_NUMBER (int64_t)0x8000000000000000//the biggest negative 64bit number


uint32_t getEmulatorMemorySafe(uint32_t address, uint8_t size){
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
   return 0x00000000;
}


HexViewer::HexViewer(QWidget *parent) :
   QDialog(parent),
   ui(new Ui::HexViewer){
   ui->setupUi(this);

   ui->tabWidget->setCurrentIndex(0);

   ui->tabWidget->setTabText(0, "Hex");
   ui->tabWidget->setTabText(1, "Graphics");

   bitsPerEntry = 8;
   hexRadioButtonHandler();
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
         QString value;
         value += stringFromNumber(address, true, 8);
         value += ":";
         value += stringFromNumber(getEmulatorMemorySafe(address, bits), true, bits / 8 * 2);
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
