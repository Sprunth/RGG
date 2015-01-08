#include "cmbNucPinLibrary.h"

cmbNucPinLibrary::cmbNucPinLibrary()
{
  this->Connection = new cmbNucPinLibraryConnection;
  this->Connection->v = this;
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
  return LabelToPin.find(l) != LabelToPin.end();
}

bool cmbNucPinLibrary::nameConflicts(std::string n) const
{
  return NameToPin.find(n) != NameToPin.end();
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
        delete pc;
        *in = PinCells[LabelToPin.find(pc->getLabel())->second];
        return true;
    }
  }
  return false;
}

void cmbNucPinLibrary::removePincell(PinCell* pc)
{
  if(pc == NULL) return;
  bool found = false;

  std::map<std::string, size_t>::iterator ni = NameToPin.find(pc->getName());
  std::map<std::string, size_t>::iterator li = LabelToPin.find(pc->getLabel());
  if(ni == NameToPin.end()) return;
  this->PinCells.erase(this->PinCells.begin() + ni->second);
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
