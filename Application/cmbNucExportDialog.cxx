#include "cmbNucExportDialog.h"
#include "cmbNucAssembly.h"
#include "ui_qExporterDialog.h"
#include "ui_qProgress.h"
#include "cmbNucPreferencesDialog.h"

#include <QFileDialog>
#include <QDebug>
#include <QFileInfo>
#include <QMessageBox>

#include "cmbNucMainWindow.h"

cmbNucExportDialog::cmbNucExportDialog(cmbNucMainWindow *mainWindow)
: QDialog(mainWindow)
{
  this->ui = new Ui_qExporterDialog;
  this->ui->setupUi(this);
  this->Progress = new cmbProgressDialog(this);

  this->OuterCylinder = new cmbNucGenerateOuterCylinder();

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

  connect( this, SIGNAL(process( const QStringList &, const QString, const QString,
                                 const QString, const QString, const QString, const QString, const QString, bool)),
          this->Exporter, SLOT(run( const QStringList &, const QString, const QString,
                                    const QString, const QString, const QString, const QString, const QString, bool)));
  connect( this->Exporter, SIGNAL(fileDone()), this, SIGNAL(fileFinish()));
  Thread.start();
}

cmbNucExportDialog::~cmbNucExportDialog()
{
  delete this->ui;
  delete this->Progress;
  delete this->OuterCylinder;
  std::cout << "quit dialog thread" << std::endl;
  Thread.quit();
  std::cout << "wait for dialog thread" << std::endl;
  Thread.wait();
  std::cout << "Done wait for dialog thread" << std::endl;
  delete Exporter;
  std::cout << "Done with deleting dialog" << std::endl;
}

void cmbNucExportDialog::exportFile(cmbNucCore * core)
{
  this->hide();
  MainWindow->onUpdateINPFiles();
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
  int numberOfThreads;
  QString assygenExe, assyGenLibs, coregenExe, coreGenLibs, cubitExe;
  if(!cmbNucPreferencesDialog::getExecutable(assygenExe, assyGenLibs, cubitExe,
                                             coregenExe, coreGenLibs, numberOfThreads))
  {
    qDebug() << "One of the export exe is missing";
    emit error("One of the export exe is missing");
    return;
  }


  Exporter->setAssygen(assygenExe,assyGenLibs);
  Exporter->setCubit(cubitExe);
  Exporter->setNumberOfProcessors(numberOfThreads);
  coreGenLibs.append((":" + QFileInfo(cubitExe).absolutePath().toStdString()).c_str());
  Exporter->setCoregen(coregenExe, coreGenLibs);

  send_core_mesh = true;
  if(this->AssygenFileList.empty())
  {
    this->runCoregen();
    return;
  }

  if(CoregenFile.isEmpty())
  {
    this->runAssygen();
    send_core_mesh = true;
    return;
  }
  send_core_mesh = true;

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

  OuterCylinder->exportFiles(this->Core);
  if(OuterCylinder->generateCylinder())
  {
    emit process(this->AssygenFileList, CoregenFile, outputMesh,
                 OuterCylinder->getAssygenFileName(), OuterCylinder->getCubitFileName(),
                 this->Core->Params.BackgroundFullPath.c_str(),
                 OuterCylinder->getCoreGenFileName(), OuterCylinder->getSATFileName(),
                 this->ui->keepGoingOnError->isChecked());
                 
  }
  else
  {
    emit process(this->AssygenFileList, CoregenFile, outputMesh, QString(), QString(), QString(), QString(), QString(),
                 this->ui->keepGoingOnError->isChecked());
  }
}

void cmbNucExportDialog::runAssygen()
{
  QString assygenExe, assyGenLibs, coregenExe, coreGenLibs, cubitExe;
  int thread_count;
  if(!cmbNucPreferencesDialog::getExecutable(assygenExe, assyGenLibs, cubitExe,
                                             coregenExe, coreGenLibs, thread_count))
  {
    qDebug() << "One of the export exe is missing";
    emit error("One of the export exe is missing");
    return;
  }
  this->hide();
  Exporter->setAssygen(assygenExe,assyGenLibs);
  Exporter->setCubit(cubitExe);
  Exporter->setNumberOfProcessors(thread_count);

  if(this->AssygenFileList.empty())
  {
    return;
  }

  this->Progress->show();
  send_core_mesh = false;
  emit process(this->AssygenFileList, QString(), QString(), QString(), QString(), QString(), QString(), QString(),
               this->ui->keepGoingOnError->isChecked());
}

void cmbNucExportDialog::runSelectedAssygen()
{
  QList<QListWidgetItem *> selectedItems = this->ui->assygenFileList->selectedItems();
  this->AssygenFileList.clear();
  for (int i = 0; i < selectedItems.size(); ++i)
  {
    this->AssygenFileList.append(selectedItems[i]->text());
  }
  send_core_mesh = false;
  this->runAssygen();
}

void cmbNucExportDialog::runCoregen()
{
  qDebug() << "SENDING TO THREAD";
  send_core_mesh = true;
  QString assygenExe, assyGenLibs, coregenExe, coreGenLibs, cubitExe;
  int thread_count;
  if(!cmbNucPreferencesDialog::getExecutable(assygenExe, assyGenLibs, cubitExe,
                                             coregenExe, coreGenLibs, thread_count))
  {
    qDebug() << "One of the export exe is missing";
    emit error("One of the export exe is missing");
    return;
  }
  coreGenLibs.append((":" + QFileInfo(cubitExe).absolutePath().toStdString()).c_str());
  Exporter->setCoregen(coregenExe,coreGenLibs);
  Exporter->setNumberOfProcessors(thread_count);
  this->hide();
  if(CoregenFile.isEmpty())
  {
    this->runAssygen();
    send_core_mesh = true;
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

  OuterCylinder->exportFiles(this->Core);
  if(OuterCylinder->generateCylinder())
  {
    emit process(QStringList(), CoregenFile, outputMesh,
                 OuterCylinder->getAssygenFileName(), OuterCylinder->getCubitFileName(),
                 this->Core->Params.BackgroundFullPath.c_str(),
                 OuterCylinder->getCoreGenFileName(), OuterCylinder->getSATFileName(),
                 this->ui->keepGoingOnError->isChecked());

  }
  else
  {
    emit process( QStringList(), CoregenFile, outputMesh, QString(), QString(), QString(), QString(), QString(),
                  this->ui->keepGoingOnError->isChecked());
  }
}

void cmbNucExportDialog::GetRunnableAssyFiles(bool force)
{
  this->AssygenFileList.clear();
  MainWindow->checkForNewCUBH5MFiles();
  for (int i = 0; i < Core->GetNumberOfAssemblies(); ++i)
  {
    cmbNucAssembly * assy = this->Core->GetAssembly(i);
    if(force || assy->changeSinceLastGenerate())
    {
      for(std::map< Lattice::CellDrawMode, std::string >::const_iterator iter = assy->ExportFileNames.begin(); iter != assy->ExportFileNames.end(); ++iter)
      {
        this->AssygenFileList.append(QString(iter->second.c_str()));
      }
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
    CoregenFile = Core->ExportFileName.c_str();
  }
  this->ui->coregenInputFile->setText(CoregenFile);
}

void cmbNucExportDialog::cancel()
{
  if(OuterCylinder->generateCylinder())
  {
    OuterCylinder->deleteTempFiles();
  }
  this->Progress->ui->command->setText("");
  qDebug() << "CANCELING";
  Exporter->cancel();
  qDebug() << "Canceling done";
  this->Progress->hide();
}

void cmbNucExportDialog::done()
{
  if(OuterCylinder->generateCylinder())
  {
    OuterCylinder->deleteTempFiles();
  }
  this->Progress->ui->command->setText("");
  this->Progress->hide();
  QFileInfo fi(CoregenFile);
  QString path = fi.absolutePath();
  qDebug() << Core->h5mFile.c_str();
  QString outputMesh = path + "/" + QString(Core->h5mFile.c_str()).trimmed();
  if(send_core_mesh)
    emit(finished(outputMesh));
}
