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

  // Hardcoded materials and  colors
  this->MaterialColors.insert("gap", QColor::fromRgbF(0.0, 0.0, 0.0, 0.0));
  this->MaterialColors.insert("MaterialBlock", QColor::fromRgbF(.7, .1, .7, 1.0));
  this->MaterialColors.insert("c1", QColor::fromRgbF(0.3, 0.5, 1.0, 1.0));
  this->MaterialColors.insert("m3", QColor::fromRgbF(1.0, 0.1, 0.1, 1.0));
  this->MaterialColors.insert("fuel1", QColor::fromRgbF(1.0, 0.1, 0.1, 1.0));
  this->MaterialColors.insert("fuel2", QColor::fromRgbF(1.0, 0.5, 0.5, 1.0));
  this->MaterialColors.insert("cntr1", QColor::fromRgbF(0.4, 1.0, 0.4, 1.0));
  this->MaterialColors.insert("graphite", QColor::fromRgbF(.4, .4, .4, 1.0));
  this->MaterialColors.insert("metal", QColor::fromRgbF(.6, .6, .6, 1.0));
  this->MaterialColors.insert("coolant", QColor::fromRgbF(0.3, 0.5, 1.0, 1.0));
  this->MaterialColors.insert("fuel_uox1", QColor::fromRgbF(0.694, 0.0, 0.149, 1.0));
  this->MaterialColors.insert("fuel_uox2", QColor::fromRgbF(0.890, 0.102, 0.110, 1.0));
  this->MaterialColors.insert("mox_43", QColor::fromRgbF(0.988, 0.306, 0.165, 1.0));
  this->MaterialColors.insert("mox_73", QColor::fromRgbF(0.992, 0.553, 0.235, 1.0));
  this->MaterialColors.insert("mox_87", QColor::fromRgbF(0.996, 0.698, 0.298, 1.0));
  this->MaterialColors.insert("water", QColor::fromRgbF(0.651, 0.741, 0.859, 0.5));
  this->MaterialColors.insert("water_rod", QColor::fromRgbF(0.212, 0.565, 0.753, 1.0));
  this->MaterialColors.insert("barod16", QColor::fromRgbF(0.000, 0.427, 0.173, 1.0));
  this->MaterialColors.insert("barod18", QColor::fromRgbF(0.192, 0.639, 0.329, 1.0));
  this->MaterialColors.insert("barod28", QColor::fromRgbF(0.455, 0.769, 0.463, 1.0));
  this->MaterialColors.insert("control_rod", QColor::fromRgbF(0.729, 0.894, 0.702, 1.0));
  this->MaterialColors.insert("cladding", QColor::fromRgbF(0.75, 0.75, 0.75, 1.0));
  

  // default pin and duct color
  this->MaterialColors.insert("pin", QColor::fromRgbF(1.0, 0.1, 0.1));
  this->MaterialColors.insert("duct", QColor::fromRgbF(1.0, 1.0, 1.0));
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
  if(fileNames.count()==0)
    {
    return;
    }
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

  //vtkDataObjectTreeIterator *coreiter = coredata->NewTreeIterator();
  //coreiter->SetSkipEmptyNodes(false);
  unsigned int realflatidx=0;
  for(unsigned int block=0; block<numCoreBlocks; block++)
    {
    realflatidx++;
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
      int pins = dimensions.first * dimensions.second;

//      vtkDataObjectTreeIterator *iter = data->NewTreeIterator();
//      iter->SetSkipEmptyNodes(false);
      int pin_count = 0;
      int ducts_count = 0;

      std::string pinMaterial = "pin";
      PinCell* pinCell = assy->PinCells.size()>0 ? assy->PinCells[0] : NULL;
      if(pinCell)
        {
        if(pinCell->cylinders.size()>0)
          {
          pinMaterial = pinCell->cylinders[0]->material;
          }
        else if(pinCell->frustums.size()>0)
          {
          pinMaterial = pinCell->frustums[0]->material;
          }
        }
      pinMaterial = QString(pinMaterial.c_str()).toLower().toStdString();
//      while(!iter->IsDoneWithTraversal())
      int numAssyBlocks = data->GetNumberOfBlocks();
      for(unsigned int idx=0; idx<numAssyBlocks; idx++)
        {
        realflatidx++;
        if(pin_count < pins)
          {
          int i = realflatidx;
          pinMaterial = assy->GetCellMaterial(pin_count);
          pinMaterial = QString(pinMaterial.c_str()).toLower().toStdString();
          //std::cout << "Pin Material = " << pinMaterial << "\n";
          QColor pinColor = this->MaterialColors[pinMaterial.c_str()];
          double color[] = { pinColor.redF(), pinColor.greenF(), pinColor.blueF() };
          attributes->SetBlockColor(i, color);
          attributes->SetBlockOpacity(i, pinColor.alphaF());
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
              int i = ++realflatidx;
              QColor matColor = this->MaterialColors.value(layerMaterial.c_str());
              double color[] = { matColor.redF(), matColor.greenF(), matColor.blueF() };
              attributes->SetBlockColor(i, color);
              attributes->SetBlockOpacity(i, matColor.alphaF());
              }
            ducts_count++;
            }
          }
        }
      }
    }
}
