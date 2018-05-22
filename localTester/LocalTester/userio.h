#pragma once

#include <QObject>
#include <QString>

#include <vector>

class UserIO : public QObject
{
   Q_OBJECT

private:
   std::vector<QString> jsStrings;
   std::vector<QString> cxxStrings;
   void (*refreshHandler)() = nullptr;

public:
   explicit UserIO(QObject* parent = nullptr);
   void setRefreshHandler(void (*newRefreshHandler)());

   Q_INVOKABLE bool stringAvailableJs();
   Q_INVOKABLE QString readStringJs();
   Q_INVOKABLE void writeStringJs(QString data);

   bool stringAvailableCxx();
   QString readStringCxx();
   void writeStringCxx(QString data);

   void resetStrings();

signals:

public slots:
};
