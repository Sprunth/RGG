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

std::string cmbNucPinLibrary::addPin(PinCell* pc)
{
  if(pc == NULL) return "";
  std::string label = pc->getLabel();
  PinCells.push_back(pc);
  return label;
}

void cmbNucPinLibrary::removePincell(PinCell* pc)
{
  if(pc == NULL) return;
  bool found = false;
  for(size_t i = 0; i < this->PinCells.size(); i++)
  {
    if(this->PinCells[i] == pc)
    {
      found = true;
      this->PinCells.erase(this->PinCells.begin() + i);
      break;
    }
  }
  if(found)
  {
    this->Connection->pinRemoved(pc);
    delete pc;
  }
}

PinCell* cmbNucPinLibrary::GetPinCell(const std::string &l)
{
  for(size_t i = 0; i < this->PinCells.size(); i++)
  {
    PinCell *pincell = this->GetPinCell(i);
    std::string tmp = pincell->getLabel();
    if(tmp == l)
    {
      return this->PinCells[i];
    }
  }
  return NULL;
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
