#include "mainwindow.h"

#include <QApplication>


int main(int argc, char* argv[]){
   QApplication a(argc, argv);
   MainWindow w;

   a.setOrganizationDomain("meepingsnesroms.github.com");
   a.setApplicationName("Mu");

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
   w.showFullScreen();
#else
   w.show();
#endif

   return a.exec();
}
