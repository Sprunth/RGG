#ifndef cmbNucPinLibrary_H
#define cmbNucPinLibrary_H

#include <string>
#include <vector>
#include <map>
#include <QColor>
#include <QObject>
#include <QPointer>
#include <QStringList>

#include "cmbNucPartDefinition.h"
#include "cmbNucPinCell.h"

class cmbNucPinLibrary;
class cmbNucPinLibraryConnection: public QObject
{
  Q_OBJECT
  friend class cmbNucPinLibrary;
private:
  cmbNucPinLibrary * v;
signals:
  void pinRemoved(PinCell *);
  void labelChanged(QString o, QString n);
};

class cmbNucPinLibrary: public AssyPartObj
{
public:
  enum AddMode{KeepOriginal, Replace};
  enum ConflictMode{No_Conflict = 0, Name_Conflict, Label_Conflict, Both_Conflict};
  cmbNucPinLibrary();
  ~cmbNucPinLibrary();

  cmbNucPinLibraryConnection * GetConnection() {return this->Connection; }

  //takes ownership when successful. Returns true if successful.
  bool addPin(PinCell** pc, AddMode mode);
  void removePincell(PinCell* pc);
  PinCell* GetPinCell(const std::string &label);
  PinCell* GetPinCell(int pc) const;
  std::size_t GetNumberOfPinCells() const;
  void fillList(QStringList & l);
  QString extractLabel(QString const& el) const;
  ConflictMode testPinConflicts(PinCell* pc) const;
  bool labelConflicts(std::string l) const;
  bool nameConflicts(std::string n) const;
  void replaceLabel(std::string oldL, std::string newL);
  void replaceName(std::string oldN, std::string newN);

protected:
  std::vector<PinCell*> PinCells;
  std::map<std::string, size_t> NameToPin;
  std::map<std::string, size_t> LabelToPin;
  cmbNucPinLibraryConnection * Connection;
};

#endif