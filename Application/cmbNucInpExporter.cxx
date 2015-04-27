#include "cmbNucInpExporter.h"
#include "inpFileIO.h"
#include "cmbNucCore.h"
#include "cmbNucAssembly.h"
#include "cmbNucDuctLibrary.h"

#include <QFileInfo>
#include <QFileDialog>
#include <QSettings>

#include <set>

cmbNucInpExporter
::cmbNucInpExporter()
{
}

void cmbNucInpExporter
::setCore(cmbNucCore * core)
{
  this->NuclearCore = core;
  this->updateCoreLayers(true);
}

bool cmbNucInpExporter
::exportInpFiles()
{
  QString coreName = this->NuclearCore->getExportFileName().c_str();
  if(coreName.isEmpty())
  {
    coreName = requestInpFileName("","Core");
  }
  if(coreName.isEmpty()) return false;

  //clone the pins and ducts.  These are needed for determining layers
  cmbNucDuctLibrary * odl = this->NuclearCore->getDuctLibrary();
  cmbNucPinLibrary * pl = this->NuclearCore->getPinLibrary()->clone();
  cmbNucDuctLibrary * dl = odl->clone();

  //split ducts if needed
  for(unsigned int i = 0; i < dl->GetNumberOfDuctCells(); ++i)
  {
    if(!odl->GetDuctCell(i)->isUsed()) continue;
    dl->GetDuctCell(i)->uniformizeMaterialLayers();
    dl->GetDuctCell(i)->splitDucts(this->coreLevelLayers.levels);
  }

  //split pins if needed
  for(unsigned int i = 0; i < pl->GetNumberOfPinCells(); ++i)
  {
    pl->GetPinCell(i)->splitPin(this->coreLevelLayers.levels);
  }

  QFileInfo coreinfo(coreName);

  std::map< std::string, std::set< Lattice::CellDrawMode > > cells = this->NuclearCore->getDrawModesForAssemblies();

  for(std::map< std::string, std::set< Lattice::CellDrawMode > >::const_iterator iter = cells.begin();
      iter != cells.end(); ++iter)
  {
    cmbNucAssembly* assembly = NuclearCore->GetAssembly(iter->first);
    assembly->setPath(coreinfo.dir().absolutePath().toStdString());
    std::set< Lattice::CellDrawMode > const& forms = iter->second;
    if(assembly == NULL) continue;
    cmbNucAssembly* assemblyClone = assembly->clone(pl, dl);
    this->exportInpFile(assemblyClone, false, forms);
    delete assemblyClone;
  }
  if(this->NuclearCore->changeSinceLastGenerate())
  {
    inpFileWriter::write(coreName.toStdString(), *(this->NuclearCore));
  }
  if(this->NuclearCore->boundryLayerChangedSinceLastGenerate())
  {
    QString path = coreinfo.absolutePath();
    this->NuclearCore->generateExportBoundryLayers();
    //std::string previous = this->NuclearCore->getMeshFilename(0);
    for(int i = 0; i < this->NuclearCore->getNumberOfExportBoundryLayers(); ++i)
    {
      std::string prev = this->NuclearCore->getMeshFilename(static_cast<size_t>(i));
      std::string current = this->NuclearCore->getMeshFilename(static_cast<size_t>(i+1));
      std::ofstream out((path.toStdString() + "/" + current+".txt").c_str());
      cmbNucCore::boundryLayer const* bl = this->NuclearCore->getExportBoundryLayer(i);
      out << bl->jou_file_name << std::endl;
      out << current+".inp" << std::endl;
      out << prev << std::endl;
      out << "g_" << bl->fixed_material->getName().toLower().toStdString() << std::endl;
      out << bl->fixed_material->getName().toLower().toStdString() << "_side1_ss" << std::endl;
      out << bl->Thickness << std::endl;
      out << bl->Intervals << std::endl;
      out << bl->Bias << std::endl;
      out << current << std::endl;
      out.close();
    }
  }
  return true;
}

bool cmbNucInpExporter
::exportCylinderINPFile(QString filename, QString random)
{
  //clone the pins and ducts.  These are needed for determining layers
  cmbNucDuctLibrary * odl = this->NuclearCore->getDuctLibrary();
  cmbNucPinLibrary * pl = this->NuclearCore->getPinLibrary()->clone();
  cmbNucDuctLibrary * dl = odl->clone();

  //split ducts if needed
  for(unsigned int i = 0; i < dl->GetNumberOfDuctCells(); ++i)
  {
    if(!odl->GetDuctCell(i)->isUsed()) continue;
    dl->GetDuctCell(i)->splitDucts(this->coreLevelLayers.levels);
  }

  //Generate temp inp file of outer cores of an assembly
  QFileInfo fi(filename);
  cmbNucAssembly * temp = this->NuclearCore->GetUsedAssemblies()[0]->clone(pl,dl);
  QString fname = QString(temp->getLabel().c_str()).toLower() + random + ".inp";
  fname = fname.toLower();
  QString fullPath =fi.dir().absoluteFilePath(fname);
  temp->setPath(fullPath.toStdString());
  temp->setFileName(fname.toStdString());

  std::set< Lattice::CellDrawMode > set;
  set.insert(temp->getLattice().getFullCellMode());
  this->exportInpFile(temp, true, set);

  delete temp;
  delete pl;
  delete dl;

  //Generate temp inp file of type geometry of core
  QString corename = QString("core") + random + ".inp";
  fullPath =fi.dir().absoluteFilePath(corename);
  inpFileWriter::writeGSH(fullPath.toStdString(), *NuclearCore, fname.toStdString());
  return true;
}

bool cmbNucInpExporter
::updateCoreLevelLayers()
{
  
  return true;
}

void cmbNucInpExporter
::updateCoreLayers(bool ignore_regen)
{
  std::vector<double> levels;
  std::set<DuctCell *> dc;
  std::set<PinCell *> pc;
  std::set<double> unique_levels;

  std::vector< cmbNucAssembly* > used = this->NuclearCore->GetUsedAssemblies();
  for(std::vector< cmbNucAssembly* >::const_iterator iter = used.begin(); iter != used.end(); ++iter)
  {
    cmbNucAssembly * assy = *iter;
    dc.insert(&(assy->getAssyDuct()));
    std::size_t npc = assy->GetNumberOfPinCells();
    for(std::size_t i = 0; i < npc; ++i)
    {
      pc.insert(assy->GetPinCell(i));
    }
  }
  for(std::set<DuctCell *>::const_iterator iter = dc.begin(); iter != dc.end(); ++iter)
  {
    std::vector<double> tmp = (*iter)->getDuctLayers();
    for(unsigned int i = 0; i < tmp.size(); ++i)
    {
      unique_levels.insert(tmp[i]);
    }
  }
  for(std::set<PinCell *>::const_iterator iter = pc.begin(); iter != pc.end(); ++iter)
  {
    std::vector<double> tmp = (*iter)->getPinLayers();
    for(unsigned int i = 0; i < tmp.size(); ++i)
    {
      unique_levels.insert(tmp[i]);
    }
  }
  for(std::set<double>::const_iterator iter = unique_levels.begin(); iter != unique_levels.end(); ++iter)
  {
    levels.push_back(*iter);
  }

  std::sort(levels.begin(), levels.end());

  if(!ignore_regen)
  {
    bool same = coreLevelLayers.levels.size() == levels.size();
    for(unsigned int i = 0; i < levels.size() && same; ++i)
    {
      same = levels[i] == coreLevelLayers.levels[i];
    }
    if(!same)
    {
      for(std::vector< cmbNucAssembly* >::const_iterator iter = used.begin(); iter != used.end(); ++iter)
      {
        (*iter)->setAndTestDiffFromFiles(true);
      }
    }
  }
  coreLevelLayers.levels = levels;
}

bool cmbNucInpExporter
::exportInpFile(cmbNucAssembly * assy, bool isCylinder,
                std::set< Lattice::CellDrawMode > const& forms)
{
  for(std::set< Lattice::CellDrawMode >::const_iterator fiter = forms.begin(); fiter != forms.end(); ++fiter)
  {
    double deg = assy->getZAxisRotation();
    if( assy->getLattice().getFullCellMode() == Lattice::HEX_FULL_30 )
    {
      deg += 30;
    }
    assy->removeOldTransforms(0);
    if(deg != 0)
    {
      assy->addTransform(new cmbNucAssembly::Rotate(cmbNucAssembly::Transform::Z, deg));
    }

    Lattice::CellDrawMode mode = *fiter;
    std::string const& fname = assy->getFileName(mode, forms.size());

    switch(mode)
    {
      case Lattice::HEX_SIXTH_FLAT_BOTTOM:
      case Lattice::HEX_SIXTH_VERT_BOTTOM:
      case Lattice::HEX_TWELFTH_BOTTOM:
        assy->addTransform(new cmbNucAssembly::Section( cmbNucAssembly::Transform::Y, 0, 1));
        break;
      case Lattice::HEX_SIXTH_FLAT_TOP:
      case Lattice::HEX_SIXTH_VERT_TOP:
        assy->addTransform(new cmbNucAssembly::Rotate(cmbNucAssembly::Transform::Z, 30));
        assy->addTransform(new cmbNucAssembly::Section( cmbNucAssembly::Transform::X, 0, 1));
        assy->addTransform(new cmbNucAssembly::Rotate(cmbNucAssembly::Transform::Z, -30));
        break;
      case Lattice::HEX_TWELFTH_TOP:
        assy->addTransform(new cmbNucAssembly::Rotate(cmbNucAssembly::Transform::Z, -30));
        assy->addTransform(new cmbNucAssembly::Section( cmbNucAssembly::Transform::Y, 0, -1));
        assy->addTransform(new cmbNucAssembly::Rotate(cmbNucAssembly::Transform::Z, 30));
        break;
      case Lattice::HEX_SIXTH_FLAT_CENTER:
      case Lattice::HEX_SIXTH_VERT_CENTER:
        assy->addTransform(new cmbNucAssembly::Section( cmbNucAssembly::Transform::Y, 0, 1));
        assy->addTransform(new cmbNucAssembly::Rotate(cmbNucAssembly::Transform::Z, 30));
        assy->addTransform(new cmbNucAssembly::Section( cmbNucAssembly::Transform::X, 0, 1));
        assy->addTransform(new cmbNucAssembly::Rotate(cmbNucAssembly::Transform::Z, -30));
        break;
      case Lattice::HEX_TWELFTH_CENTER:
        assy->addTransform(new cmbNucAssembly::Section( cmbNucAssembly::Transform::Y, 0, 1));
        assy->addTransform(new cmbNucAssembly::Rotate(cmbNucAssembly::Transform::Z, -30));
        assy->addTransform(new cmbNucAssembly::Section( cmbNucAssembly::Transform::Y, 0, -1));
        assy->addTransform(new cmbNucAssembly::Rotate(cmbNucAssembly::Transform::Z, 30));
        break;
      case Lattice::RECT:
      case Lattice::HEX_FULL:
      case Lattice::HEX_FULL_30:
        break;
    }
    QFileInfo assyFile(fname.c_str());

    if(!assyFile.exists() || assy->changeSinceLastGenerate())
    {
      inpFileWriter::write(fname, *assy, false, isCylinder);
    }
  }
  assy->removeOldTransforms(0);
  return true;
}

QString cmbNucInpExporter::requestInpFileName(QString name,
                                              QString type)
{
  QString defaultName("");
  QString defaultLoc;
  if(!name.isEmpty())
  {
    QFileInfo fi(name);
    QDir dir = fi.dir();
    if(dir.path() == ".")
    {
      defaultName = fi.baseName();
      QDir tdir = QSettings("CMBNuclear", "CMBNuclear").value("cache/lastDir",
                                                              QDir::homePath()).toString();
      defaultLoc = tdir.path();
    }
    else
    {
      defaultLoc = dir.path();
      defaultName = fi.baseName();
    }
  }
  if(defaultLoc.isEmpty())
  {
    QDir dir = QSettings("CMBNuclear", "CMBNuclear").value("cache/lastDir",
                                                           QDir::homePath()).toString();
    defaultLoc = dir.path();
  }

  QFileDialog saveQD( NULL, "Save "+type+" File...", defaultLoc, "INP Files (*.inp)");
  saveQD.setOptions(QFileDialog::DontUseNativeDialog); //There is a bug on the mac were one does not seem to be able to set the default name.
  saveQD.setAcceptMode(QFileDialog::AcceptSave);
  saveQD.selectFile(defaultName);
  QString fileName;
  if(saveQD.exec()== QDialog::Accepted)
  {
    fileName = saveQD.selectedFiles().first();
  }
  else
  {
    return QString();
  }
  if( !fileName.endsWith(".inp") )
    fileName += ".inp";

  if(!fileName.isEmpty())
  {
    // Cache the directory for the next time the dialog is opened
    QFileInfo info(fileName);
    QSettings("CMBNuclear", "CMBNuclear").setValue("cache/lastDir",
                                                   info.dir().path());
  }
  return fileName;
}

