#include "cmbNucPinLibrary.h"

cmbNucPinLibrary::cmbNucPinLibrary()
{
  this->Connection = new cmbNucPinLibraryConnection;
  this->Connection->v = this;
  this->resetConflictResolution();
}

cmbNucPinLibrary::~cmbNucPinLibrary()
{
  AssyPartObj::deleteObjs(this->PinCells);
  delete Connection;
}

cmbNucPinLibrary::ConflictMode
cmbNucPinLibrary::testPinConflicts(PinCell* pc) const
{
  std::string label = pc->getLabel();
  std::string name = pc->getName();
  bool nc = nameConflicts(name);
  bool lc = labelConflicts(label);
  if(nc && lc) return Both_Conflict;
  if(nc) return Name_Conflict;
  if(lc) return Label_Conflict;
  return No_Conflict;
}

bool cmbNucPinLibrary::labelConflicts(std::string l) const
{
  return this->LabelToPin.find(l) != this->LabelToPin.end();
}

bool cmbNucPinLibrary::nameConflicts(std::string n) const
{
  return this->NameToPin.find(n) != this->NameToPin.end();
}

cmbNucPinLibrary::PinAddedMode
cmbNucPinLibrary::addPin(PinCell ** in, AddMode mode)
{
  if(in == NULL) return PinNull;
  PinCell * pc = *in;
  if(pc == NULL) return PinNull; //Ignore null
  ConflictMode cm = this->testPinConflicts(pc);
  if(cm == No_Conflict)
  {
    int loc = PinCells.size();
    PinCells.push_back(pc);
    std::string label = pc->getLabel();
    std::string name = pc->getName();
    NameToPin[name] = loc;
    LabelToPin[label] = loc;
    this->Connection->libraryChanged();
    QObject::connect(pc->GetConnection(), SIGNAL(Changed()),
                     this->Connection, SIGNAL(libraryChanged()));
    return PinAdded;
  }
  else if(cm == Both_Conflict)
  {
    PinCell * other = PinCells[LabelToPin.find(pc->getLabel())->second];
    PinAddedMode am;
    switch(mode)
    {
      case Replace:
        other->fill(pc);
        am = PinAdded;
        break;
      case KeepOriginal:
        am = PinExists;
        break;
    }
    delete pc;
    *in = other;
    return am;
  }
  return PinAddFailed;
}

cmbNucPinLibrary::PinAddedMode
cmbNucPinLibrary::addPin(PinCell** pc, bool keepEqualent,
                         std::map<std::string, std::string> & nameChange)
{
  if(pc == NULL) return PinNull;
  if(*pc == NULL) return PinNull;
  PinAddedMode resultMode = PinAdded;

  ConflictMode mode = this->testPinConflicts(*pc);
  std::string oldLabel = (*pc)->getLabel();
  bool isEq = false;

  if(mode == Both_Conflict && !keepEqualent && (isEq = isEquivelence(*pc)))
  {
    if(oldLabel != (*pc)->getLabel())
    {
      nameChange[oldLabel] = (*pc)->getLabel();
    }
    return this->addPin(pc, KeepOriginal);
  }
  else if(mode)
  {
    resultMode = PinRenamed;
    //rename loop
    std::string n = (*pc)->getName();
    int count = 0;
    while(this->nameConflicts(n))
    {
      n = (QString((*pc)->getName().c_str()) + QString::number(count++)).toStdString();
    }
    //relabel loop
    std::string l = (*pc)->getLabel();
    count = 0;
    while(this->labelConflicts(l))
    {
      l = (QString((*pc)->getLabel().c_str()) + QString::number(count++)).toStdString();
    }
    (*pc)->setName(n);
    if((*pc)->getLabel() != l)
    {
      OldLabelToNewPincell[(*pc)->getLabel()] = *pc;
      nameChange[(*pc)->getLabel()] = l;
    }
    (*pc)->setLabel(l);
  }
  if(this->addPin(pc, this->conflictResMode))
  {
    return resultMode;
  }
  else
  {
    return PinAddFailed;
  }
}

void cmbNucPinLibrary::resetConflictResolution()
{
  this->keepGoing = false;
  this->renamePin = false;
  this->conflictResMode = KeepOriginal;
}

void cmbNucPinLibrary::removePincell(PinCell* pc)
{
  if(pc == NULL) return;

  std::map<std::string, size_t>::iterator ni = NameToPin.find(pc->getName());
  std::map<std::string, size_t>::iterator li = LabelToPin.find(pc->getLabel());
  if(ni == NameToPin.end()) return;
  this->PinCells.erase(this->PinCells.begin() + ni->second);
  for(size_t i = ni->second; i < this->PinCells.size(); ++i)
  {
    PinCell * pci = this->PinCells[i];
    NameToPin[pci->getName()] = i;
    LabelToPin[pci->getLabel()] = i;
  }
  NameToPin.erase(ni);
  LabelToPin.erase(li);
  this->Connection->pinRemoved(pc);
  this->Connection->libraryChanged();
  delete pc;
}

PinCell* cmbNucPinLibrary::GetPinCell(const std::string &l)
{
  std::map<std::string, size_t>::iterator li = LabelToPin.find(l);
  if(li == LabelToPin.end()) return NULL;
  return this->PinCells[li->second];
}

PinCell* cmbNucPinLibrary::GetPinCell(int pc) const
{
  if(static_cast<size_t>(pc) < this->PinCells.size())
  {
    return this->PinCells[pc];
  }
  return NULL;
}

std::size_t cmbNucPinLibrary::GetNumberOfPinCells() const
{
  return this->PinCells.size();
}

void cmbNucPinLibrary::fillList(QStringList & l)
{
  l.append("Empty Cell (xx)");
  for(std::vector<PinCell*>::const_iterator it = this->PinCells.begin(); it != this->PinCells.end(); ++it)
  {
    PinCell *pincell = *it;
    std::string tmp = pincell->getName() + " (" + pincell->getLabel() + ")";
    l.append(tmp.c_str());
  }
}

QString cmbNucPinLibrary::extractLabel(QString const& el) const
{
  QString seperator("(");
  QStringList ql = el.split( seperator );
  return ql[1].left(ql[1].size() -1);
}

void cmbNucPinLibrary::replaceLabel(std::string oldL, std::string newL)
{
  std::map<std::string, size_t>::iterator li = LabelToPin.find(oldL);
  if(li == LabelToPin.end()) return;
  LabelToPin[newL] = li->second;
  LabelToPin.erase(li);
  
}

void cmbNucPinLibrary::replaceName(std::string oldN, std::string newN)
{
  std::map<std::string, size_t>::iterator ni = NameToPin.find(oldN);
  if(ni == NameToPin.end()) return;
  NameToPin[newN] = ni->second;
  NameToPin.erase(ni);
}

cmbNucPinLibrary * cmbNucPinLibrary::clone() const
{
  cmbNucPinLibrary * result = new cmbNucPinLibrary();
  for(std::vector<PinCell*>::const_iterator iter = this->PinCells.begin(); iter != this->PinCells.end(); ++iter)
  {
    PinCell * pc = new PinCell();
    pc->fill(*iter);
    result->addPin(&pc, KeepOriginal);
  }
  return result;
}

void cmbNucPinLibrary::clearRenameEquivelence()
{
  OldLabelToNewPincell.clear();
}

bool cmbNucPinLibrary::isEquivelence(PinCell* pc) const
{
  PinCell * other = PinCells[LabelToPin.find(pc->getLabel())->second];
  if(*other == *pc) return true;
  std::map<std::string, PinCell*>::const_iterator i = this->OldLabelToNewPincell.find(pc->getLabel());
  if(i!=this->OldLabelToNewPincell.end() && *(i->second) == *pc)
  {
    pc->setLabel(i->second->getLabel());
    pc->setName(i->second->getName());
    return true;
  }
  return false;
}
