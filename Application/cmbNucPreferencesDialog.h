#ifndef cmbNucPrefereneceDialog_H
#define cmbNucPrefereneceDialog_H

#include <QDialog>
#include <QStringList>
#include <QThread>
#include <QString>
#include "cmbNucPartDefinition.h"
#include "ui_Preferences.h"

// Forward Qt class declarations
class QMainWindow;
class QLineEdit;

class cmbNucPreferencesDialog : public QDialog
{
  Q_OBJECT
public:
  cmbNucPreferencesDialog(QMainWindow* mainWindow);
  ~cmbNucPreferencesDialog();

  static bool isOk();
  static bool hasPackaged();
  static bool usePackaged();
  static bool getExecutable(QString & assygenExe, QString & assygenLib,
                            QString & cubitExe,
                            QString & coregenExe, QString & coregenLib,
                            int & numberOfProcessors);
  static bool getPostBLInpFileGenerator(QString & exe);
  static bool getPostBL(QString & exe, QString & lib);
  static bool getPackaged(QString & assygenExe, QString & coregenExe);
  static bool getPackaged(QString & postBL);

public slots:
  void setPreferences(bool e = false);

signals:
  void actionParallelProjection(bool);
  void valuesSet();

protected slots:
  void browserAssygenExecutable();
  void browserCubitExecutable();
  void browserCoregenExecutable();
  void browserPostBLExectuable();
  void setValues();
  void checkValues();

private:
  // Designer form
  void browserExectuable( QLineEdit* );
  Ui_Preferences *ui;
  bool EmitValuesSet;
};

#endif
