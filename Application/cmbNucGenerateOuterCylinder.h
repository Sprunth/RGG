#ifndef cmbNucGenerateOuterCylinder_h
#define cmbNucGenerateOuterCylinder_h

#include <QDialog>
#include <QStringList>
#include <QThread>
#include "cmbNucPartDefinition.h"
#include "cmbNucExport.h"
#include "cmbNucCore.h"
#include "ui_qProgress.h"

// Forward Qt class declarations
class Ui_CylinderCreateGui;
class cmbNucMainWindow;

class cmbNucGenerateOuterCylinder : public QDialog
{
  Q_OBJECT
public:
  cmbNucGenerateOuterCylinder(cmbNucMainWindow* mainWindow);
  ~cmbNucGenerateOuterCylinder();

public slots:
  void exportFile(cmbNucCore * core);

protected slots:
  void SetFileName();
  void Generate();
  void done();
  void cancel();
  void updateCylinder();

signals:
  void process( const QString assygenExe,
               const QString assygenLib,
               const QString assygenFile,
               const QString cubitExe,
               const QString cubitFile,
               const QString cubitOutputFile,
               const QString coregenExe,
               const QString coregenLib,
               const QString coregenFile,
               const QString coregenResultFile );
public:
signals:
  void error(QString);
  void finished(QString);
  void fileFinish();
  void drawCylinder(double r, int inter);
  void clearCylinder();

private:
  // Designer form
  Ui_CylinderCreateGui *ui;
  cmbNucCore * Core;
  cmbNucMainWindow *MainWindow;

  cmbNucExport * Exporter;
  QThread Thread;
  QString FileName;
  QString random;

  void deleteTempFiles();
};


#endif
