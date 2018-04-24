#pragma once

#include <QDialog>
#include <QString>

namespace Ui {
class HexViewer;
}

class HexViewer : public QDialog
{
   Q_OBJECT

public:
   explicit HexViewer(QWidget *parent = 0);
   ~HexViewer();

private slots:
   int64_t numberFromString(QString str, bool negativeAllowed);
   QString stringFromNumber(int64_t number, bool hex, uint32_t forcedZeros = 0);

   void hexRadioButtonHandler();

   void on_hexUpdate_clicked();

   void on_hex8Bit_clicked();

   void on_hex16Bit_clicked();

   void on_hex32Bit_clicked();

   void on_hexDump_clicked();

private:
   uint8_t bitsPerEntry;
   Ui::HexViewer *ui;
};
