#ifndef cmbNucGenerateOuterCylinder_h
#define cmbNucGenerateOuterCylinder_h

#include <QDialog>
#include <QStringList>
#include <QThread>
#include "cmbNucPartDefinition.h"
#include "cmbNucCore.h"

class cmbNucGenerateOuterCylinder
{
public:
  cmbNucGenerateOuterCylinder();
  ~cmbNucGenerateOuterCylinder();

  void exportFiles(cmbNucCore * core);
  void deleteTempFiles();

  QString getAssygenFileName();
  QString getCubitFileName();
  QString getCoreGenFileName();
  QString getSATFileName();

  bool generateCylinder();

protected:
  void Generate();

private:
  // Designer form
  cmbNucCore * Core;

  QString FileName;
  QString random;

};


#endif
