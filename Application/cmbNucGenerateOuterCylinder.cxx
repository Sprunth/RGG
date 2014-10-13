#include "cmbNucGenerateOuterCylinder.h"

#include "cmbNucAssembly.h"
#include "cmbNucCore.h"
#include "cmbNucPartsTreeItem.h"
#include "cmbNucMaterialColors.h"
#include "cmbNucDefaults.h"

#include "inpFileIO.h"

#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>

QString GetRandomString(int length)
{
  const QString possibleCharacters("abcdefghijklmnopqrstuvwxyz0123456789");

  QString randomString;
  for(int i=0; i<length; ++i)
  {
    int index = qrand() % possibleCharacters.length();
    QChar nextChar = possibleCharacters.at(index);
    randomString.append(nextChar);
  }
  return randomString;
}

cmbNucGenerateOuterCylinder
::cmbNucGenerateOuterCylinder()
: Core(NULL)
{
}

cmbNucGenerateOuterCylinder
::~cmbNucGenerateOuterCylinder()
{
  deleteTempFiles();
}

void
cmbNucGenerateOuterCylinder
::exportFiles(cmbNucCore * core)
{
  this->Core = core;
  if(this->Core == NULL) return;
  deleteTempFiles();
  if(this->Core->Params.BackgroundMode == cmbNucCoreParams::Generate)
  {
    FileName = this->Core->Params.BackgroundFullPath.c_str();
    if(FileName.isEmpty())
    {
      QMessageBox msgBox;
      msgBox.setText(QString("Could not generate a file outer jacket file, skipping"));
      msgBox.exec();
    }
    else
    {
      random = GetRandomString(8);
      Generate();
    }
  }
}

void
cmbNucGenerateOuterCylinder
::Generate()
{
  //Generate temp inp file of outer cores of an assembly
  QFileInfo fi(FileName);
  cmbNucAssembly * temp = this->Core->GetUsedAssemblies()[0];
  QString fname = QString(temp->getLabel().c_str()).toLower() + random + ".inp";
  fname = fname.toLower();
  QString fullPath =fi.dir().absoluteFilePath(fname);
  inpFileWriter::write(fullPath.toStdString(), *temp, false, true);

  //Generate temp inp file of type geometry of core
  QString corename = QString("core") + random + ".inp";
  fullPath =fi.dir().absoluteFilePath(corename);
  inpFileWriter::writeGSH(fullPath.toStdString(), *Core, fname.toStdString());

  //Generate temp jou file for coregen
  QString jouname = QString("cylinder") + random + ".jou";
  fullPath =fi.dir().absoluteFilePath(jouname);
  std::ofstream output(fullPath.toStdString().c_str());
  output << "{include(\"" << QFileInfo(fname).completeBaseName().toStdString() << ".template.jou\")}\n";
  output << "{rings = " << Core->getLattice().Grid.size() << "}\n";
  output << "#{OUTER_CYL_EDGE_INTERVAL = " << this->Core->getCylinderOuterSpacing() << "}\n";
  output << "#{rd = " << this->Core->getCylinderRadius() << "}\n";
  output << "#{tol = 1e-2}\n";
  output << "#{TOTAL_VOLS_LARGE = 4000}\n";
  output << "{xmove = rings*PITCH}\n";
  output << "{y0 = (rings-1)*PITCH*cosd(30)}\n";
  output << "create cylinder radius {rd} height {Z_HEIGHT}\n";
  output << "move vol 1 x {xmove} y {-y0}  z {Z_HEIGHT/2}\n";
  output << "group 'one' equals vol 1\n";
  output << "import '" <<  QFileInfo(corename).completeBaseName().toStdString() << ".sat'\n";
  output << "group 'gall' equals vol 2 to {TOTAL_VOLS_LARGE}\n";
  output << "subtract vol 2 to 4000 from vol 1\n";
  output << "export acis '" << QFileInfo(jouname).completeBaseName().toStdString() << ".sat' over\n";
  output << "surface with z_coord > {Z_MID -.1*Z_HEIGHT} and z_coord < {Z_MID + .1*Z_HEIGHT} size {AXIAL_MESH_SIZE}\n";
  output << "curve with z_coord > {Z_HEIGHT - tol} and length < {PITCH} interval {TOP_EDGE_INTERVAL}\n";
  output << "curve with z_coord > {Z_HEIGHT - tol} and length > {PITCH} interval {OUTER_CYL_EDGE_INTERVAL}\n";
  output << "mesh vol all\n";
  output << "block 9999 vol all\n";
  output << "save as '" << fi.fileName().toStdString() << "' over\n";
  output.close();
}

QString cmbNucGenerateOuterCylinder
::getAssygenFileName()
{
  QFileInfo fi(FileName);
  cmbNucAssembly * temp = this->Core->GetUsedAssemblies()[0];
  QString fname = QString(temp->getLabel().c_str()).toLower() + random + ".inp";
  return fi.dir().absoluteFilePath(fname);
}

bool
cmbNucGenerateOuterCylinder
::generateCylinder()
{
  return this->Core != NULL && this->Core->Params.BackgroundMode == cmbNucCoreParams::Generate && !FileName.isEmpty();
}

QString cmbNucGenerateOuterCylinder
::getCubitFileName()
{
  QFileInfo fi(FileName);
  QString jouname = QString("cylinder") + random + ".jou";
  return fi.dir().absoluteFilePath(jouname);
}

QString cmbNucGenerateOuterCylinder
::getCoreGenFileName()
{
  QFileInfo fi(FileName);
  QString corename = QString("core") + random + ".inp";
  return fi.dir().absoluteFilePath(corename);
}

QString cmbNucGenerateOuterCylinder
::getSATFileName()
{
  QFileInfo fi(FileName);
  QString corename = QString("core") + random + ".inp";
  return fi.dir().absoluteFilePath(QFileInfo(corename).completeBaseName() + ".sat");
}

void cmbNucGenerateOuterCylinder
::deleteTempFiles()
{
  if(this->generateCylinder())
  {
    if(random.isEmpty()) return;
    QString path = QFileInfo(FileName).dir().absolutePath();;
    QDir dir(path);
    dir.setNameFilters(QStringList() << ("*" + random + ".*"));
    dir.setFilter(QDir::Files);
    foreach(QString dirFile, dir.entryList())
    {
      dir.remove(dirFile);
    }
    random = QString();
  }
}
