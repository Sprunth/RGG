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
::exportFiles(cmbNucCore * core, cmbNucInpExporter & inpExporter)
{
  this->Core = core;
  if(this->Core == NULL) return;
  deleteTempFiles();
  if(this->Core->Params.BackgroundMode == cmbNucCoreParams::Generate)
  {
    QFileInfo qi(this->Core->ExportFileName.c_str());
    std::string tmp = this->Core->Params.Background;
    if(tmp.empty())
    {
      QMessageBox msgBox;
      msgBox.setText(QString("Could not generate a file outer jacket file, skipping"));
      msgBox.exec();
    }
    else
    {
      FileName = qi.dir().absolutePath() + "/" + tmp.c_str();
      this->Core->Params.BackgroundFullPath = FileName.toStdString();
      random = GetRandomString(8);
      Generate(inpExporter);
    }
  }
}

void
cmbNucGenerateOuterCylinder
::Generate(cmbNucInpExporter & inpExporter)
{
  inpExporter.exportCylinderINPFile(FileName,random);

  //Generate temp jou file for coregen
  QFileInfo fi(FileName);
  QString jouname  = QString("cylinder") + random + ".jou";
  QString fullPath = fi.dir().absoluteFilePath(jouname);
  QString corename = QString("core") + random + ".inp";
  std::ofstream output(fullPath.toStdString().c_str());
  QString fname = QString(this->Core->GetUsedAssemblies()[0]->getLabel().c_str()).toLower() + random + ".inp";
  double z0;
  this->Core->GetDefaults()->getZ0(z0);
  output << "{include(\"" << QFileInfo(fname).completeBaseName().toStdString() << ".template.jou\")}\n";
  output << "#{OUTER_CYL_EDGE_INTERVAL = " << this->Core->getCylinderOuterSpacing() << "}\n";
  output << "#{rd = " << this->Core->getCylinderRadius() << "}\n";
  output << "#{z0 = " << z0 << "}\n";
  output << "#{tol = 1e-2}\n";
  output << "#{TOTAL_VOLS_LARGE = 4000}\n";
  if(this->Core->IsHexType())
  {
    output << "#{rings = " << Core->getLattice().getSize() << "}\n";
    output << "#{xmove = rings*PITCH}\n";
    output << "#{y0 = (rings-1)*PITCH*cosd(30)}\n";
  }
  else
  {
    /*double outerDuctWidth;
    double outerDuctHeight;
    this->Core->GetDefaults()->getDuctThickness(outerDuctWidth, outerDuctHeight);
    double tx = outerDuctWidth*(core->getLattice().getSize()-1)*0.5;
    double ty = outerDuctHeight*(core->getLattice().getSize(0)-1)*0.5;
    double extraXTrans = ty,
    double extraYTrans = tx-outerDuctWidth*(core->getLattice().getSize()-1);*/
    output << "#{latwidth = " << this->Core->getLattice().getSize(0) << "}\n";
    output << "#{latheight = " << this->Core->getLattice().getSize() << "}\n";
    output << "#{xmove = (PITCHX*(latwidth-1))*0.5}\n";
    output << "#{y0 = (((-1*PITCHY*(latheight-1))*0.5) + (PITCHY*(latheight-1)))}\n";
  }
  output << "create cylinder radius {rd} height {Z_HEIGHT}\n";
  output << "move vol 1 x {xmove} y {-y0}  z {Z_HEIGHT/2+z0}\n";
  output << "import '" <<  QFileInfo(corename).completeBaseName().toStdString() << ".sat'\n";
  output << "group 'gall' equals vol 2 to {TOTAL_VOLS_LARGE}\n";
  output << "subtract vol 2 to 4000 from vol 1\n";
  output << "export acis '" << QFileInfo(jouname).completeBaseName().toStdString() << ".sat' over\n";
  output << "surface with z_coord > {Z_MID -.1*Z_HEIGHT} and z_coord < {Z_MID + .1*Z_HEIGHT} size {AXIAL_MESH_SIZE}\n";
  if(this->Core->IsHexType())
  {
    output << "curve with z_coord > {Z_HEIGHT - tol} and length < {PITCH} interval {TOP_EDGE_INTERVAL}\n";
    output << "curve with z_coord > {Z_HEIGHT - tol} and length > {PITCH} interval {OUTER_CYL_EDGE_INTERVAL}\n";
  }
  else
  {
    output << "curve with z_coord > {Z_HEIGHT - tol} and x_length < {PITCHX} interval {TOP_EDGE_INTERVAL}\n";
    output << "curve with z_coord > {Z_HEIGHT - tol} and length > {PITCHX} interval {OUTER_CYL_EDGE_INTERVAL}\n";
  }
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
