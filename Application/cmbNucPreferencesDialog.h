#ifndef cmbNucPrefereneceDialog_H
#define cmbNucPrefereneceDialog_H

#include <QDialog>
#include <QStringList>
#include <QThread>
#include <QString>
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

  static bool isOk();
  static bool hasPackaged();
  static bool usePackaged();
  static bool getExecutable(QString & assygenExe, QString & assygenLib,
                            QString & cubitExe,
                            QString & coregenExe, QString & coregenLib);
  static bool getPackaged(QString & assygenExe, QString & coregenExe);

public slots:
  void setPreferences(bool e = false);

signals:
  void actionParallelProjection(bool);
  void valuesSet();

protected slots:
  void browserAssygenExecutable();
  void browserCubitExecutable();
  void browserCoregenExecutable();
  void setValues();
  void checkValues();

private:
  // Designer form
  Ui_Preferences *ui;
  bool EmitValuesSet;
};

#endif
