#ifndef cmbNucPinLibrary_H
#define cmbNucPinLibrary_H

#include <string>
#include <vector>
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
  cmbNucPinLibrary();
  ~cmbNucPinLibrary();

  cmbNucPinLibraryConnection * GetConnection() {return this->Connection; }

  //takes ownership  return the old label if changed
  std::string addPin(PinCell* pc);
  void removePincell(PinCell* pc);
  PinCell* GetPinCell(const std::string &label);
  PinCell* GetPinCell(int pc) const;
  std::size_t GetNumberOfPinCells() const;
  void fillList(QStringList & l);
  QString extractLabel(QString const& el) const;
protected:
  std::vector<PinCell*> PinCells;
  cmbNucPinLibraryConnection * Connection;
};

#endif
