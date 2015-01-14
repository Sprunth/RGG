#ifndef cmbNucDuctLibrary_H
#define cmbNucDuctLibrary_H

#include <string>
#include <vector>
#include <map>
#include <QColor>
#include <QObject>
#include <QPointer>
#include <QStringList>

#include "cmbNucPartDefinition.h"
#include "cmbNucDuctCell.h"

class cmbNucDuctLibrary;

class cmbNucDuctLibraryConnection: public QObject
{
  Q_OBJECT
  friend class cmbNucDuctLibrary;
private:
  cmbNucDuctLibrary * v;
signals:
  void DuctRemoved(DuctCell *);
  void labelChanged(QString n);
};

class cmbNucDuctLibrary: public AssyPartObj
{
public:
  cmbNucDuctLibrary();
  ~cmbNucDuctLibrary();

  cmbNucDuctLibraryConnection * GetConnection() {return this->Connection; }

  //takes ownership when successful. Returns true if successful.
  bool addDuct(DuctCell* dc);
  void removeDuctcell(DuctCell* dc);
  DuctCell* GetDuctCell(const std::string &name);
  DuctCell* GetDuctCell(int pc) const;
  std::size_t GetNumberOfDuctCells() const;
  void fillList(QStringList & l);
  bool nameConflicts(std::string n) const;
  bool replaceName(std::string oldN, std::string newN);

protected:
  std::vector<DuctCell*> DuctCells;
  std::map<std::string, size_t> NameToDuct;
  cmbNucDuctLibraryConnection * Connection;
};

#endif
