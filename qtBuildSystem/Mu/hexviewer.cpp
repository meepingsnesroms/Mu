#include "hexviewer.h"
#include "ui_hexviewer.h"

#include <QString>


#define INVALID_NUMBER (int64_t)0x8000000000000000//the biggest negative 64bit number


uint8_t bytesPerEntry;


HexViewer::HexViewer(QWidget *parent) :
   QDialog(parent),
   ui(new Ui::HexViewer){
   ui->setupUi(this);

   ui->tabWidget->setTabText(0, "Hex");
   ui->tabWidget->setTabText(1, "Graphics");

   bytesPerEntry = 1;
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
      hexString += QString::number(number, 16);
      while(hexString.length() < (int)forcedZeros)hexString.push_front("0");
      hexString.push_front("0x");
      return hexString;
   }

   return QString::number(number, 10);
}

void HexViewer::hexRadioButtonHandler(){
   switch(bytesPerEntry){

      case 1:
         ui->hex8Bit->setDown(true);
         ui->hex16Bit->setDown(false);
         ui->hex32Bit->setDown(false);
         break;

      case 2:
         ui->hex8Bit->setDown(false);
         ui->hex16Bit->setDown(true);
         ui->hex32Bit->setDown(false);
         break;

      case 4:
         ui->hex8Bit->setDown(false);
         ui->hex16Bit->setDown(false);
         ui->hex32Bit->setDown(true);
         break;
   }
}

void HexViewer::on_hexUpdate_clicked(){
   int64_t address = numberFromString(ui->hexAddress->text(), false/*negative allowed*/);
   int64_t length = numberFromString(ui->hexLength->text(), false/*negative allowed*/);
   uint8_t bytes = bytesPerEntry;

   ui->hexValueList->clear();

   if(address != INVALID_NUMBER && length != INVALID_NUMBER && address + bytes * length <= 0xFFFFFFFF){
      for(int64_t count = 0; count < length; count++){
         QString value;
         value += stringFromNumber(address, true, 8);
         value += ":";
         value += "emu memory access not added yet";
         ui->hexValueList->addItem(value);
         address += bytes;
      }
   }
   else{
      ui->hexValueList->addItem("Invalid Parameters");
   }
}

void HexViewer::on_hex8Bit_clicked(){
    bytesPerEntry = 1;
    hexRadioButtonHandler();
}

void HexViewer::on_hex16Bit_clicked(){
   bytesPerEntry = 2;
   hexRadioButtonHandler();
}

void HexViewer::on_hex32Bit_clicked(){
   bytesPerEntry = 4;
   hexRadioButtonHandler();
}
