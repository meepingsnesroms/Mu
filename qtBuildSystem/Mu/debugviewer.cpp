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
   setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

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
   QString data = "";

   if(address != INT64_MIN && length != INT64_MIN && length != 0 && address + bits / 8 * length - 1 <= 0xFFFFFFFF){
      for(int64_t count = 0; count < length; count++){
         uint64_t emuData = emu.debugGetEmulatorMemory(address, bits);
         QString value;
         value += QString::asprintf("0x%08X", (uint32_t)address);
         value += ":";
         if(emuData != UINT64_MAX)
            value += QString::asprintf("0x%0*X", bits / 8 * 2, (uint32_t)emuData);
         else
            value += "Unsafe Access";
         data += value + '\n';
         address += bits / 8;
      }
   }
   else{
      data += "Invalid Parameters";
   }

   ui->debugValueList->setText(data);
}

void DebugViewer::on_debugDecompile_clicked(){
   EmuWrapper& emu = ((MainWindow*)parentWidget())->emu;
   int64_t address = numberFromString(ui->debugAddress->text(), true/*negative not allowed*/);
   int64_t length = numberFromString(ui->debugLength->text(), true/*negative not allowed*/);

   if(address != INT64_MIN && length != INT64_MIN && length != 0)
      ui->debugValueList->setText(emu.debugDisassemble(address, length));
   else
      ui->debugValueList->setText("Invalid Parameters");
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

   fileBuffer = ui->debugValueList->toPlainText();

   if(fileOut.open(QFile::ReadWrite)){
      //if a QString is used for output a '\0' will be appended to every character
      fileOut.write(fileBuffer.toStdString().c_str());
      fileOut.close();
   }
}

void DebugViewer::on_debugDumpToTerminal_clicked(){
   printf("%s\n", ui->debugValueList->toPlainText().toStdString().c_str());
   fflush(stdout);
}

void DebugViewer::on_debugShowRegisters_clicked(){
   EmuWrapper& emu = ((MainWindow*)parentWidget())->emu;

   ui->debugValueList->setText(emu.debugGetCpuRegisterString());
}

void DebugViewer::on_debugShowDebugLogs_clicked(){
   EmuWrapper& emu = ((MainWindow*)parentWidget())->emu;
   QVector<QString>& debugStrings = emu.debugLogEntrys();
   QVector<uint64_t>& duplicateCallCount = emu.debugDuplicateLogEntryCount();
   uint64_t& deletedLogEntrys = emu.debugDeletedLogEntryCount();
   int64_t length = numberFromString(ui->debugLength->text(), true/*negative allowed*/);
   QString data = "";

   if(deletedLogEntrys > 0)
      data += QString::asprintf("(there are %llu deleted log entrys before these)\n", emu.debugDeletedLogEntryCount());

   if(length != INT64_MIN && qAbs(length) < debugStrings.size()){
      if(length < 0){
         for(uint64_t stringNum = debugStrings.size() + length; stringNum < debugStrings.size(); stringNum++)
            data += debugStrings[stringNum] + "(printed " + QString::number(duplicateCallCount[stringNum]) + " times)\n";
      }
      else{
         for(uint64_t stringNum = 0; stringNum < length; stringNum++)
            data += debugStrings[stringNum] + "(printed " + QString::number(duplicateCallCount[stringNum]) + " times)\n";
      }
   }
   else{
      for(uint64_t stringNum = 0; stringNum < debugStrings.size(); stringNum++)
         data += debugStrings[stringNum] + "(printed " + QString::number(duplicateCallCount[stringNum]) + " times)\n";
   }

   ui->debugValueList->setText(data);
}

void DebugViewer::on_debugEraseDebugLogs_clicked(){
   EmuWrapper& emu = ((MainWindow*)parentWidget())->emu;

   emu.debugLogEntrys().clear();
   emu.debugDuplicateLogEntryCount().clear();
   emu.debugDeletedLogEntryCount() = 0;
}
