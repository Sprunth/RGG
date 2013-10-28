#include "cmbNucMainWindow.h"

#include "ui_qNucMainWindow.h"
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCompositePolyDataMapper2.h>
#include <QFileDialog>
#include <QFileInfo>
#include <QStringList>
#include <QDebug>
#include <QDockWidget>
#include <QProcess>
#include <QTemporaryFile>
#include <QTimer>

#include "cmbNucAssembly.h"
#include "cmbNucCore.h"
#include "cmbNucInputPropertiesWidget.h"
#include "cmbNucInputListWidget.h"
#include "cmbNucMaterialColors.h"

#include "vtkAxesActor.h"
#include "vtkProperty.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkInformation.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkAlgorithm.h"
#include "vtkNew.h"
#include "vtkCmbLayeredConeSource.h"

// Constructor
cmbNucMainWindow::cmbNucMainWindow()
{
//  vtkNew<vtkCompositeDataPipeline> compositeExec;
//  vtkAlgorithm::SetDefaultExecutivePrototype(compositeExec.GetPointer());

  this->ui = new Ui_qNucMainWindow;
  this->ui->setupUi(this);
  this->NuclearCore = new cmbNucCore();

  // VTK/Qt wedded
  this->Renderer = vtkSmartPointer<vtkRenderer>::New();

  // setup depth peeling
  vtkRenderWindow *renderWindow = this->ui->qvtkWidget->GetRenderWindow();
  renderWindow->SetAlphaBitPlanes(1);
  renderWindow->SetMultiSamples(0);
  this->Renderer->SetUseDepthPeeling(1);
  this->Renderer->SetMaximumNumberOfPeels(100);

  renderWindow->AddRenderer(this->Renderer);
  this->Mapper = vtkSmartPointer<vtkCompositePolyDataMapper2>::New();
  this->Mapper->SetScalarVisibility(0);
  this->Actor = vtkSmartPointer<vtkActor>::New();
  this->Actor->SetMapper(this->Mapper.GetPointer());
  this->Actor->GetProperty()->SetShading(1);
  this->Actor->GetProperty()->SetInterpolationToPhong();
// this->Actor->GetProperty()->EdgeVisibilityOn();
  this->Renderer->AddActor(this->Actor);

  vtkCmbLayeredConeSource *cone = vtkCmbLayeredConeSource::New();
  cone->SetNumberOfLayers(3);
  cone->SetHeight(20.0);
  cone->Update();
  this->Mapper->SetInputDataObject(cone->GetOutput());
  cone->Delete();

  vtkCompositeDataDisplayAttributes *attributes = vtkCompositeDataDisplayAttributes::New();
  this->Mapper->SetCompositeDataDisplayAttributes(attributes);
  attributes->Delete();

  // add axes actor
  vtkAxesActor *axesActor = vtkAxesActor::New();
 // this->Renderer->AddActor(axesActor);
  axesActor->Delete();

  // Set up action signals and slots
  connect(this->ui->actionExit, SIGNAL(triggered()), this, SLOT(onExit()));
  connect(this->ui->actionOpenFile, SIGNAL(triggered()), this, SLOT(onFileOpen()));
  connect(this->ui->actionSaveFile, SIGNAL(triggered()), this, SLOT(onFileSave()));
  connect(this->ui->actionNew, SIGNAL(triggered()), this, SLOT(onFileNew()));
  connect(this->ui->actionPreferences, SIGNAL(triggered()), this, SLOT(onShowPreferences()));
  connect(this->ui->actionRunAssygen, SIGNAL(triggered()), this, SLOT(onRunAssygen()));

  // Initial materials and  colors
  this->MaterialColors = new cmbNucMaterialColors();
  QString materialfile =
    QCoreApplication::applicationDirPath() + "/materialcolors.ini";
  this->MaterialColors->OpenFile(materialfile);

  this->initPanels();

  // by default 1:1 scaling for the Z-axis
  this->ZScale = 1.0;
  connect(this->ui->viewScaleSlider, SIGNAL(valueChanged(int)),
          this->ui->viewScaleSpinBox, SLOT(setValue(int)));
  connect(this->ui->viewScaleSpinBox, SIGNAL(valueChanged(int)),
          this->ui->viewScaleSlider, SLOT(setValue(int)));
  connect(this->ui->viewScaleSpinBox, SIGNAL(valueChanged(int)),
          this, SLOT(zScaleChanged(int)));

  QTimer::singleShot(0, this, SLOT(ResetView()));
}

cmbNucMainWindow::~cmbNucMainWindow()
{
  this->PropertyWidget->setObject(NULL, NULL);
  this->PropertyWidget->setAssembly(NULL);
  this->InputsWidget->setCore(NULL);
  delete this->NuclearCore;
  delete this->MaterialColors;
}

void cmbNucMainWindow::initPanels()
{
  this->InputsWidget = new cmbNucInputListWidget(this);
  this->PropertyWidget = new cmbNucInputPropertiesWidget(this);
  this->PropertyWidget->updateMaterials();
  this->ui->InputsDock->setWidget(this->InputsWidget);
  this->ui->InputsDock->setFeatures(
    QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

  this->ui->PropertyDock->setWidget(this->PropertyWidget);
  this->ui->PropertyDock->setFeatures(
    QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

  this->PropertyWidget->setEnabled(0);
  this->InputsWidget->setEnabled(0);
  this->InputsWidget->setCore(this->NuclearCore);

  QObject::connect(this->InputsWidget,
    SIGNAL(objectSelected(AssyPartObj*, const char*)), this,
    SLOT(onObjectSelected(AssyPartObj*, const char*)));
  QObject::connect(this->InputsWidget,
    SIGNAL(objectRemoved()), this,
    SLOT(onObjectModified()));

  QObject::connect(this->PropertyWidget,
    SIGNAL(currentObjectModified(AssyPartObj*)), this,
    SLOT(onObjectModified(AssyPartObj*)));
  QObject::connect(this->InputsWidget,
    SIGNAL(materialColorChanged(const QString&)), this,
    SLOT(onObjectModified()));

}

void cmbNucMainWindow::onObjectSelected(AssyPartObj* selObj,
  const char* name)
{
  if(!selObj)
    {
    this->ui->PropertyDock->setEnabled(0);
    return;
    }
  this->ui->PropertyDock->setEnabled(1);
  this->PropertyWidget->setAssembly(this->InputsWidget->getCurrentAssembly());
  this->PropertyWidget->setObject(selObj, name);
}

void cmbNucMainWindow::onObjectModified(AssyPartObj* obj)
{
  // update material colors
  this->updateMaterialColors();

  if(obj && obj->GetType() == CMBNUC_CORE)
    {
    this->Renderer->ResetCamera();
    }
  // render
  this->ui->qvtkWidget->update();
}

void cmbNucMainWindow::onExit()
{
  qApp->exit();
}

void cmbNucMainWindow::onFileNew()
{
  this->PropertyWidget->setObject(NULL, NULL);
  this->PropertyWidget->setAssembly(NULL);
  this->InputsWidget->onNewAssembly();
  this->Renderer->ResetCamera();
  this->Renderer->Render();
}

void cmbNucMainWindow::onFileOpen()
{
  QStringList fileNames =
    QFileDialog::getOpenFileNames(this,
                                 "Open Assygen File...",
                                 QDir::homePath(),
                                 "INP Files (*.inp)");
  if(fileNames.count()==0)
    {
    return;
    }
  this->openFiles(fileNames);

  // update render view
  this->Renderer->ResetCamera();
  this->Renderer->Render();
}

void cmbNucMainWindow::openFiles(const QStringList &fileNames)
{
  this->setCursor(Qt::BusyCursor);
  // clear old assembly
  this->PropertyWidget->setObject(NULL, NULL);
  this->PropertyWidget->setAssembly(NULL);
  int numExistingAssy = this->NuclearCore->GetNumberOfAssemblies();
  int numNewAssy = 0;
  QList<cmbNucAssembly*> assemblies;
  foreach(QString fileName, fileNames)
    {
    if(!fileName.isEmpty())
      {
      cmbNucAssembly* assy = this->loadAssemblyFromFile(fileName);
      assemblies.append(assy);
      numNewAssy++;
      }
    }
  // the very first time
  if(numExistingAssy == 0)
    {
    this->NuclearCore->SetDimensions(numNewAssy, numNewAssy);
    for(int i=0; i<numNewAssy ; i++)
      {
      this->NuclearCore->SetAssemblyLabel(i, 0, assemblies.at(i)->label);
      for(int j=1; j<numNewAssy ; j++)
        {
        this->NuclearCore->SetAssemblyLabel(i, j, "xx");
        }
      }
    }
  // update data colors
  this->updateMaterialColors();
  // render
  this->InputsWidget->updateUI(numExistingAssy==0 && numNewAssy>1);

  this->unsetCursor();
}

cmbNucAssembly* cmbNucMainWindow::loadAssemblyFromFile(const QString &fileName)
{
  // read file and create new assembly
  QFileInfo finfo(fileName);
  cmbNucAssembly* assembly = new cmbNucAssembly;
  assembly->label = finfo.baseName().toStdString();
//  assembly->label = QString("Assy").append(
//    QString::number(this->NuclearCore->GetNumberOfAssemblies()+1)).toStdString();
  this->NuclearCore->AddAssembly(assembly);
  assembly->ReadFile(fileName.toStdString());
  return assembly;
}

void cmbNucMainWindow::onFileSave()
{
  QString fileName =
    QFileDialog::getSaveFileName(this,
                                 "Save Assygen File...",
                                 QDir::homePath(),
                                 "INP Files (*.inp)");
  if(!fileName.isEmpty())
    {
    this->setCursor(Qt::BusyCursor);
    this->saveFile(fileName);
    this->unsetCursor();
    }
}

void cmbNucMainWindow::saveFile(const QString &fileName)
{
  if(!this->InputsWidget->getCurrentAssembly())
    {
    qDebug() << "no assembly to save";
    return;
    }

  this->InputsWidget->getCurrentAssembly()->WriteFile(fileName.toStdString());
}

void cmbNucMainWindow::updateMaterialColors()
{
  // regenerate core and assembly view
  vtkSmartPointer<vtkMultiBlockDataSet> coredata = this->NuclearCore->GetData();
  this->Mapper->SetInputDataObject(coredata);
  if(!coredata)
  {
  return;
  }
  unsigned int numCoreBlocks = coredata->GetNumberOfBlocks();
  vtkCompositeDataDisplayAttributes *attributes =
    this->Mapper->GetCompositeDataDisplayAttributes();
  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();

  //vtkDataObjectTreeIterator *coreiter = coredata->NewTreeIterator();
  //coreiter->SetSkipEmptyNodes(false);
  unsigned int realflatidx=0;
  for(unsigned int block=0; block<numCoreBlocks; block++)
    {
    realflatidx++;// for assembly block
    if(!coredata->GetBlock(block))
      {
      continue;
      }
    vtkMultiBlockDataSet* data = vtkMultiBlockDataSet::SafeDownCast(coredata->GetBlock(block));
    if(!data)
      {
      continue;
      }
    // for each assembly
    if(coredata->GetMetaData(block)->Has(vtkCompositeDataSet::NAME()))
      {
      std::string assyLabel = coredata->GetMetaData(block)->Get(
        vtkCompositeDataSet::NAME());
      cmbNucAssembly* assy = this->NuclearCore->GetAssembly(assyLabel);
      if(!assy)
        {
        qCritical() << "no specified assembly to found in core: " << assyLabel.c_str();
        return;
        }

      std::pair<int, int> dimensions = assy->AssyLattice.GetDimensions();

      // count number of pin blocks in the data set
      int pins = 0;
      for(int i = 0; i < dimensions.first; i++)
        {
        for(int j = 0; j < dimensions.second; j++)
          {
          pins += 1;
          }
        }

      int pin_count = 0;
      int ducts_count = 0;

      std::string pinMaterial = "pin";
      int numAssyBlocks = data->GetNumberOfBlocks();
      for(unsigned int idx=0; idx<numAssyBlocks; idx++)
        {
        realflatidx++;
        if(pin_count < pins)
          {
          std::string label = assy->AssyLattice.GetCell(pin_count);
          PinCell* pinCell = assy->GetPinCell(label);

          if(pinCell)
            {
//            realflatidx++;
            std::string pinMaterial;

            for(unsigned int idx=0; idx<pinCell->cylinders.size(); idx++)
              {
              realflatidx++; // increase one for this cylinder
              for(int k = 0; k < pinCell->GetNumberOfLayers(); k++)
                {
                pinMaterial = pinCell->cylinders[idx]->materials[k];
                matColorMap->SetBlockMaterialColor(attributes, ++realflatidx, pinMaterial);
                }
              }
            for(unsigned int idx=0; idx<pinCell->frustums.size(); idx++)
              {
              realflatidx++; // increase one for this frustum
              for(int k = 0; k < pinCell->GetNumberOfLayers(); k++)
                {
                pinMaterial = pinCell->frustums[idx]->materials[k];
                matColorMap->SetBlockMaterialColor(attributes, ++realflatidx, pinMaterial);
                }
              }
            }
          pin_count++;
          }
        else // ducts need to color by layers
          {
          if(vtkMultiBlockDataSet* ductBlock =
            vtkMultiBlockDataSet::SafeDownCast(data->GetBlock(idx)))
            {
            Duct* duct = assy->AssyDuct.Ducts[ducts_count];
            unsigned int numBlocks = ductBlock->GetNumberOfBlocks();
            for(unsigned int b=0; b<numBlocks; b++)
              {
              std::string layerMaterial =
                (duct && b<duct->materials.size()) ? duct->materials[b] : "duct";
              if(layerMaterial.empty())
                {
                layerMaterial = "duct";
                }
              layerMaterial = QString(layerMaterial.c_str()).toLower().toStdString();
              matColorMap->SetBlockMaterialColor(attributes, ++realflatidx, layerMaterial);
              }
            ducts_count++;
            }
          }
        }
      }
    }
}

void cmbNucMainWindow::onShowPreferences()
{
}

void cmbNucMainWindow::onRunAssygen()
{
  // write input file
  QTemporaryFile file("XXXXXX.inp");
  file.setAutoRemove(false);
  file.open();
  file.close();
  saveFile(file.fileName());

  QString input_file_name = file.fileName();
  input_file_name.chop(4); // remove '.inp' suffix

  QString assygen = "/home/kyle/SiMBA/meshkit/build/meshkit/src/meshkit-build/rgg/assygen";
  QStringList args;
  args.append(input_file_name);
  QProcess proc;
  proc.start(assygen, args);
  qDebug() << "ran: " << assygen << input_file_name;

  proc.waitForFinished(-1);
  qDebug() << proc.readAllStandardOutput();
  qDebug() << proc.readAllStandardError();
}

void cmbNucMainWindow::zScaleChanged(int value)
{
  this->ZScale = 1.0 / value;
  this->Actor->SetScale(1, 1, this->ZScale);
  this->ResetView();

  emit updateGlobalZScale(this->ZScale);
}

void cmbNucMainWindow::ResetView()
{
  this->Renderer->ResetCamera();
  this->ui->qvtkWidget->update();
}
