#include "cmbNucExportDialog.h"
#include "cmbNucAssembly.h"
#include "ui_qExporterDialog.h"
#include "ui_qProgress.h"

#include <QFileDialog>
#include <QDebug>
#include <QSettings>
#include <QFileInfo>
#include <QMessageBox>

#include "cmbNucMainWindow.h"

cmbNucExportDialog::cmbNucExportDialog(cmbNucMainWindow *mainWindow)
: QDialog(mainWindow)
{
  this->ui = new Ui_qExporterDialog;
  this->ui->setupUi(this);
  this->Progress = new cmbProgressDialog(this);

  this->MainWindow = mainWindow;

  Exporter = new cmbNucExport();
  Exporter->moveToThread(&Thread);

  connect( this->Exporter, SIGNAL(progress(int)),
           this->Progress->ui->status, SLOT(setValue(int)));
  connect( this->Exporter, SIGNAL(statusMessage(QString)),
           this->Progress->ui->OutputArea, SLOT(appendPlainText(const QString&)));
  connect( this->Exporter, SIGNAL(successful()),
           this, SLOT(done()));
  connect( this->Exporter, SIGNAL(currentProcess(QString)),
           this->Progress->ui->command, SLOT(setText(const QString &)));
  connect( this->Progress->ui->cancel, SIGNAL(clicked()),
           this, SLOT(cancel()));
  connect( this->ui->buttonBox, SIGNAL(accepted()),
           this, SLOT(sendSignalToProcess() ));
  QAbstractButton * button = this->ui->buttonBox->button( QDialogButtonBox::Ok );
  button->setText("Process All");
  connect( this->ui->runAssy, SIGNAL(clicked()), this, SLOT(runAssygen()));
  connect( this->ui->runSelectedAssy, SIGNAL(clicked()), this, SLOT(runSelectedAssygen()));
  connect( this->ui->runCore, SIGNAL(clicked()), this, SLOT(runCoregen()));

  connect( this->ui->forceAssy, SIGNAL(clicked(bool)),
           this, SLOT(GetRunnableAssyFiles(bool)));
  connect( this->ui->forceCore, SIGNAL(clicked(bool)),
          this, SLOT(GetRunnableCoreFile(bool)));
  connect( this, SIGNAL(process( const QString, const QStringList &,
                                 const QString, const QString,
                                 const QString, const QString)),
           this->Exporter, SLOT(run( const QString, const QStringList &,
                                     const QString, const QString,
                                     const QString, const QString)));
  connect( this, SIGNAL(process( const QString, const QStringList &,
                                 const QString)),
           this->Exporter, SLOT(runAssy( const QString, const QStringList &,
                                         const QString)));
  connect( this, SIGNAL(process( const QString, const QString,
                                const QString)),
           this->Exporter, SLOT(runCore( const QString, const QString,
                                         const QString)));
  connect( this->Exporter, SIGNAL(fileDone()), this, SIGNAL(fileFinish()));
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
  this->Progress->ui->OutputArea->clear();
  if (core == NULL)
  {
    emit error("core was null");
    return;
  }
  Core = core;
  this->GetRunnableCoreFile(this->ui->forceCore->isChecked());
  this->GetRunnableAssyFiles(this->ui->forceAssy->isChecked());
  this->show();
}

void cmbNucExportDialog::sendSignalToProcess()
{
  qDebug() << "SENDING TO THREAD";
  QSettings settings("CMBNuclear", "CMBNuclear");
  QString assygenExe = settings.value("EXPORTER/assygen_exe").toString();
  QString coregenExe = settings.value("EXPORTER/coregen_exe").toString();
  QString cubitExe = settings.value("EXPORTER/cubit_exe").toString();
  if(assygenExe.isEmpty())
  {
    qDebug() << "Failed assygen is empty";
    emit error("Failed assygen is empty");
    return;
  }
  if(cubitExe.isEmpty())
  {
    qDebug() << "Failed cubit is empty";
    emit error("Failed cubit is empty");
    return;
  }
  if(coregenExe.isEmpty())
  {
    qDebug() << "Failed coregen is empty";
    emit error("Failed coregen is empty");
    return;
  }
  if(this->AssygenFileList.empty())
  {
    this->runCoregen();
    return;
  }
  if(CoregenFile.isEmpty())
  {
    this->runAssygen();
    return;
  }
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

void cmbNucExportDialog::runAssygen()
{
  QSettings settings("CMBNuclear", "CMBNuclear");
  QString assygenExe = settings.value("EXPORTER/assygen_exe").toString();
  this->hide();

  if(assygenExe.isEmpty())
  {
    qDebug() << "Failed assygen is empty";
    emit error("Failed assygen is empty");
    return;
  }

  QString cubitExe = settings.value("EXPORTER/cubit_exe").toString();
  if(cubitExe.isEmpty())
  {
    qDebug() << "Failed cubit is empty";
    emit error("Failed cubit is empty");
    return;
  }

  if(this->AssygenFileList.empty())
  {
    return;
  }

  this->Progress->show();
  emit process(assygenExe, this->AssygenFileList, cubitExe);
}

void cmbNucExportDialog::runSelectedAssygen()
{
  QList<QListWidgetItem *> selectedItems = this->ui->assygenFileList->selectedItems();
  this->AssygenFileList.clear();
  for (int i = 0; i < selectedItems.size(); ++i)
  {
    this->AssygenFileList.append(selectedItems[i]->text());
  }

  this->runAssygen();
}

void cmbNucExportDialog::runCoregen()
{
  qDebug() << "SENDING TO THREAD";
  QSettings settings("CMBNuclear", "CMBNuclear");
  QString coregenExe = settings.value("EXPORTER/coregen_exe").toString();
  this->hide();
  if(CoregenFile.isEmpty())
  {
    this->runAssygen();
    return;
  }
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

  emit process( coregenExe, CoregenFile, outputMesh);
}

void cmbNucExportDialog::GetRunnableAssyFiles(bool force)
{
  this->AssygenFileList.clear();
  MainWindow->checkForNewCUBH5MFiles();
  for (size_t i = 0; i < Core->GetNumberOfAssemblies(); ++i)
  {
    cmbNucAssembly * assy = this->Core->GetAssembly(i);
    if(force || assy->changeSinceLastGenerate())
    {
      if(assy->changeSinceLastSave())
      {
        QMessageBox msgBox;
        msgBox.setText(("Assembly " + assy->label + " has change since last.").c_str());
        msgBox.setInformativeText("Do you want to save your changes?");
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard );
        msgBox.setDefaultButton(QMessageBox::Save);
        int ret = msgBox.exec();
        switch (ret) {
          case QMessageBox::Save:
            MainWindow->saveFile(assy);
            break;
          default:
            // should never be reached
            break;
        }
      }
      this->AssygenFileList.append(QString(assy->FileName.c_str()));
    }
  }
  this->ui->assygenFileList->clear();
  this->ui->assygenFileList->addItems(AssygenFileList);
}

void cmbNucExportDialog::GetRunnableCoreFile(bool force)
{
  CoregenFile = QString();
  MainWindow->checkForNewCUBH5MFiles();
  if(force || Core->changeSinceLastGenerate())
  {
    if(Core->changeSinceLastSave())
    {
      QMessageBox msgBox;
      msgBox.setText("Core has change since last.");
      msgBox.setInformativeText("Do you want to save your changes?");
      msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard );
      msgBox.setDefaultButton(QMessageBox::Save);
      int ret = msgBox.exec();
      switch (ret) {
        case QMessageBox::Save:
          MainWindow->saveFile(Core);
          break;
        default:
          // should never be reached
          break;
      }
    }
    CoregenFile = Core->FileName.c_str();
  }
  this->ui->coregenInputFile->setText(CoregenFile);
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
