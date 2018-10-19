#include "debugviewer.h"
#include "ui_debugviewer.h"

#include <QString>
#include <QFile>
#include <QDir>

#include <vector>
#include <stdint.h>

#include "mainwindow.h"
#include "emuwrapper.h"


DebugViewer::DebugViewer(QWidget* parent) :
   QDialog(parent),
   ui(new Ui::DebugViewer){
   ui->setupUi(this);

   bitsPerEntry = 8;
   debugRadioButtonHandler();
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
      return INT64_MIN;
   return value;
}

QString DebugViewer::stringFromNumber(int64_t number, bool hex, uint32_t forcedZeros){
   QString numString;

   if(hex){
      numString += QString::number(number, 16).toUpper();
      while(numString.length() < (int)forcedZeros)numString.push_front("0");
      numString.push_front("0x");
   }
   else{
      numString += QString::number(qAbs(number), 10);
      while(numString.length() < (int)forcedZeros)numString.push_front("0");
      if(number < 0)
         numString.push_front("-");
   }

   return numString;
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
   EmuWrapper& emu = ((MainWindow*)parentWidget())->emu;
   int64_t address = numberFromString(ui->debugAddress->text(), false/*negative allowed*/);
   int64_t length = numberFromString(ui->debugLength->text(), false/*negative allowed*/);
   uint8_t bits = bitsPerEntry;

   ui->debugValueList->clear();

   if(address != INT64_MIN && length != INT64_MIN && length != 0 && address + bits / 8 * length - 1 <= 0xFFFFFFFF){
      for(int64_t count = 0; count < length; count++){
         uint64_t data = emu.getEmulatorMemory(address, bits);
         QString value;
         value += stringFromNumber(address, true, 8);
         value += ":";
         if(data != UINT64_MAX)
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
   QString fileBuffer;
   QFile fileOut(((MainWindow*)parentWidget())->settings->value("resourceDirectory", "").toString() + "/debugDumps/" + ui->debugFilePath->text());

   for(int index = 0; index < ui->debugValueList->count(); index++){
      fileBuffer += ui->debugValueList->item(index)->text();
      fileBuffer += '\n';
   }

   if(fileOut.open(QFile::ReadWrite)){
      //if a QString is used for output a '\0' will be appended to every character
      fileOut.write(fileBuffer.toStdString().c_str());
      fileOut.close();
   }
}

void DebugViewer::on_debugPrintAudioBuffer_clicked(){
   EmuWrapper& emu = ((MainWindow*)parentWidget())->emu;
   const int16_t* samples = emu.getAudioSamples();

   ui->debugValueList->clear();
   for(uint32_t index = 0; index < AUDIO_SAMPLES_PER_FRAME * 2; index++)
      ui->debugValueList->addItem(stringFromNumber(index, false, 5) + ":" + stringFromNumber(samples[index], false, 5));
}

void DebugViewer::on_debugShowRegisters_clicked(){
   std::vector<uint32_t> registers = ((MainWindow*)parentWidget())->emu.getCpuRegisters();

   ui->debugValueList->clear();
   for(uint8_t dRegs = 0; dRegs <= 7; dRegs++)
      ui->debugValueList->addItem("D" + stringFromNumber(dRegs, false, 0) + ":" + stringFromNumber(registers[dRegs], true, 8));
   for(uint8_t aRegs = 0; aRegs <= 7; aRegs++)
      ui->debugValueList->addItem("A" + stringFromNumber(aRegs, false, 0) + ":" + stringFromNumber(registers[8 + aRegs], true, 8));
   ui->debugValueList->addItem("SP:" + stringFromNumber(registers[15], true, 8));
   ui->debugValueList->addItem("PC:" + stringFromNumber(registers[16], true, 8));
   ui->debugValueList->addItem("SR:" + stringFromNumber(registers[17], true, 4));
}

void DebugViewer::on_debugPrintDebugLogs_clicked(){
   EmuWrapper& emu = ((MainWindow*)parentWidget())->emu;
   std::vector<QString>& debugStrings = emu.getDebugStrings();
   std::vector<uint64_t>& duplicateCallCount = emu.getDuplicateCallCount();
   int64_t length = numberFromString(ui->debugLength->text(), true/*negative allowed*/);

   ui->debugValueList->clear();
   if(length != INT64_MIN && qAbs(length) < debugStrings.size()){
      if(length < 0){
         for(uint64_t stringNum = debugStrings.size() + length; stringNum < debugStrings.size(); stringNum++)
            ui->debugValueList->addItem(debugStrings[stringNum] + "(printed " + QString::number(duplicateCallCount[stringNum]) + " times)");
      }
      else{
         for(uint64_t stringNum = 0; stringNum < length; stringNum++)
            ui->debugValueList->addItem(debugStrings[stringNum] + "(printed " + QString::number(duplicateCallCount[stringNum]) + " times)");
      }
   }
   else{
      for(uint64_t stringNum = 0; stringNum < debugStrings.size(); stringNum++)
         ui->debugValueList->addItem(debugStrings[stringNum] + "(printed " + QString::number(duplicateCallCount[stringNum]) + " times)");
   }
}

void DebugViewer::on_debugEraseDebugLogs_clicked(){
   EmuWrapper& emu = ((MainWindow*)parentWidget())->emu;

   emu.getDebugStrings().clear();
   emu.getDuplicateCallCount().clear();
}
