#include <QApplication>
#include "cmbNucMainWindow.h"
 
int main( int argc, char** argv )
{
  QApplication::setApplicationName("CMB Nuclear Energy Reactor Geometry Generator");
  QApplication::setApplicationVersion("0.1.0");
  QApplication::setOrganizationName("Kitware Inc");
 
  // QT Stuff
  QApplication app( argc, argv );
 
  cmbNucMainWindow cmbNucAppUI;
  cmbNucAppUI.show();
 
  return app.exec();
}
