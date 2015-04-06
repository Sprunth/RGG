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
  void libraryChanged();
};

class cmbNucDuctLibrary: public AssyPartObj
{
public:
  cmbNucDuctLibrary();
  virtual ~cmbNucDuctLibrary();

  cmbNucDuctLibraryConnection * GetConnection() {return this->Connection; }

  //takes ownership when successful. Returns true if successful.
  bool addDuct(DuctCell * dc);
  bool addDuct(DuctCell ** dc); //Adds a duct if it is unique.  if not returns the previous added duct
  bool addDuct(DuctCell * dc,
               std::map<std::string, std::string> & nameChange );
  void removeDuctcell(DuctCell* dc);
  DuctCell* GetDuctCell(const std::string &name);
  DuctCell* GetDuctCell(int pc) const;
  std::size_t GetNumberOfDuctCells() const;
  void fillList(QStringList & l);
  bool nameConflicts(std::string n) const;
  bool replaceName(std::string oldN, std::string newN);

  cmbNucDuctLibrary * clone();

protected:
  std::vector<DuctCell*> DuctCells;
  std::map<std::string, size_t> NameToDuct;
  cmbNucDuctLibraryConnection * Connection;
};

#endif
