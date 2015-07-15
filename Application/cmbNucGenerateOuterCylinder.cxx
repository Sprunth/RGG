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

cmbNucGenerateOuterCylinder
::cmbNucGenerateOuterCylinder()
: Core(NULL)
{
  random = "_gen_for_cylinder";
}

cmbNucGenerateOuterCylinder
::~cmbNucGenerateOuterCylinder()
{
}

bool
cmbNucGenerateOuterCylinder
::exportFiles(cmbNucCore * core, cmbNucInpExporter & inpExporter)
{
  this->Core = core;
  if(this->Core == NULL) return false;
  if(this->Core->getParams().BackgroundMode == cmbNucCoreParams::Generate)
  {
    QFileInfo qi(this->Core->getExportFileName().c_str());
    std::string tmp = this->Core->getParams().Background;
    if(tmp.empty())
    {
      QMessageBox msgBox;
      msgBox.setText(QString("Could not generate a outer jacket file"));
      msgBox.exec();
      return false;
    }
    else
    {
      FileName = qi.dir().absolutePath() + "/" + tmp.c_str();
      this->Core->getParams().BackgroundFullPath = FileName.toStdString();
      return Generate(inpExporter);
    }
  }
  return true;
}

bool
cmbNucGenerateOuterCylinder
::Generate(cmbNucInpExporter & inpExporter)
{
  if(!inpExporter.exportCylinderINPFile(FileName,random))
  {
    return false;
  }
  cmbNucInpExporter::layers const& layers = inpExporter.getLayers();
  std::vector< cmbNucAssembly* > used = this->Core->GetUsedAssemblies();
  if (used.empty())
  {
    return false;
  }

  //Generate temp jou file for coregen
  QFileInfo fi(FileName);
  QString jouname  = QString("cylinder") + random + ".jou";
  QString fullPath = fi.dir().absoluteFilePath(jouname);
  QString corename = QString("core") + random + ".inp";
  std::ofstream output(fullPath.toStdString().c_str());
  QString fname = QString(used[0]->getLabel().c_str()).toLower() +
                          random + ".inp";
  double z0;
  this->Core->GetDefaults()->getZ0(z0);
  double height;
  this->Core->GetDefaults()->getHeight(height);
  output << "reset\n";
  output << "{include(\""
         << QFileInfo(fname).completeBaseName().toStdString()
         << ".template.jou\")}\n";
  output << "#{OUTER_CYL_EDGE_INTERVAL = "
         << this->Core->getCylinderOuterSpacing() << "}\n";
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
    output << "#{latwidth = " << this->Core->getLattice().getSize(0) << "}\n";
    output << "#{latheight = " << this->Core->getLattice().getSize() << "}\n";
    output << "#{xmove = (PITCHX*(latwidth-1))*0.5}\n";
    output << "#{y0 = (((-1*PITCHY*(latheight-1))*0.5) "
              "+ (PITCHY*(latheight-1)))}\n";
  }
  output << "create cylinder radius {rd} height {Z_HEIGHT}\n";
  output << "move vol 1 x {xmove} y {-y0}  z {Z_HEIGHT/2+z0}\n";
  output << "import '" <<  QFileInfo(corename).completeBaseName().toStdString()
         << ".sat'\n";
  output << "group 'gall' equals vol 2 to {TOTAL_VOLS_LARGE}\n";
  if(!this->Core->IsHexType())
  {
    output << "imprint vol all\n";
  }
  output << "subtract vol 2 to 4000 from vol 1\n";

  output << "group 'g1' equals vol all\n";
  output << "import '" <<  QFileInfo(corename).completeBaseName().toStdString()
         << ".sat'\n";
  output << "imprint vol all\n";
  if(!this->Core->IsHexType())
  {
    output << "group 'g2' vol all\n";
  }
  output << "group 'g2' equals vol all\n";
  if(!this->Core->IsHexType())
  {
    output << "group 'g3' subtract vol in g2 from g1\n";
  }
  output << "group 'g3' subtract vol in g1 from g2\n";
  output << "delete vol in g3\n";
  output << "delete group g2 g3\n";
  if(layers.levels.size() >= 2)
  {
    output << "### loop one less than total number of blocks\n";
    output << "web vol in g1  with zplane offset {BLOCK2_ZBOT}\n";
    output << "##group 'g1' equals vol all\n";
    output << "web vol in g1  with zplane offset {BLOCK3_ZBOT}\n";
    output << "### loop ends\n";
    output << "merge vol all\n";
    output << "imprint vol all\n";
  }
  output << "export acis '"
         << QFileInfo(jouname).completeBaseName().toStdString()
         << ".sat' over\n";
  if(layers.levels.size() <= 2)
  {
    output << "surface with z_coord > {Z_MID -.1*Z_HEIGHT}"
              " and z_coord < {Z_MID + .1*Z_HEIGHT} size {AXIAL_MESH_SIZE}\n";
  }
  if(this->Core->IsHexType())
  {
    output << "curve with z_coord > {Z_HEIGHT - tol} and "
              "length < {PITCH} interval {TOP_EDGE_INTERVAL}\n";
    output << "curve with z_coord > {Z_HEIGHT - tol} and length > {PITCH} "
              "interval {OUTER_CYL_EDGE_INTERVAL}\n";
  }
  else
  {
    output << "curve with z_coord > {Z_HEIGHT - tol} and length > "
              "{PITCHX - tol} interval {TOP_EDGE_INTERVAL}\n";
    output << "curve with z_coord > {Z_HEIGHT - tol} and length > "
              "{PITCHY - tol} interval {TOP_EDGE_INTERVAL}\n";
    output << "curve with z_coord > {Z_HEIGHT - tol} and length > "
              "{PITCHX + PITCHY} interval {OUTER_CYL_EDGE_INTERVAL}\n";
  }
  if(layers.levels.size() > 2)
  {
    output << "mesh surface with z_coord = {BLOCK2_ZTOP = " << height << "}\n";
    output << "### Setting Z intervals on ducts and meshing along Z\n";
    output << "surf with z_coord  > {BLOCK2_ZBOT} and z_coord < {BLOCK2_ZTOP} "
              "interval {BLOCK2_Z_INTERVAL}\n";
    output << "mesh vol with z_coord  > {BLOCK2_ZBOT} and z_coord < "
              "{BLOCK2_ZTOP}\n";
    output << "##\n";
    output << "surf with z_coord  > {BLOCK1_ZBOT} and z_coord < {BLOCK1_ZTOP} "
              "interval {BLOCK1_Z_INTERVAL}\n";
    output << "mesh vol with z_coord> {BLOCK1_ZBOT} and z_coord < "
              "{BLOCK1_ZTOP}\n";
    output << "##\n";
  }
  else
  {
    output << "mesh vol all\n";
  }
  output << "block 9999 vol all\n";
  output << "save as '" << fi.fileName().toStdString() << "' over\n";
  output.close();
  return true;
}

QString cmbNucGenerateOuterCylinder
::getAssygenFileName()
{
  QFileInfo fi(FileName);
  std::vector< cmbNucAssembly* > used = this->Core->GetUsedAssemblies();
  if(used.empty()) return QString();
  QString fname = QString(used[0]->getLabel().c_str()).toLower() +
                  random + ".inp";
  return fi.dir().absoluteFilePath(fname);
}

bool
cmbNucGenerateOuterCylinder
::generateCylinder()
{
  return this->Core != NULL &&
         this->Core->getParams().BackgroundMode == cmbNucCoreParams::Generate &&
         !FileName.isEmpty();
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
  return fi.dir().absoluteFilePath(QFileInfo(corename).completeBaseName() +
                                   ".sat");
}
