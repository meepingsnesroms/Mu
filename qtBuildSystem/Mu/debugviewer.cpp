#include "debugviewer.h"
#include "ui_debugviewer.h"

#include <QVector>
#include <QString>
#include <QFile>
#include <QDir>

#include <stdint.h>
#include <stdio.h>

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

void DebugViewer::debugRadioButtonHandler(){
   switch(bitsPerEntry){
      case 8:
         ui->debug8Bit->setChecked(true);
         ui->debug16Bit->setChecked(false);
         ui->debug32Bit->setChecked(false);
         break;

      case 16:
         ui->debug8Bit->setChecked(false);
         ui->debug16Bit->setChecked(true);
         ui->debug32Bit->setChecked(false);
         break;

      case 32:
         ui->debug8Bit->setChecked(false);
         ui->debug16Bit->setChecked(false);
         ui->debug32Bit->setChecked(true);
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
         uint64_t data = emu.debugGetEmulatorMemory(address, bits);
         QString value;
         value += QString::asprintf("0x%08X", (uint32_t)address);
         value += ":";
         if(data != UINT64_MAX)
            value += QString::asprintf("0x%0*X", bits / 8 * 2, (uint32_t)data);
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

void DebugViewer::on_debugDecompile_clicked(){
   EmuWrapper& emu = ((MainWindow*)parentWidget())->emu;
   int64_t address = numberFromString(ui->debugAddress->text(), true/*negative not allowed*/);
   int64_t length = numberFromString(ui->debugLength->text(), true/*negative not allowed*/);

   ui->debugValueList->clear();

   if(address != INT64_MIN && length != INT64_MIN && length != 0)
      ui->debugValueList->addItems(emu.debugDisassemble(address, length).split('\n'));
   else
      ui->debugValueList->addItem("Invalid Parameters");
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

void DebugViewer::on_debugDumpToFile_clicked(){
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

void DebugViewer::on_debugDumpToTerminal_clicked(){
   for(int index = 0; index < ui->debugValueList->count(); index++)
      printf("%s\n", ui->debugValueList->item(index)->text().toStdString().c_str());
   fflush(stdout);
}

void DebugViewer::on_debugShowRegisters_clicked(){
   EmuWrapper& emu = ((MainWindow*)parentWidget())->emu;

   ui->debugValueList->clear();
   ui->debugValueList->addItems(emu.debugGetCpuRegisterString().split('\n'));
}

void DebugViewer::on_debugShowDebugLogs_clicked(){
   EmuWrapper& emu = ((MainWindow*)parentWidget())->emu;
   QVector<QString>& debugStrings = emu.debugGetLogEntrys();
   QVector<uint64_t>& duplicateCallCount = emu.debugGetDuplicateLogEntryCount();
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

   emu.debugGetLogEntrys().clear();
   emu.debugGetDuplicateLogEntryCount().clear();
}
