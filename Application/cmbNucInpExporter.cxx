#include "cmbNucInpExporter.h"
#include "inpFileIO.h"
#include "cmbNucCore.h"
#include "cmbNucAssembly.h"

#include <QFileInfo>
#include <QFileDialog>
#include <QSettings>

cmbNucInpExporter
::cmbNucInpExporter()
{
}

void cmbNucInpExporter
::setCore(cmbNucCore * core)
{
  this->NuclearCore = core;
}

bool cmbNucInpExporter
::exportInpFiles()
{
  QString coreName = this->NuclearCore->ExportFileName.c_str();
  if(coreName.isEmpty())
  {
    coreName = requestInpFileName("","Core");
  }
  if(coreName.isEmpty()) return false;

  QFileInfo coreinfo(coreName);

  std::map< std::string, std::set< Lattice::CellDrawMode > > cells = this->NuclearCore->getDrawModesForAssemblies();

  for(std::map< std::string, std::set< Lattice::CellDrawMode > >::const_iterator iter = cells.begin();
      iter != cells.end(); ++iter)
  {
    cmbNucAssembly* assembly = NuclearCore->GetAssembly(iter->first);
    std::set< Lattice::CellDrawMode > const& forms = iter->second;
    if(assembly == NULL) continue;
    assembly->ExportFileNames.clear();
    QString fileName = assembly->ExportFileName.c_str();
    if(fileName.isEmpty())
    {
      fileName = coreinfo.dir().path() + QString("/assembly_") +
      QString(assembly->getLabel().c_str()).toLower() +
      ".inp";
    }
    QFileInfo assyFileInfo(fileName);
    assembly->ExportFileName = fileName.toStdString();
    for(std::set< Lattice::CellDrawMode >::const_iterator fiter = forms.begin(); fiter != forms.end(); ++fiter)
    {
      Lattice::CellDrawMode mode = *fiter;
      if(mode == Lattice::RECT || mode == Lattice::HEX_FULL || mode == Lattice::HEX_FULL_30 || forms.size() == 1)
      {
        assembly->ExportFileNames[mode] = fileName.toStdString();
      }
      else
      {
        assembly->ExportFileNames[mode] = (coreinfo.dir().path() +
                                           QString("/assembly_") +
                                           QString(Lattice::generate_string(assembly->getLabel(),
                                                                            mode).c_str()).toLower() +
                                           ".inp").toStdString();
      }
    }
    this->exportInpFile(assembly);
  }
  if(this->NuclearCore->changeSinceLastGenerate())
  {
    inpFileWriter::write(coreName.toStdString(), *(this->NuclearCore));
  }
  return true;
}

void cmbNucInpExporter
::removeAssembly(cmbNucAssembly * assy)
{
  
}

bool cmbNucInpExporter
::updateCoreLevelLayers()
{
  return true;
}

bool cmbNucInpExporter
::updateAssemblyLevelLayers(cmbNucAssembly * assy)
{
  return true;
}

bool cmbNucInpExporter
::exportInpFile(cmbNucAssembly * assy)
{
  for(std::map< Lattice::CellDrawMode, std::string >::const_iterator iter = assy->ExportFileNames.begin();
      iter != assy->ExportFileNames.end(); ++iter)
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

    Lattice::CellDrawMode mode = iter->first;
    std::string const& fname = iter->second;

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
    }
    QFileInfo assyFile(fname.c_str());

    if(!assyFile.exists() || assy->changeSinceLastGenerate())
    {
      inpFileWriter::write(fname, *assy, false);
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

