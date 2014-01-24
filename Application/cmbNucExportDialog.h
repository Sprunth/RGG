#ifndef cmbNucExportDialog_H
#define cmbNucExportDialog_H

#include <QDialog>
#include <QStringList>
#include <QThread>
#include "cmbNucPartDefinition.h"
#include "cmbNucExport.h"
#include "cmbNucCore.h"
#include "ui_qProgress.h"

// Forward Qt class declarations
class Ui_qExporterDialog;
class Ui_qProgress;
class cmbNucMainWindow;

class cmbProgressDialog: public QDialog
{
  Q_OBJECT
public:
  Ui_qProgress *ui;
  cmbProgressDialog(QDialog * d)
  : QDialog(d)
  {
    ui = new Ui_qProgress;
    ui->setupUi(this);
  }
  ~cmbProgressDialog()
  { delete ui; }
};

class cmbNucExportDialog : public QDialog
{
  Q_OBJECT
public:
  cmbNucExportDialog(cmbNucMainWindow* mainWindow);
  ~cmbNucExportDialog();

public slots:
  void exportFile(cmbNucCore * core);

protected slots:
  void sendSignalToProcess();

signals:
  void process( const QString, const QStringList &,
                const QString, const QString,
                const QString);
  void error(QString);

private:
  // Designer form
  Ui_qExporterDialog *ui;
  cmbNucCore * Core;

  cmbProgressDialog *Progress;
  QStringList AssygenFileList;
  QString CoregenFile;
  cmbNucExport * Exporter;
  QThread Thread;
};

#endif
