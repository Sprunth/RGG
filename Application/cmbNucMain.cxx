#include <QApplication>
#include <QString>
#include "cmbNucMainWindow.h"
#include <QMetaObject>
#include <QDebug>
#include <QTimer>

#include "macro_helpers.h"
 
int main( int argc, char** argv )
{
  QApplication::setApplicationName("CMB Nuclear Energy Reactor Geometry Generator");
  QApplication::setApplicationVersion(RGG_VERSION_STR);
  QApplication::setOrganizationName("Kitware Inc");
 
  // QT Stuff
  QApplication app( argc, argv );
 
  cmbNucMainWindow cmbNucAppUI;
  cmbNucAppUI.show();

  bool exit = false;
  QString testFName;
  QStringList testModelCorrectImages;
  QStringList test2DCorrectImages;
  QString testDirectory;
  QString testOutputDirectory;
  QString testMeshCorrectImage;

  for(int i = 1; i < argc; ++i)
  {
    if(QString(argv[i]) == QString("--exit"))
    {
      exit = true;
    }
    if(QString(argv[i]).startsWith("--test-script"))
    {
      testFName = QString(argv[i]).split("=").at(1);
    }
    if(QString(argv[i]).startsWith("--truth-model-images"))
    {
      testModelCorrectImages = QString(argv[i]).split("=").at(1).split(";");
    }
    if(QString(argv[i]).startsWith("--truth-2D-images"))
    {
      test2DCorrectImages = QString(argv[i]).split("=").at(1).split(";");
    }
    if(QString(argv[i]).startsWith("--test-directory"))
    {
      testDirectory = QString(argv[i]).split("=").at(1);
    }
    if(QString(argv[i]).startsWith("--test-output-directory"))
    {
      testOutputDirectory = QString(argv[i]).split("=").at(1);
    }
    if(QString(argv[i]).startsWith("--truth-mesh-image"))
    {
      testMeshCorrectImage = QString(argv[i]).split("=").at(1);
    }
  }
  

  if(!testFName.isEmpty())
  {
    QString retVal;
    QTimer::singleShot(0,&cmbNucAppUI, SLOT(playTest()));
    cmbNucAppUI.setUpTests(testFName, testModelCorrectImages, test2DCorrectImages,
                           testMeshCorrectImage, testDirectory, testOutputDirectory, exit);
  }

  return app.exec();
}
