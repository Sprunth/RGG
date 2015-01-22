#ifndef cmbNucConflictDialog_H
#define cmbNucConflictDialog_H

#include <QDialog>
#include <QStringList>
#include <QThread>
#include "cmbNucPinLibrary.h"
#include "cmbNucPinCell.h"
#include "Ui_name_conflict_gui.h"

// Forward Qt class declarations
class Ui_name_conflict_gui;
class cmbNucMainWindow;
class QMainWindow;

class cmbNucConflictDialog : public QDialog
{
  Q_OBJECT
public:
  enum modeType {REPLACE, RENAME, IGNORE};
  cmbNucConflictDialog(QMainWindow* mainWindow, cmbNucPinLibrary * lib, PinCell * p);
  ~cmbNucConflictDialog();
  bool rename()
  {
    return mode == RENAME;
  }

  bool keepGoing()
  {
    return mKeepGoing;
  }

protected slots:
  void labelChanged(QString);
  void nameChanged(QString);
  void selectedReplace(bool);
  void selectedRename(bool);
  void selectedIgnore(bool);
  void apply();

private:
  // Designer form
  modeType mode;
  Ui_name_conflict_gui *ui;
  cmbNucPinLibrary * pinLibrary;
  PinCell * pin;

  bool mKeepGoing;

  bool checkLabels();

};

#endif
