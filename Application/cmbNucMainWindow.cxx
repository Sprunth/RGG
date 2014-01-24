#include "cmbNucMainWindow.h"

#include "ui_qNucMainWindow.h"

#include <vtkActor.h>
#include <vtkAlgorithm.h>
#include <vtkAxesActor.h>
#include <vtkCamera.h>
#include <vtkCompositeDataDisplayAttributes.h>
#include <vtkCompositePolyDataMapper2.h>
#include <vtkCompositeDataPipeline.h>
#include <vtkDataObjectTreeIterator.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkInformation.h>
#include <vtkInteractorObserver.h>
#include <vtkNew.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkXMLMultiBlockDataWriter.h>

#include <QFileDialog>
#include <QFileInfo>
#include <QStringList>
#include <QDebug>
#include <QDockWidget>
#include <QProcess>
#include <QTemporaryFile>
#include <QSettings>
#include <QTimer>

#include "cmbNucAssembly.h"
#include "cmbNucCore.h"
#include "cmbNucInputPropertiesWidget.h"
#include "cmbNucInputListWidget.h"
#include "cmbNucMaterialColors.h"
#include "cmbNucNewDialog.h"
#include "cmbNucExportDialog.h"
#include "cmbNucPartsTreeItem.h"
#include "cmbNucExport.h"
#include "cmbNucPreferencesDialog.h"

#include "vtkCmbLayeredConeSource.h"

int numAssemblyDefaultColors = 42;
int defaultAssemblyColors[][3] = 
{
  {66,146,198},
  {241,105,19},
  {65,171,93},
  {239,59,44},
  {128,125,186},
  {115,115,115},  
  {198,219,239},
  {253,208,162},
  {199,233,192},
  {252,187,161},
  {218,218,235},
  {217,217,217}, 
  {8,81,156},
  {166,54,3},
  {0,109,44},
  {165,15,21},
  {84,39,143},
  {37,37,37},  
  {158,202,225},
  {253,174,107},
  {161,217,155},
  {252,146,114},
  {188,189,220},
  {189,189,189},  
  {33,113,181},
  {217,72,1},
  {35,139,69},
  {203,24,29},
  {106,81,163},
  {82,82,82},  
  {107,174,214},
  {253,141,60},
  {116,196,118},
  {251,106,74},
  {158,154,200},
  {150,150,150}, 
  {8,48,10},
  {127,39,4},
  {0,68,27},
  {103,0,13},
  {63,0,125},
  {0,0,0}
};

// Constructor
cmbNucMainWindow::cmbNucMainWindow()
{
//  vtkNew<vtkCompositeDataPipeline> compositeExec;
//  vtkAlgorithm::SetDefaultExecutivePrototype(compositeExec.GetPointer());

  this->ui = new Ui_qNucMainWindow;
  this->ui->setupUi(this);
  this->NuclearCore = new cmbNucCore();

  this->NewDialog = new cmbNucNewDialog(this);
  this->ExportDialog = new cmbNucExportDialog(this);
  this->Preferences = new cmbNucPreferencesDialog(this);

  connect(this->NewDialog, SIGNAL(accepted()), this, SLOT(onNewDialogAccept()));

  // VTK/Qt wedded
  this->Renderer = vtkSmartPointer<vtkRenderer>::New();
  vtkRenderWindow *renderWindow = this->ui->qvtkWidget->GetRenderWindow();
  renderWindow->AddRenderer(this->Renderer);
  this->VTKToQt = vtkSmartPointer<vtkEventQtSlotConnect>::New();

  // setup depth peeling
  renderWindow->SetAlphaBitPlanes(1);
  renderWindow->SetMultiSamples(0);
  this->Renderer->SetUseDepthPeeling(1);
  this->Renderer->SetMaximumNumberOfPeels(5);

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
  connect(this->ui->actionOpenFile, SIGNAL(triggered()), this, SLOT(onFileOpenAssembly()));
  connect(this->ui->actionOpenCoreFile, SIGNAL(triggered()), this, SLOT(onFileOpenCore()));
  connect(this->ui->actionSaveFile, SIGNAL(triggered()), this, SLOT(onFileSave()));
  connect(this->ui->actionNew, SIGNAL(triggered()), this, SLOT(onFileNew()));
  connect(this->ui->actionPreferences, SIGNAL(triggered()),
          this->Preferences, SLOT(setPreferences()));
  connect(this->ui->actionRunAssygen, SIGNAL(triggered()), this, SLOT(onRunAssygen()));
  connect(this->ui->actionExport, SIGNAL(triggered()), this, SLOT(exportRGG()));
  connect(this->ui->actionParallel_Projection, SIGNAL(triggered(bool)),
          this, SLOT(useParallelProjection(bool)));

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

  //setup camera interaction to render more quickly and less precisely
  vtkInteractorObserver *iStyle = renderWindow->GetInteractor()->GetInteractorStyle();
  this->VTKToQt->Connect(
    iStyle, vtkCommand::StartInteractionEvent,
    this, SLOT(onInteractionTransition( vtkObject*, unsigned long)));
  this->VTKToQt->Connect(
    iStyle, vtkCommand::EndInteractionEvent,
    this, SLOT(onInteractionTransition( vtkObject*, unsigned long)));

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
  QObject::connect(this->InputsWidget, SIGNAL(pinsModified(cmbNucAssembly*)),
    this->PropertyWidget, SLOT(resetAssemblyEditor(cmbNucAssembly*)));
  QObject::connect(this->InputsWidget, SIGNAL(assembliesModified(cmbNucCore*)),
    this->PropertyWidget, SLOT(resetCore(cmbNucCore*)));

  QObject::connect(this->PropertyWidget,
    SIGNAL(objGeometryChanged(AssyPartObj*)), this,
    SLOT(onObjectGeometryChanged(AssyPartObj*)));
  QObject::connect(this->PropertyWidget,
    SIGNAL(currentObjectNameChanged(const QString&)), this,
    SLOT(updatePropertyDockTitle(const QString&)));
  QObject::connect(this->InputsWidget,
    SIGNAL(materialColorChanged(const QString&)), this,
    SLOT(onObjectModified()));
  QObject::connect(this->InputsWidget,
    SIGNAL(materialVisibilityChanged(const QString&)), this,
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

  // update RenderWindow for Core or selected Assembly.
  // Eventually, the pin editor will also be replaced using MainWindow's
  // UI panel and render window.
  if(selObj->GetType() == CMBNUC_CORE)
    {
    this->updateCoreMaterialColors();
    this->ResetView();
    }
  else if( selObj->GetType() == CMBNUC_ASSY_PINCELL ||
          selObj->GetType() == CMBNUC_ASSY_CYLINDER_PIN ||
          selObj->GetType() == CMBNUC_ASSY_FRUSTUM_PIN )
    {
    PinCell* selPin = NULL;
    if(selObj->GetType() == CMBNUC_ASSY_PINCELL)
      {
      selPin = dynamic_cast<PinCell*>(selObj);
      }
    else
      {
      cmbNucPartsTreeItem* selItem = this->InputsWidget->getSelectedPartNode();
      cmbNucPartsTreeItem* pItem = dynamic_cast<cmbNucPartsTreeItem*>(selItem->parent());
      if(pItem && pItem->getPartObject())
        {
        selPin = dynamic_cast<PinCell*>(pItem->getPartObject());
        }
      }
    if(selPin)
      {
      this->updatePinCellMaterialColors(selPin);
      this->ResetView();
      }
    }
  else // assemblies or ducts
    {
    cmbNucAssembly* assy = this->InputsWidget->getCurrentAssembly();
    this->updateAssyMaterialColors(assy);
    this->ResetView();
    }
}

void cmbNucMainWindow::onObjectGeometryChanged(AssyPartObj* obj)
{
  if(!obj)
    {
    return;
    }

  if(obj->GetType() == CMBNUC_CORE)
    {
    this->onObjectModified(obj);
    }
  else
    {
    // recreate Assembly geometry.
    cmbNucAssembly* assy = this->InputsWidget->getCurrentAssembly();
    if(assy)
      {
      assy->CreateData();
      }
    if( obj->GetType() == CMBNUC_ASSY_PINCELL )
      {
      PinCell* selPin = dynamic_cast<PinCell*>(obj);
      if(selPin)
        {
        this->updatePinCellMaterialColors(selPin);
        }
      }
    else if(assy)
      {
      this->updateAssyMaterialColors(assy);
      }
    this->ui->qvtkWidget->update();
    }
}

void cmbNucMainWindow::onObjectModified(AssyPartObj* obj)
{
  // update material colors
  this->updateCoreMaterialColors();

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
  this->NewDialog->show();
}

void cmbNucMainWindow::onNewDialogAccept()
{
  this->PropertyWidget->setGeometryType(this->NewDialog->getSelectedGeometry());
  this->PropertyWidget->setObject(NULL, NULL);
  this->PropertyWidget->setAssembly(NULL);
  this->NuclearCore->CoreLattice.SetGeometryType(
    this->PropertyWidget->getGeometryType());
  this->InputsWidget->onNewAssembly();
  this->Renderer->ResetCamera();
  this->Renderer->Render();
}

void cmbNucMainWindow::onFileOpenAssembly()
{
  // Use cached value for last used directory if there is one,
  // or default to the user's home dir if not.
  QSettings settings("CMBNuclear", "CMBNuclear");
  QDir dir = settings.value("cache/lastDir", QDir::homePath()).toString();

  QStringList fileNames =
    QFileDialog::getOpenFileNames(this,
                                 "Open Assygen File...",
                                 dir.path(),
                                 "INP Files (*.inp)");
  if(fileNames.count()==0)
    {
    return;
    }
  // Cache the directory for the next time the dialog is opened
  QFileInfo info(fileNames[0]);
  settings.setValue("cache/lastDir", info.dir().path());

  this->openAssemblyFiles(fileNames);
  // In case the loaded assembly adds new materials
  this->PropertyWidget->updateMaterials();

  // update render view
  this->Renderer->ResetCamera();
  this->Renderer->Render();
}

void cmbNucMainWindow::onFileOpenCore()
{
  // Use cached value for last used directory if there is one,
  // or default to the user's home dir if not.
  QSettings settings("CMBNuclear", "CMBNuclear");
  QDir dir = settings.value("cache/lastDir", QDir::homePath()).toString();

  QStringList fileNames =
    QFileDialog::getOpenFileNames(this,
                                 "Open Coregen File...",
                                 dir.path(),
                                 "INP Files (*.inp)");
  if(fileNames.count()==0)
    {
    return;
    }

  this->setCursor(Qt::BusyCursor);
  // clear old assembly
  this->PropertyWidget->setObject(NULL, NULL);
  this->PropertyWidget->setAssembly(NULL);

  // Cache the directory for the next time the dialog is opened
  QFileInfo info(fileNames[0]);
  settings.setValue("cache/lastDir", info.dir().path());

  this->NuclearCore->ReadFile(fileNames[0].toStdString(), numAssemblyDefaultColors,
                              defaultAssemblyColors);

  int numNewAssy = this->NuclearCore->GetNumberOfAssemblies();
  if(numNewAssy)
    {
    this->PropertyWidget->setGeometryType(
      this->NuclearCore->CoreLattice.GetGeometryType());
    }

  // update data colors
  this->updateCoreMaterialColors();

  // In case the loaded core adds new materials
  this->PropertyWidget->updateMaterials();
  this->InputsWidget->updateUI(numNewAssy);
  this->unsetCursor();

  // update render view
  this->Renderer->ResetCamera();
  this->Renderer->Render();
}

void cmbNucMainWindow::openAssemblyFiles(const QStringList &fileNames)
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
      QFileInfo finfo(fileName);
      std::string label = finfo.completeBaseName().toStdString();
      cmbNucAssembly* assy = this->NuclearCore->loadAssemblyFromFile(
        fileName.toStdString(), label);
      int acolorIndex = numExistingAssy + assemblies.size() % numAssemblyDefaultColors;
      std::cerr << "a color index = " << acolorIndex << "\n";

      QColor acolor(defaultAssemblyColors[acolorIndex][0],
                    defaultAssemblyColors[acolorIndex][1],
                    defaultAssemblyColors[acolorIndex][2]);
      assy->SetLegendColor(acolor);
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
      this->NuclearCore->SetAssemblyLabel(i, 0, assemblies.at(i)->label, assemblies.at(i)->GetLegendColor());
      std::cerr << "Setting color!\n";
      for(int j=1; j<this->NuclearCore->CoreLattice.Grid[i].size() ; j++)
        {
        this->NuclearCore->ClearAssemblyLabel(i, j);
        }
      }
    }

  if(assemblies.count())
    {
    this->PropertyWidget->setGeometryType(
      assemblies.at(0)->AssyLattice.GetGeometryType());
    }

  // update data colors
  this->updateCoreMaterialColors();
  // render
  this->InputsWidget->updateUI(numExistingAssy==0 && numNewAssy>1);

  this->unsetCursor();
}

void cmbNucMainWindow::onFileSave()
{
  // Use cached value for last used directory if there is one,
  // or default to the user's home dir if not.
  QSettings settings("CMBNuclear", "CMBNuclear");
  QDir dir = settings.value("cache/lastDir", QDir::homePath()).toString();
  QString fileName =
    QFileDialog::getSaveFileName(this,
                                 "Save Assygen File...",
                                 dir.path(),
                                 "INP Files (*.inp);; vtk Files (*.vtm)");
  if(!fileName.isEmpty())
    {
    // Cache the directory for the next time the dialog is opened
    QFileInfo info(fileName);
    settings.setValue("cache/lastDir", info.dir().path());

    this->setCursor(Qt::BusyCursor);

    QString ext = info.suffix();
    if (ext == QString("vtm"))
      {
      this->exportVTKFile(fileName);
      }
    else
      {
      this->saveFile(fileName);
      }
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

void cmbNucMainWindow::updatePinCellMaterialColors(PinCell* pin)
{
  if(!pin)
    {
    return;
    }
  vtkMultiBlockDataSet *data_set = pin->CachedData;

  this->Mapper->SetInputDataObject(data_set);
  vtkCompositeDataDisplayAttributes *attributes =
    this->Mapper->GetCompositeDataDisplayAttributes();

  size_t numCyls = pin->cylinders.size();
  size_t numFrus = pin->frustums.size();
  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  unsigned int flat_index = 1; // start from first child
  for(unsigned int idx=0; idx<data_set->GetNumberOfBlocks(); idx++)
    {
    std::string pinMaterial;
    vtkMultiBlockDataSet* aSection = vtkMultiBlockDataSet::SafeDownCast(
      data_set->GetBlock(idx));
    if(idx < numCyls)
      {
      flat_index++; // increase one for this cylinder
      for(int k = 0; k < pin->GetNumberOfLayers(); k++)
        {
        pinMaterial = pin->cylinders[idx]->materials[k];
        matColorMap->SetBlockMaterialColor(attributes, flat_index++, pinMaterial);
        }
      }
    else
      {
      flat_index++; // increase one for this frustum
      for(int k = 0; k < pin->GetNumberOfLayers(); k++)
        {
        pinMaterial = pin->frustums[idx-numCyls]->materials[k];
        matColorMap->SetBlockMaterialColor(attributes, flat_index++, pinMaterial);
        }
      }
    }
}

void cmbNucMainWindow::updateAssyMaterialColors(cmbNucAssembly* assy)
{
  if(!assy)
    {
    return;
    }

  // regenerate core and assembly view
  vtkSmartPointer<vtkMultiBlockDataSet> assydata = assy->GetData();
  this->Mapper->SetInputDataObject(assydata);
  if(!assydata)
    {
    return;
    }
  vtkCompositeDataDisplayAttributes *attributes =
    this->Mapper->GetCompositeDataDisplayAttributes();

  unsigned int realflatidx = 0;
  assy->updateMaterialColors(realflatidx, attributes);
}

void cmbNucMainWindow::exportVTKFile(const QString &fileName)
{
  if(!this->InputsWidget->getCurrentAssembly())
    {
    qDebug() << "no assembly to save";
    return;
    }

  vtkSmartPointer<vtkMultiBlockDataSet> coredata = this->NuclearCore->GetData();
  vtkSmartPointer<vtkXMLMultiBlockDataWriter> writer =
    vtkSmartPointer<vtkXMLMultiBlockDataWriter>::New();
  writer->SetInputData(coredata);
  writer->SetFileName(fileName.toLocal8Bit().data());
  writer->Write();
}

void cmbNucMainWindow::updateCoreMaterialColors()
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

  unsigned int realflatidx = 0;
  for(unsigned int block = 0; block < numCoreBlocks; block++)
    {
    realflatidx++; // for assembly block
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

      assy->updateMaterialColors(realflatidx, attributes);
      }
    }
}

void cmbNucMainWindow::exportRGG()
{
  this->ExportDialog->exportFile(NuclearCore);
}

void cmbNucMainWindow::onRunAssygen()
{
#if 0
  std::vector<std::string> assygenInput;
  assygenInput.push_back("/Users/jacobbecker/Documents/sixth_hexflatcore/a1");
  assygenInput.push_back("/Users/jacobbecker/Documents/sixth_hexflatcore/a2");
  assygenInput.push_back("/Users/jacobbecker/Documents/sixth_hexflatcore/a3");
  std::string coregenInput = "/Users/jacobbecker/Documents/sixth_hexflatcore/sixth_hexflatcore";
  std::string assygenExe = "/Users/jacobbecker/Programing/SiMBA/build/meshkit/install/bin/assygen";
  std::string cubitExe = "/Applications/Cubit-13.1/Cubit.app/Contents/MacOS/Cubit";
  std::string coregenExe = "/Users/jacobbecker/Programing/SiMBA/build/meshkit/install/bin/coregen";
  std::string em;
  qDebug() << "EXPORT!";
  if(!cmbNucExport::cmbNucExport(assygenExe,cubitExe,coregenExe,assygenInput,coregenInput,em))
    {
    qDebug() << QString(em.c_str());
    }
  else
    qDebug() << QString("Successfully RAN");
  // write input file
#endif
#if 0
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
#endif
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

void cmbNucMainWindow::onInteractionTransition(vtkObject * obj, unsigned long event)
{
  switch (event)
    {
    case vtkCommand::StartInteractionEvent:
      //this->Renderer->UseDepthPeelingOff();
      this->Renderer->SetMaximumNumberOfPeels(3);
      break;
    case vtkCommand::EndInteractionEvent:
      //this->Renderer->UseDepthPeelingOn();
      this->Renderer->SetMaximumNumberOfPeels(100);
      break;
    }
}

void cmbNucMainWindow::useParallelProjection(bool val)
{
  if (val)
    {
    this->Renderer->GetActiveCamera()->ParallelProjectionOn();
    }
  else
    {
    this->Renderer->GetActiveCamera()->ParallelProjectionOff();
    }
  this->ui->qvtkWidget->update();
}

void cmbNucMainWindow::updatePropertyDockTitle(const QString& title)
{
  if (title == "")
    {
    this->ui->PropertyDock->setWindowTitle("Properties");
    }
  else
    {
    this->ui->PropertyDock->setWindowTitle(title);
    }
}
