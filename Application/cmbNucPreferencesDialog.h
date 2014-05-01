#ifndef cmbNucPrefereneceDialog_H
#define cmbNucPrefereneceDialog_H

#include <QDialog>
#include <QStringList>
#include <QThread>
#include "cmbNucPartDefinition.h"
#include "ui_Preferences.h"

// Forward Qt class declarations
class cmbNucMainWindow;

class cmbNucPreferencesDialog : public QDialog
{
  Q_OBJECT
public:
  cmbNucPreferencesDialog(cmbNucMainWindow* mainWindow);
  ~cmbNucPreferencesDialog();

public slots:
  void setPreferences();

signals:
  void actionParallelProjection(bool);

protected slots:
  void browserAssygenExecutable();
  void browserCubitExecutable();
  void browserCoregenExecutable();
  void setValues();

private:
  // Designer form
  Ui_Preferences *ui;
};

#endif
