#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
   QApplication a(argc, argv);
   MainWindow w;

   a.setOrganizationDomain("meepingsnesroms.github.com");
   a.setApplicationName("Mu");
   w.show();

   return a.exec();
}
