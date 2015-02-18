#include "cmbNucImporter.h"
#include "cmbNucCore.h"
#include "inpFileIO.h"
#include "cmbNucMainWindow.h"
#include "cmbNucInputListWidget.h"
#include "cmbNucAssembly.h"
#include "cmbNucInputPropertiesWidget.h"
#include "xmlFileIO.h"
#include "cmbNucDefaults.h"
#include "cmbNucMaterialColors.h"
#include "cmbNucDuctLibrary.h"

#include <QSettings>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QStringList>
#include <QDebug>

extern int defaultAssemblyColors[][3];
extern int numAssemblyDefaultColors;

cmbNucImporter::cmbNucImporter(cmbNucMainWindow * mw)
:mainWindow(mw)
{
}

bool cmbNucImporter::importXMLPins()
{
  // Use cached value for last used directory if there is one,
  // or default to the user's home dir if not.
  QSettings settings("CMBNuclear", "CMBNuclear");
  QDir dir = settings.value("cache/lastDir", QDir::homePath()).toString();

  QStringList fileNames =
  QFileDialog::getOpenFileNames(mainWindow,
                                "Open File...",
                                dir.path(),
                                "RGG XML File (*.RXF)");
  if(fileNames.count()==0)
  {
    return false;
  }

  // Cache the directory for the next time the dialog is opened
  QFileInfo info(fileNames[0]);
  settings.setValue("cache/lastDir", info.dir().path());
  std::map<std::string, std::string> junk;

  double tmpD = 10.0;
  assert(mainWindow->NuclearCore->HasDefaults());
  mainWindow->NuclearCore->GetDefaults()->getHeight(tmpD);

  cmbNucMaterialColors * materials = new cmbNucMaterialColors;
  mainWindow->NuclearCore->getPinLibrary()->setKeepGoingAsRename();

  for( int i = 0; i < fileNames.count(); ++i)
  {
    materials->clear();

    std::vector<PinCell*> pincells;
    
    xmlFileReader::read(fileNames[i].toStdString(), pincells, materials);
    for(unsigned int j = 0; j < pincells.size(); ++j)
    {
      this->addPin(pincells[j], tmpD, junk);
    }
  }
  delete materials;
  return true;
}

bool cmbNucImporter::importXMLDucts()
{
  // Use cached value for last used directory if there is one,
  // or default to the user's home dir if not.
  QSettings settings("CMBNuclear", "CMBNuclear");
  QDir dir = settings.value("cache/lastDir", QDir::homePath()).toString();

  QStringList fileNames =
  QFileDialog::getOpenFileNames(mainWindow,
                                "Open File...",
                                dir.path(),
                                "RGG XML File (*.RXF)");
  if(fileNames.count()==0)
  {
    return false;
  }

  // Cache the directory for the next time the dialog is opened
  QFileInfo info(fileNames[0]);
  settings.setValue("cache/lastDir", info.dir().path());
  std::map<std::string, std::string> junk;

  double tmpD = 10;
  assert(mainWindow->NuclearCore->HasDefaults());
  mainWindow->NuclearCore->GetDefaults()->getHeight(tmpD);

  double ductThick[] = {10.0, 10.0};
  mainWindow->NuclearCore->GetDefaults()->getDuctThickness(ductThick[0],ductThick[1]);

  cmbNucMaterialColors * materials = new cmbNucMaterialColors;

  cmbNucDuctLibrary * dl = mainWindow->NuclearCore->getDuctLibrary();

  for( int i = 0; i < fileNames.count(); ++i)
  {
    materials->clear();

    std::vector<DuctCell*> ductcells;

    xmlFileReader::read(fileNames[i].toStdString(), ductcells, materials);
    for(unsigned int j = 0; j < ductcells.size(); ++j)
    {
      this->addDuct(ductcells[j], tmpD, ductThick, junk);
    }
  }
  delete materials;
  return true;
}

bool cmbNucImporter::importInpFile()
{
  // Use cached value for last used directory if there is one,
  // or default to the user's home dir if not.
  QSettings settings("CMBNuclear", "CMBNuclear");
  QDir dir = settings.value("cache/lastDir", QDir::homePath()).toString();

  QStringList fileNames =
  QFileDialog::getOpenFileNames(mainWindow,
                                "Open File...",
                                dir.path(),
                                "INP Files (*.inp)");
  if(fileNames.count()==0)
  {
    return false;
  }
  
  // Cache the directory for the next time the dialog is opened
  QFileInfo info(fileNames[0]);
  settings.setValue("cache/lastDir", info.dir().path());
  int numExistingAssy = mainWindow->NuclearCore->GetNumberOfAssemblies();
  bool need_to_use_assem = false;

  mainWindow->NuclearCore->getPinLibrary()->resetConflictResolution();

  for( int i = 0; i < fileNames.count(); ++i)
  {
    inpFileReader freader;
    switch(freader.open(fileNames[i].toStdString()))
    {
      case inpFileReader::ASSEMBLY_TYPE:
      {
        if(!mainWindow->InputsWidget->isEnabled() || mainWindow->InputsWidget->onlyMeshLoaded())
        {
          mainWindow->doClearAll(true);
        }
        QFileInfo finfo(fileNames[i]);
        std::string label = finfo.completeBaseName().toStdString();
        cmbNucAssembly *assembly = new cmbNucAssembly();
        assembly->label = label;
        if(!freader.read(*assembly, mainWindow->NuclearCore->getPinLibrary(),
                         mainWindow->NuclearCore->getDuctLibrary()))
        {
          QMessageBox msgBox;
          msgBox.setText("Invalid INP file");
          msgBox.setInformativeText(fileNames[i]+" could not be readed.");
          msgBox.exec();
          delete assembly;
          return false;
        }
        if(mainWindow->InputsWidget->isEnabled() &&
           assembly->IsHexType() != mainWindow->NuclearCore->IsHexType())
        {
          QMessageBox msgBox;
          msgBox.setText("Not the same type");
          msgBox.setInformativeText(fileNames[i]+" is not the same geometry type as current core.");
          msgBox.exec();
          delete assembly;
          return false;
        }
        int acolorIndex = numExistingAssy +
        mainWindow->NuclearCore->GetNumberOfAssemblies() % numAssemblyDefaultColors;

        QColor acolor(defaultAssemblyColors[acolorIndex][0],
                      defaultAssemblyColors[acolorIndex][1],
                      defaultAssemblyColors[acolorIndex][2]);
        assembly->SetLegendColor(acolor);
        bool need_to_calc_defaults = mainWindow->NuclearCore->GetNumberOfAssemblies() == 0;
        mainWindow->NuclearCore->AddAssembly(assembly);
        if(need_to_calc_defaults)
        {
          mainWindow->NuclearCore->calculateDefaults();
          switch(assembly->getLattice().GetGeometryType())
          {
            case RECTILINEAR:
              mainWindow->NuclearCore->setGeometryLabel("Rectangular");
              break;
            case HEXAGONAL:
              mainWindow->NuclearCore->setGeometryLabel("HexFlat");
              break;
          }
        }
        else
        {
          assembly->setFromDefaults( mainWindow->NuclearCore->GetDefaults() );
        }
        need_to_use_assem = true;

        if( mainWindow->NuclearCore->getLattice().GetGeometrySubType() & ANGLE_60 &&
           mainWindow->NuclearCore->getLattice().GetGeometrySubType() & VERTEX )
        {
          assembly->getLattice().setFullCellMode(Lattice::HEX_FULL);
        }
        else
        {
          assembly->getLattice().setFullCellMode(Lattice::HEX_FULL_30);
        }
        assembly->adjustRotation();
        break;
      }
      case inpFileReader::CORE_TYPE:
        // clear old assembly
        if(!mainWindow->checkFilesBeforePreceeding()) return false;
        mainWindow->doClearAll();
        mainWindow->PropertyWidget->setObject(NULL, NULL);
        mainWindow->PropertyWidget->setAssembly(NULL);
        if(!freader.read(*(mainWindow->NuclearCore)))
        {
          QMessageBox msgBox;
          msgBox.setText("Invalid INP file");
          msgBox.setInformativeText(fileNames[i]+" could not be readed.");
          msgBox.exec();
          mainWindow->unsetCursor();
          return false;
        }
        mainWindow->NuclearCore->setAndTestDiffFromFiles(false);
        mainWindow->NuclearCore->SetLegendColorToAssemblies(numAssemblyDefaultColors,
                                                      defaultAssemblyColors);
        mainWindow->PropertyWidget->resetCore(mainWindow->NuclearCore);

        for(unsigned int j = 0; j < mainWindow->NuclearCore->GetNumberOfAssemblies(); ++j)
        {
          mainWindow->NuclearCore->GetAssembly(j)->adjustRotation();
          if( mainWindow->NuclearCore->getLattice().GetGeometrySubType() & ANGLE_60 &&
             mainWindow->NuclearCore->getLattice().GetGeometrySubType() & VERTEX )
          {
            mainWindow->NuclearCore->GetAssembly(j)->getLattice().setFullCellMode(Lattice::HEX_FULL);
          }
          else
          {
            mainWindow->NuclearCore->GetAssembly(j)->getLattice().setFullCellMode(Lattice::HEX_FULL_30);
          }
        }
        break;
      default:
        qDebug() << "could not open" << fileNames[i];
    }
  }
  int numNewAssy = mainWindow->NuclearCore->GetNumberOfAssemblies() - numExistingAssy;
  if(numNewAssy)
  {
    mainWindow->PropertyWidget->resetCore(mainWindow->NuclearCore);
  }

  // update data colors
  mainWindow->updateCoreMaterialColors();

  // In case the loaded core adds new materials
  mainWindow->InputsWidget->updateUI(numNewAssy);
  return true;
}

void cmbNucImporter::addPin(PinCell * pc, double dh, std::map<std::string, std::string> & nc)
{
  pc->setHeight(dh);
  //adjust materials
  if(pc->cellMaterialSet())
  {
    QPointer<cmbNucMaterial> cm = pc->getCellMaterial();
    QPointer<cmbNucMaterial> tmpm = this->getMaterial(pc->getCellMaterial());
    assert(tmpm != NULL);
    assert(tmpm != cmbNucMaterialColors::instance()->getUnknownMaterial());
    pc->setCellMaterial(tmpm);
  }
  int layers = pc->GetNumberOfLayers();
  for(int k = 0; k < layers; ++k)
  {
    QPointer<cmbNucMaterial> tmpm = this->getMaterial(pc->Material(k));
    assert(tmpm != NULL);
    assert(tmpm != cmbNucMaterialColors::instance()->getUnknownMaterial());
    pc->SetMaterial(k, tmpm);
  }
  mainWindow->NuclearCore->getPinLibrary()->addPin(&pc, nc);
}

void cmbNucImporter::addDuct(DuctCell * dc, double dh, double dt[2], std::map<std::string, std::string> & nc)
{
  dc->setDuctThickness(dt[0], dt[1]);
  dc->setLength(dh);

  //adjust materials
  for(unsigned int k = 0; k < dc->numberOfDucts(); ++k)
  {
    Duct * d = dc->getDuct(k);
    int layers = d->NumberOfLayers();
    for(int l = 0; l < layers; ++l)
    {
      QPointer<cmbNucMaterial> tmpm = this->getMaterial(d->getMaterial(l));
      assert(tmpm != NULL);
      assert(tmpm != cmbNucMaterialColors::instance()->getUnknownMaterial());
      d->setMaterial(l, tmpm);
      if(mainWindow->NuclearCore->IsHexType())
      {
        double * tmp = d->getNormThick(l);
        tmp[1] = tmp[0];
      }
    }
  }
  mainWindow->NuclearCore->getDuctLibrary()->addDuct(dc, nc);
}

QPointer<cmbNucMaterial> cmbNucImporter::getMaterial(QPointer<cmbNucMaterial> cm)
{
  cmbNucMaterialColors * matColorMap = cmbNucMaterialColors::instance();
  if( matColorMap->nameUsed(cm->getName()))
  {
    return matColorMap->getMaterialByName(cm->getName());
  }
  else if(matColorMap->labelUsed(cm->getLabel()))
  {
    return matColorMap->getMaterialByLabel(cm->getLabel());
  }
  else
  {
    return  matColorMap->AddMaterial(cm->getName(),
                                     cm->getLabel(),
                                     cm->getColor());
  }
}
