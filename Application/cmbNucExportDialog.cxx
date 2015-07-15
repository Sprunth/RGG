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
           this, SLOT(exportDone()));
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

  connect( this, SIGNAL(process( Message const& )),
          this->Exporter, SLOT(run( Message const& )));
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

void cmbNucExportDialog::exportFile(cmbNucCore * core, cmbNucInpExporter & inpExporter)
{
  this->hide();
  if(!MainWindow->onUpdateINPFiles()) return;
  this->Progress->ui->OutputArea->clear();
  if (core == NULL)
  {
    emit error("core was null");
    return;
  }
  Core = core;
  InpExporter = &inpExporter;
  this->GetRunnableCoreFile(this->ui->forceCore->isChecked());
  this->GetRunnableAssyFiles(this->ui->forceAssy->isChecked());
  this->show();
}

void cmbNucExportDialog::sendSignalToProcess()
{
  if(Core == NULL) return;
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
  if(TasksToSend.empty())
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

  Message message;
  message.assemblyTasks = TasksToSend;
  message.coregenFile   = this->CoregenFile;
  message.keepGoingAfterError = this->ui->keepGoingOnError->isChecked();
  
  send_core_mesh = true;

  this->Progress->show();
  message.CoreGenOutputFile
            = QFileInfo(CoregenFile).absolutePath() + "/"
              + QString(this->Core->getMeshOutputFilename().c_str()).trimmed();

  OuterCylinder->exportFiles(this->Core, *InpExporter);
  if(OuterCylinder->generateCylinder())
  {
    message.cylinderTask =
      Message::CylinderTask(OuterCylinder->getAssygenFileName(),
                            OuterCylinder->getCoreGenFileName(),
                            OuterCylinder->getSATFileName(),
                            OuterCylinder->getCubitFileName(),
                            this->Core->getParams().BackgroundFullPath.c_str());
  }
  emit process(message);
}

void cmbNucExportDialog::runAssygen()
{
  QString assygenExe, assyGenLibs, coregenExe, coreGenLibs, cubitExe;

  int thread_count;
  if(!cmbNucPreferencesDialog::getExecutable(assygenExe, assyGenLibs, cubitExe,
                                             coregenExe, coreGenLibs, thread_count) )
  {
    qDebug() << "One of the export exe is missing";
    emit error("One of the export exe is missing");
    return;
  }
  this->hide();
  Exporter->setAssygen(assygenExe,assyGenLibs);
  Exporter->setCubit(cubitExe);
  Exporter->setNumberOfProcessors(thread_count);

  if(TasksToSend.empty())
  {
    return;
  }
  Message message;
  message.assemblyTasks = TasksToSend;
  message.keepGoingAfterError = this->ui->keepGoingOnError->isChecked();

  this->Progress->show();
  send_core_mesh = false;
  emit process(message);
}

void cmbNucExportDialog::runSelectedAssygen()
{
  QList<QListWidgetItem *> selectedItems = this->ui->assygenFileList->selectedItems();
  this->TasksToSend.clear();
  for (int i = 0; i < selectedItems.size(); ++i)
  {
    int r = this->ui->assygenFileList->row( selectedItems[i] );
    this->TasksToSend.push_back(AllUsableAssyTasks[r]);
  }
  send_core_mesh = false;
  this->runAssygen();
}

void cmbNucExportDialog::runCoregen()
{
  if(Core == NULL) return;
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

  Message message;
  message.coregenFile = CoregenFile;
  
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
  message.CoreGenOutputFile = QFileInfo(CoregenFile).absolutePath() + "/" +
                              QString(this->Core->getMeshOutputFilename().c_str()).trimmed();

  message.keepGoingAfterError = this->ui->keepGoingOnError->isChecked();
  message.cylinderTask.valid = false;

  if(!OuterCylinder->exportFiles(this->Core, *InpExporter))
  {
    this->Progress->hide();
    return;
  }
  if(OuterCylinder->generateCylinder())
  {
    message.cylinderTask = Message::CylinderTask(OuterCylinder->getAssygenFileName(),
                                                 OuterCylinder->getCoreGenFileName(),
                                                 OuterCylinder->getSATFileName(),
                                                 OuterCylinder->getCubitFileName(),
                                                 this->Core->getParams().BackgroundFullPath.c_str());
  }

  emit process( message );
}

void cmbNucExportDialog::GetRunnableAssyFiles(bool force)
{
  this->AllUsableAssyTasks.clear();
  MainWindow->checkForNewCUBH5MFiles();
  QStringList AssygenFileList;

  std::map< std::string, std::set< Lattice::CellDrawMode > > cells =
      this->Core->getDrawModesForAssemblies();
  std::string assyFname;

  typedef std::map< std::string, std::set< Lattice::CellDrawMode > >
      stringModeMap;
  typedef std::set< Lattice::CellDrawMode > modeSet;

  for(stringModeMap::const_iterator cell_iter = cells.begin();
      cell_iter != cells.end(); ++cell_iter)
  {
    cmbNucAssembly* assembly = this->Core->GetAssembly(cell_iter->first);
    if(assembly == NULL) continue;
    std::set< Lattice::CellDrawMode > const& forms = cell_iter->second;

    for(modeSet::const_iterator form_iter = forms.begin();
        form_iter != forms.end(); ++form_iter)
    {
      if(assembly->needToRunMode(*form_iter, assyFname, forms.size()) || force)
      {
        QString assyname = assyFname.c_str();
        AssygenFileList.append(assyname);
        QString extension(assembly->getOutputExtension().c_str());
        Message::AssygenTask task( assyname,  extension, false );
        this->AllUsableAssyTasks.push_back(task);
      }
    }
  }

  this->TasksToSend = AllUsableAssyTasks;
  this->ui->assygenFileList->clear();
  this->ui->assygenFileList->addItems(AssygenFileList);
}

void cmbNucExportDialog::GetRunnableCoreFile(bool force)
{
  this->CoregenFile = QString();
  MainWindow->checkForNewCUBH5MFiles();
  if(force || Core->changeSinceLastGenerate())
  {
    this->CoregenFile = Core->getExportFileName().c_str();
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

void cmbNucExportDialog::exportDone()
{
  this->Progress->ui->command->setText("");
  this->Progress->hide();
  QFileInfo fi(CoregenFile);
  QString path = fi.absolutePath();
  QString outputMesh = path + "/" + QString(Core->getMeshOutputFilename().c_str()).trimmed();
  if(send_core_mesh)
    emit(finished(outputMesh));
}
