#include "cmbNucDuctLibrary.h"

cmbNucDuctLibrary
::cmbNucDuctLibrary()
{
  this->Connection = new cmbNucDuctLibraryConnection;
  this->Connection->v = this;
}

cmbNucDuctLibrary
::~cmbNucDuctLibrary()
{
  delete this->Connection;
}

bool cmbNucDuctLibrary
::addDuct(DuctCell* dc)
{
  if(dc == NULL) return false;
  std::string name = dc->getName();
  if(this->nameConflicts(name)) return false;
  size_t s = this->DuctCells.size();
  this->DuctCells.push_back(dc);
  this->NameToDuct[name] = s;
  return true;
}

void cmbNucDuctLibrary
::removeDuctcell(DuctCell* dc)
{
  if(dc == NULL) return;
  std::string name = dc->getName();
  std::map<std::string, size_t>::iterator iter = this->NameToDuct.find(name);
  if(iter == this->NameToDuct.end()) return;
  this->DuctCells.erase(this->DuctCells.begin() + iter->second);
  this->NameToDuct.erase(iter);
  this->Connection->DuctRemoved(dc);
  delete dc;
}

DuctCell* cmbNucDuctLibrary
::GetDuctCell(const std::string &name)
{
  std::map<std::string, size_t>::const_iterator iter = this->NameToDuct.find(name);
  if(iter == this->NameToDuct.end()) return NULL;
  return this->DuctCells[iter->second];
}

DuctCell* cmbNucDuctLibrary
::GetDuctCell(int pc) const
{
  if(pc >= this->DuctCells.size()) return NULL;
  return this->DuctCells[pc];
}

std::size_t cmbNucDuctLibrary
::GetNumberOfDuctCells() const
{
  return this->DuctCells.size();
}

void cmbNucDuctLibrary
::fillList(QStringList & l)
{
  for(std::vector<DuctCell*>::const_iterator it = this->DuctCells.begin(); it != this->DuctCells.end(); ++it)
  {
    DuctCell *dc = *it;
    std::string tmp = dc->getName();
    l.append(tmp.c_str());
  }
}

bool cmbNucDuctLibrary
::nameConflicts(std::string n) const
{
  return this->NameToDuct.find(n) != this->NameToDuct.end();
}

bool cmbNucDuctLibrary
::replaceName(std::string oldN, std::string newN)
{
  if(this->nameConflicts(newN)) return false;
  std::map<std::string, size_t>::iterator iter = this->NameToDuct.find(oldN);
  if(iter != this->NameToDuct.end()) return false;

  this->NameToDuct[newN] = iter->second;
  this->NameToDuct.erase(iter);
  return true;
}
