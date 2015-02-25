#include "cmbNucPinLibrary.h"
#include "cmbNucConflictDialog.h"

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

bool cmbNucPinLibrary::addPin(PinCell ** in, AddMode mode)
{
  if(in == NULL) return true;
  PinCell * pc = *in;
  if(pc == NULL) return true; //Ignore null
  ConflictMode cm = this->testPinConflicts(pc);
  if(cm == No_Conflict)
  {
    int loc = PinCells.size();
    PinCells.push_back(pc);
    std::string label = pc->getLabel();
    std::string name = pc->getName();
    NameToPin[name] = loc;
    LabelToPin[label] = loc;
    return true;
  }
  else if(cm == Both_Conflict)
  {
    switch(mode)
    {
      case Replace:
        PinCells[LabelToPin.find(pc->getLabel())->second]->fill(pc);
      case KeepOriginal:
      {
        std::string label = pc->getLabel();
        delete pc;
        *in = NULL;
        *in = PinCells[LabelToPin.find(label)->second];
        return true;
      }
    }
  }
  return false;
}

bool cmbNucPinLibrary::addPin(PinCell** pc, std::map<std::string, std::string> & nameChange)
{
  if(pc == NULL) return false;
  if(*pc == NULL) return false;
  if(this->keepGoing)
  {
    if(this->testPinConflicts(*pc) && this->renamePin)
    {
      //rename loop
      std::string n = (*pc)->getName();
      int count = 0;
      while(this->nameConflicts(n))
      {
        n = (QString(n.c_str()) + QString::number(count++)).toStdString();
      }
      //relabel loop
      std::string l = (*pc)->getLabel();
      count = 0;
      while(this->labelConflicts(l))
      {
        l = (QString(l.c_str()) + QString::number(count++)).toStdString();
      }
      (*pc)->setName(n);
      if((*pc)->getLabel() != l) nameChange[(*pc)->getLabel()] = l;
      (*pc)->setLabel(l);
    }
  }
  else
  {
    this->conflictResMode = cmbNucPinLibrary::KeepOriginal;
    if(this->testPinConflicts(*pc))
    {
      std::string old_label = (*pc)->getLabel();
      cmbNucConflictDialog cncd(NULL, this, *pc);
      this->conflictResMode = static_cast<cmbNucPinLibrary::AddMode>(cncd.exec());
      this->renamePin = cncd.rename();
      this->keepGoing = cncd.keepGoing();
      std::string new_label = (*pc)->getLabel();
      if(new_label != old_label) nameChange[old_label] = new_label;
    }
  }
  return this->addPin(pc, this->conflictResMode);
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
  bool found = false;

  std::map<std::string, size_t>::iterator ni = NameToPin.find(pc->getName());
  std::map<std::string, size_t>::iterator li = LabelToPin.find(pc->getLabel());
  if(ni == NameToPin.end()) return;
  this->PinCells.erase(this->PinCells.begin() + ni->second);
  for(size_t i = ni->second; i < this->PinCells.size(); ++i)
  {
    PinCell * pc = this->PinCells[i];
    NameToPin[pc->getName()] = i;
    LabelToPin[pc->getLabel()] = i;
  }
  NameToPin.erase(ni);
  LabelToPin.erase(li);
  this->Connection->pinRemoved(pc);
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
