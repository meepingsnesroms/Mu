#pragma once

#include <QDialog>
#include <QString>

#include <stdint.h>

namespace Ui{
class DebugViewer;
}

class DebugViewer : public QDialog{
   Q_OBJECT

public:
   explicit DebugViewer(QWidget* parent = nullptr);
   ~DebugViewer();

private:
   int64_t numberFromString(QString str, bool negativeAllowed);

private slots:
   void debugRadioButtonHandler();

   void on_debugGetHexValues_clicked();
   void on_debugDecompile_clicked();

   void on_debug8Bit_clicked();
   void on_debug16Bit_clicked();
   void on_debug32Bit_clicked();

   void on_debugDumpToFile_clicked();
   void on_debugDumpToTerminal_clicked();
   void on_debugShowRegisters_clicked();
   void on_debugShowDebugLogs_clicked();
   void on_debugEraseDebugLogs_clicked();

private:
   uint8_t          bitsPerEntry;
   Ui::DebugViewer* ui;
};
