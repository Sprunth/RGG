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
  void libraryChanged();
};

class cmbNucPinLibrary: public AssyPartObj
{
public:
  enum AddMode{KeepOriginal, Replace};
  enum ConflictMode{No_Conflict = 0, Name_Conflict, Label_Conflict, Both_Conflict};
  enum PinAddedMode{PinAddFailed = 0, PinNull, PinRenamed, PinExists, PinAdded};
  cmbNucPinLibrary();
  virtual ~cmbNucPinLibrary();

  cmbNucPinLibraryConnection * GetConnection() {return this->Connection; }

  //takes ownership when successful. Returns true if successful.
  PinAddedMode addPin(PinCell** pc, AddMode mode);
  PinAddedMode addPin(PinCell** pc, bool keepEqualent, std::map<std::string, std::string> & nameChange);
  void resetConflictResolution();
  void setKeepGoingAsRename()
  {
    this->keepGoing = true;
    this->renamePin = true;
  }
  void removePincell(PinCell* pc);
  PinCell* GetPinCell(const std::string& label);
  PinCell* GetPinCell(int pc) const;
  std::size_t GetNumberOfPinCells() const;
  void fillList(QStringList & l);
  QString extractLabel(QString const& el) const;
  ConflictMode testPinConflicts(PinCell* pc) const;
  bool labelConflicts(const std::string& l) const;
  bool nameConflicts(const std::string& n) const;
  void replaceLabel(const std::string& oldL, const std::string& newL);
  void replaceName(const std::string& oldN, const std::string& newN);

  cmbNucPinLibrary * clone() const;

  void clearRenameEquivelence();

  bool isEquivelence(PinCell* pc) const;

  void removeFakeBoundaryLayer(std::string blname);

protected:
  std::vector<PinCell*> PinCells;
  std::map<std::string, PinCell*> OldLabelToNewPincell;
  std::map<std::string, size_t> NameToPin;
  std::map<std::string, size_t> LabelToPin;
  cmbNucPinLibraryConnection * Connection;
  bool keepGoing, renamePin;
  AddMode conflictResMode;

  void setLabelLocal(std::string l, size_t i);
  void setNameLocal(std::string n, size_t i);
  std::map<std::string, size_t>::iterator findLabel(std::string l);
  std::map<std::string, size_t>::iterator findName(std::string n);
  std::map<std::string, size_t>::const_iterator findLabel(std::string l) const;
  std::map<std::string, size_t>::const_iterator findName(std::string n) const;
};

#endif
