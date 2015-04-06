#ifndef cmbNucExportDialog_H
#define cmbNucExportDialog_H

#include <QDialog>
#include <QStringList>
#include <QThread>
#include "cmbNucPartDefinition.h"
#include "cmbNucExport.h"
#include "cmbNucGenerateOuterCylinder.h"
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
  : QDialog(d, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint)
  {
    ui = new Ui_qProgress;
    ui->setupUi(this);
    ui->OutputArea->setVisible(false);
    connect( ui->OutputPromp, SIGNAL(clicked(bool)), ui->OutputArea, SLOT(setVisible(bool)));
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

  void waitTillDone()
  {
    Exporter->waitTillDone();
  }

public slots:
  void exportFile(cmbNucCore * core, cmbNucInpExporter & inpExporter);

protected slots:
  void sendSignalToProcess();
  void runAssygen();
  void runSelectedAssygen();
  void runCoregen();
  void cancel();
  void exportDone();
  void GetRunnableAssyFiles(bool);
  void GetRunnableCoreFile(bool);

signals:
  void process( Message const& msg );
public:
signals:
  void error(QString);
  void finished(QString);
  void fileFinish();

private:
  // Designer form
  Ui_qExporterDialog *ui;
  cmbNucCore * Core;
  cmbNucInpExporter * InpExporter;
  cmbNucMainWindow *MainWindow;

  bool send_core_mesh;

  std::vector<Message::AssygenTask> AllUsableAssyTasks;
  std::vector<Message::AssygenTask> TasksToSend;

  cmbProgressDialog *Progress;
  QString CoregenFile;
  cmbNucExport * Exporter;
  cmbNucGenerateOuterCylinder * OuterCylinder;
  QThread Thread;
};

#endif
