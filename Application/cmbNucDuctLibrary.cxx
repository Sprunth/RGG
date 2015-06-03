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
  AssyPartObj::deleteObjs(this->DuctCells);
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
  this->Connection->libraryChanged();
  QObject::connect(dc->GetConnection(), SIGNAL(Changed()),
                   this->Connection, SIGNAL(libraryChanged()));
  return true;
}

bool cmbNucDuctLibrary
::addDuct(DuctCell ** dc)
{
  std::string original_name = (*dc)->getName();
  for(unsigned int i = 0; i < DuctCells.size(); ++i)
  {
    DuctCell * other = DuctCells[i];
    (*dc)->setName(other->getName());
    if(**dc == *other)
    {
      delete *dc;
      *dc = other;
      return true;
    }
  }
  (*dc)->setName(original_name);
  return this->addDuct(*dc);
}

bool cmbNucDuctLibrary
::addDuct(DuctCell * dc, std::map<std::string, std::string> & nameChange)
{
  if(dc == NULL) return false;
  //rename loop
  std::string n = (dc)->getName();
  int count = 0;
  //check equivelence
  while(this->nameConflicts(n))
  {
    n = (QString((dc)->getName().c_str()) + QString::number(count++)).toStdString();
  }
  if(n != (dc)->getName()) nameChange[(dc)->getName()] = n;
  (dc)->setName(n);
  return this->addDuct(dc);
}

void cmbNucDuctLibrary
::removeDuctcell(DuctCell* dc)
{
  if(dc == NULL) return;
  std::string name = dc->getName();
  std::map<std::string, size_t>::iterator iter = this->NameToDuct.find(name);
  if(iter == this->NameToDuct.end()) return;
  size_t second = iter->second;
  assert(dc == this->DuctCells[second]);
  this->DuctCells.erase(this->DuctCells.begin() + second);
  this->NameToDuct.erase(iter);
  for(size_t i = second; i < this->DuctCells.size(); ++i)
  {
    DuctCell * dci = this->DuctCells[i];
    NameToDuct[dci->getName()] = i;
  }
  this->Connection->DuctRemoved(dc);
  this->Connection->libraryChanged();
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
  if(static_cast<std::size_t>(pc) >= this->DuctCells.size()) return NULL;
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
  if(iter == this->NameToDuct.end()) return false;

  this->NameToDuct[newN] = iter->second;
  this->NameToDuct.erase(iter);
  return true;
}

cmbNucDuctLibrary * cmbNucDuctLibrary
::clone()
{
  cmbNucDuctLibrary * result = new cmbNucDuctLibrary();

  for(std::vector<DuctCell*>::const_iterator it = this->DuctCells.begin();
      it != this->DuctCells.end(); ++it)
  {
    DuctCell * dc = new DuctCell();
    dc->fill(*it);
    result->addDuct(dc);
  }

  return result;
}

void cmbNucDuctLibrary
::removeFakeBoundaryLayer(std::string blname)
{
  for(std::vector<DuctCell*>::const_iterator it = this->DuctCells.begin();
      it != this->DuctCells.end(); ++it)
  {
    DuctCell * dc = *it;
    dc->removeFakeBoundaryLayer(blname);
  }
}
