#include "mainwindow.h"

#include <QApplication>
#include <QScreen>


int main(int argc, char* argv[]){
   QApplication a(argc, argv);
   MainWindow w;

   a.setOrganizationDomain("meepingsnesroms.github.com");
   a.setApplicationName("Mu");

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
   //Qt doesnt set up the screen size properly on Android(it seems to use the total display size without subtracting both nav bars height)
   QScreen* screen = QApplication::screens().at(0);
   w.setFixedSize(screen->size().width(), screen->size().height());
#endif

   w.show();

   return a.exec();
}
