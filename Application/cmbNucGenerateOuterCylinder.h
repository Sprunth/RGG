#ifndef cmbNucGenerateOuterCylinder_h
#define cmbNucGenerateOuterCylinder_h

#include <QDialog>
#include <QStringList>
#include <QThread>
#include "cmbNucPartDefinition.h"
#include "cmbNucCore.h"
#include "cmbNucInpExporter.h"

class cmbNucGenerateOuterCylinder
{
public:
  cmbNucGenerateOuterCylinder();
  ~cmbNucGenerateOuterCylinder();

  void exportFiles(cmbNucCore * core, cmbNucInpExporter & inpExporter);

  QString getAssygenFileName();
  QString getCubitFileName();
  QString getCoreGenFileName();
  QString getSATFileName();

  bool generateCylinder();

protected:
  void Generate(cmbNucInpExporter & inpExporter);

private:
  // Designer form
  cmbNucCore * Core;

  QString FileName;
  QString random;

};


#endif
