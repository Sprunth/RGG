#include "cmbNucInpExporter.h"
#include "inpFileIO.h"
#include "cmbNucCore.h"
#include "cmbNucAssembly.h"
#include "cmbNucAssemblyLink.h"
#include "cmbNucDuctLibrary.h"

#include <QFileInfo>
#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>

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
    if(assembly != NULL)
    {
        assembly->setPath(coreinfo.dir().absolutePath().toStdString());
        std::set< Lattice::CellDrawMode > const& forms = iter->second;
        cmbNucAssembly* assemblyClone = assembly->clone(pl, dl);
        this->exportInpFile(assemblyClone, false, forms);
        delete assemblyClone;
    }
    else
    {
        // check to see if iter->first is a link
        // if it is a link but there is no corresponding target assembly with the same mode
        // clone the link target assembly and write it out with the link mode

        cmbNucAssemblyLink * correspondingLink = NuclearCore->GetAssemblyLink(iter->first);;
//        for(std::vector< cmbNucAssemblyLink* >::const_iterator link_iter = NuclearCore->AssemblyLinks.begin(); link_iter != NuclearCore->AssemblyLinks.end(); ++link_iter)
//        {
//            if (link_iter->getLabel().compare(iter->getLabel()) == 0)
//            {
//                correspondingLink = *link_iter;
//                break;
//            }
//        }
        if (correspondingLink == NULL)
        {
            continue;
        }
        // correspondingLink is the link that this current iter is linked with
        // this assumes only 1 link can be had for any label

        //std::string linkTargetLabel = correspondingLink->link.getLabel();
        std::string linkTargetLabel = correspondingLink->getLink()->getLabel();
        cmbNucAssembly* linkTargetAssy = NuclearCore->GetAssembly(linkTargetLabel);

        if (linkTargetAssy == NULL)
        {
            continue;
        }

        // now find the assembly that goes along with the linkTargetLabel
//        for(std::vector< cmbNucAssembly* >::const_iterator assy_iter = NuclearCore->AssemblyLinks.begin(); assy_iter != NuclearCore->AssemblyLinks.end(); ++assy_iter)
//        {
//            if (assy_iter->getLabel().compare(linkTargetLabel) == 0)
//            {
//                linkTargetAssy = *assy_iter;
//                break;
//            }
//        }

        // same as above. Maybe worth merging?
        linkTargetAssy->setPath(coreinfo.dir().absolutePath().toStdString());
        std::set< Lattice::CellDrawMode > const& forms = iter->second;
        cmbNucAssembly* assemblyClone = linkTargetAssy->clone(pl, dl);
        assemblyClone->setLabel(iter->first);
        this->exportInpFile(assemblyClone, false, forms);
        delete assemblyClone;
    }
  }
  if(this->NuclearCore->changeSinceLastGenerate())
  {
    inpFileWriter::write(coreName.toStdString(), *(this->NuclearCore));
  }

  return true;
}

bool cmbNucInpExporter
::exportCylinderINPFile(QString filename, QString random)
{
  std::vector< cmbNucAssembly* > used = this->NuclearCore->GetUsedAssemblies();
  if (used.empty())
  {
    QMessageBox msgBox;
    msgBox.setText("Could not export cylinder");
    msgBox.setInformativeText("In order to generate a outer "
                              "cylinder, the core must use at least"
                              " one assyembly.");
    msgBox.exec();
    return false;
  }
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
  cmbNucAssembly * temp = used[0]->clone(pl,dl);
  QString fname = QString(temp->getLabel().c_str()).toLower() + random + ".inp";
  fname = fname.toLower();
  QString fullPath =fi.dir().absolutePath();
  qDebug() << fullPath << "   " << fname;
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
      pc.insert(assy->GetPinCell(static_cast<int>(i)));
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
    std::string const& fname = assy->getFileName(mode, forms.size()+1);

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
      inpFileWriter::write(fname, *assy, this->NuclearCore->getBoundaryLayers(), false, isCylinder);
    }
  }
  assy->removeOldTransforms(0);
  return true;
}
