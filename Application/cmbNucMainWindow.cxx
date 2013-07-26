#include "cmbNucMainWindow.h"

#include "ui_qNucMainWindow.h"
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCompositePolyDataMapper2.h>
#include <QFileDialog>
#include <QStringList>
#include <QDebug>
#include <QDockWidget>

#include "cmbNucAssembly.h"
#include "cmbNucCore.h"
#include "cmbNucInputPropertiesWidget.h"
#include "cmbNucInputListWidget.h"

#include "vtkAxesActor.h"
#include "vtkProperty.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkInformation.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkAlgorithm.h"
#include "vtkNew.h"

// Constructor
cmbNucMainWindow::cmbNucMainWindow()
{
  // vtkNew<vtkCompositeDataPipeline> compositeExec;
  // vtkAlgorithm::SetDefaultExecutivePrototype(compositeExec.GetPointer());

  this->ui = new Ui_qNucMainWindow;
  this->ui->setupUi(this);
  this->NuclearCore = new cmbNucCore();

  this->initPanels();

  // VTK/Qt wedded
  this->Renderer = vtkSmartPointer<vtkRenderer>::New();
  this->ui->qvtkWidget->GetRenderWindow()->AddRenderer(this->Renderer);
  this->Mapper = vtkSmartPointer<vtkCompositePolyDataMapper2>::New();
  this->Actor = vtkSmartPointer<vtkActor>::New();
  this->Actor->SetMapper(this->Mapper.GetPointer());
  this->Actor->GetProperty()->SetShading(1);
  this->Actor->GetProperty()->SetInterpolationToPhong();
//  this->Actor->GetProperty()->EdgeVisibilityOn();
  this->Renderer->AddActor(this->Actor);

  vtkCompositeDataDisplayAttributes *attributes = vtkCompositeDataDisplayAttributes::New();
  this->Mapper->SetCompositeDataDisplayAttributes(attributes);
  attributes->Delete();

  // add axes actor
  vtkAxesActor *axesActor = vtkAxesActor::New();
  this->Renderer->AddActor(axesActor);
  axesActor->Delete();

  // Set up action signals and slots
  connect(this->ui->actionExit, SIGNAL(triggered()), this, SLOT(onExit()));
  connect(this->ui->actionOpenFile, SIGNAL(triggered()), this, SLOT(onFileOpen()));
  connect(this->ui->actionSaveFile, SIGNAL(triggered()), this, SLOT(onFileSave()));
  connect(this->ui->actionNew, SIGNAL(triggered()), this, SLOT(onFileNew()));

  // Hardcoded material colors
  this->MaterialColors.insert("g1", QColor::fromRgbF(0.5, 0.5, 0.5, 0.5));
  this->MaterialColors.insert("c1", QColor::fromRgbF(0.6, 0.4, 0.2, 0.7));
  this->MaterialColors.insert("m3", QColor::fromRgbF(1.0, 0.4, 0.0));
}

cmbNucMainWindow::~cmbNucMainWindow()
{
  this->PropertyWidget->setObject(NULL, NULL);
  this->PropertyWidget->setAssembly(NULL);
  this->InputsWidget->setCore(NULL);
  delete this->NuclearCore;
}

void cmbNucMainWindow::initPanels()
{
  this->InputsWidget = new cmbNucInputListWidget(this);
  this->PropertyWidget = new cmbNucInputPropertiesWidget(this);
  this->ui->InputsDock->setWidget(this->InputsWidget);
  this->ui->PropertyDock->setWidget(this->PropertyWidget);
  this->ui->PropertyDock->setEnabled(0);
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
      cmbNucAssembly* assy = this->openFile(fileName);
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
  this->Renderer->ResetCamera();
  this->Renderer->Render();
  this->InputsWidget->updateUI(numExistingAssy==0 && numNewAssy>1);

  this->unsetCursor();
}

cmbNucAssembly* cmbNucMainWindow::openFile(const QString &fileName)
{
  // read file and create new assembly
  cmbNucAssembly* assembly = new cmbNucAssembly;
  assembly->label = QString("Assembly").append(
    QString::number(this->NuclearCore->GetNumberOfAssemblies()+1)).toStdString();
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

  for(unsigned int block=0; block<coredata->GetNumberOfBlocks(); block++)
    {
    if(!coredata->GetBlock(block))
      {
      continue;
      }
    vtkMultiBlockDataSet* data = vtkMultiBlockDataSet::SafeDownCast(coredata->GetBlock(block));
    if(!data)
      {
      continue;
      }
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
      int pins = dimensions.first * dimensions.second;
      vtkCompositeDataDisplayAttributes *attributes =
        this->Mapper->GetCompositeDataDisplayAttributes();

      vtkDataObjectTreeIterator *iter = data->NewTreeIterator();
      iter->SetSkipEmptyNodes(false);
      int pin_count = 0;

      while(!iter->IsDoneWithTraversal())
        {
        int i = iter->GetCurrentFlatIndex();
        if(pin_count < pins)
          {
          double color[] = { 1.0, 0.4, 0.0 };
          attributes->SetBlockColor(i, color);
          pin_count++;
          }
        else
          {
          double color[] = { 0.6, 0.4, 0.2 };
          attributes->SetBlockColor(i, color);
          attributes->SetBlockOpacity(i, 0.7);
          }
        iter->GoToNextItem();
        }
      iter->Delete();
      }
    }
}
