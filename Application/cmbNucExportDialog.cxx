#include "cmbNucExportDialog.h"
#include "cmbNucAssembly.h"
#include "ui_qExporterDialog.h"
#include "ui_qProgress.h"

#include <QFileDialog>
#include <QDebug>
#include <QSettings>
#include <QFileInfo>

#include "cmbNucMainWindow.h"

cmbNucExportDialog::cmbNucExportDialog(cmbNucMainWindow *mainWindow)
: QDialog(mainWindow)
{
  this->ui = new Ui_qExporterDialog;
  this->ui->setupUi(this);
  this->Progress = new cmbProgressDialog(this);

  Exporter = new cmbNucExport();
  Exporter->moveToThread(&Thread);

  connect( this->Exporter, SIGNAL(progress(int)),
           this->Progress->ui->status, SLOT(setValue(int)));
  connect( this->Exporter, SIGNAL(successful()),
           this, SLOT(done()));
  connect( this->Exporter, SIGNAL(currentProcess(QString)),
           this->Progress->ui->command, SLOT(setText(const QString &)));
  connect( this->Progress->ui->cancel, SIGNAL(clicked()),
           this, SLOT(cancel()));
  connect( this->ui->buttonBox, SIGNAL(accepted()),
           this, SLOT(sendSignalToProcess() ));
  connect( this, SIGNAL(process( const QString, const QStringList &,
                                 const QString, const QString,
                                 const QString, const QString)),
           this->Exporter, SLOT(run( const QString, const QStringList &,
                                     const QString, const QString,
                                     const QString, const QString)));
  Thread.start();
}

cmbNucExportDialog::~cmbNucExportDialog()
{
  delete this->ui;
  delete this->Progress;
  Thread.quit();
  Thread.wait();
}

void cmbNucExportDialog::exportFile(cmbNucCore * core)
{
  this->hide();
  if (core == NULL)
  {
    emit error("core was null");
    return;
  }
  Core = core;
  QSettings settings("CMBNuclear", "CMBNuclear");
  QString assygenexe = settings.value("EXPORTER/assygen_exe").toString();
  this->ui->assygenExecutable->setText(assygenexe);
  QString coregenexe = settings.value("EXPORTER/coregen_exe").toString();
  this->ui->coregenExecutable->setText(coregenexe);
  QString cubitexe = settings.value("EXPORTER/cubit_exe").toString();
  this->ui->cubitExecutable->setText(cubitexe);
  CoregenFile = core->FileName.c_str();
  this->ui->coregenInputFile->setText(CoregenFile);
  this->AssygenFileList.clear();
  for (size_t i = 0; i < core->GetNumberOfAssemblies(); ++i)
    {
    this->AssygenFileList.append(QString(core->GetAssembly(i)->FileName.c_str()));
    }
  this->ui->assygenFileList->clear();
  this->ui->assygenFileList->addItems(AssygenFileList);
  this->show();
}

void cmbNucExportDialog::sendSignalToProcess()
{
  qDebug() << "SENDING TO THREAD";
  //void run( const QString &assygenExe,
  //          const QStringList & assygenFile,
  //          const QString &cubitExe,
  //          const QString &coregenExe,
  //          const QString &coregenFile);
  QString assygenExe = ui->assygenExecutable->text();
  if(assygenExe.isEmpty())
  {
    qDebug() << "Failed assygen is empty";
    emit error("Failed assygen is empty");
    return;
  }
  QString cubitExe = ui->cubitExecutable->text();
  if(cubitExe.isEmpty())
  {
    qDebug() << "Failed cubit is empty";
    emit error("Failed cubit is empty");
    return;
  }
  QString coregenExe = ui->coregenExecutable->text();
  if(coregenExe.isEmpty())
  {
    qDebug() << "Failed coregen is empty";
    emit error("Failed coregen is empty");
    return;
  }
  if(this->AssygenFileList.empty())
  {
    qDebug() << "Failed assygen File list is empty";
    emit error("Failed assygen File list is empty");
    return;
  }
  if(CoregenFile.isEmpty())
  {
    qDebug() << "Failed coregen file is empty";
    emit error("Failed coregen file is empty");
    return;
  }
  //SAVE FILES TO OUTPUT DIRECTORY
  //TODO!!!!!!!!!!
  this->Progress->show();
  QString outputMesh;
  if(Core == NULL || Core->h5mFile.empty())
  {
    QFileInfo fi(CoregenFile);
    QString path = fi.absolutePath();
    QString name = fi.completeBaseName();
    outputMesh = path + '/' + name + ".h5m";
    Core->h5mFile = name.toStdString();
  }
  else
  {
    QFileInfo fi(CoregenFile);
    QString path = fi.absolutePath();
    qDebug() << Core->h5mFile.c_str();
    outputMesh = path + "/" + QString(Core->h5mFile.c_str()).trimmed();
    qDebug() << outputMesh;
  }

  emit process(assygenExe, this->AssygenFileList,
               cubitExe, coregenExe, CoregenFile, outputMesh);
}

void cmbNucExportDialog::cancel()
{
  this->Progress->ui->command->setText("");
  qDebug() << "CANCELING";
  Exporter->cancel();
  qDebug() << "Canceling done";
  this->Progress->hide();
}

void cmbNucExportDialog::done()
{
  this->Progress->ui->command->setText("");
  this->Progress->hide();
  QFileInfo fi(CoregenFile);
  QString path = fi.absolutePath();
  qDebug() << Core->h5mFile.c_str();
  QString outputMesh = path + "/" + QString(Core->h5mFile.c_str()).trimmed();
  emit(finished(outputMesh));
}
