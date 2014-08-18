#include "cmbNucMainWindow.h"

#include "ui_qNucMainWindow.h"

#include <vtkActor.h>
#include <vtkAlgorithm.h>
#include <vtkAxesActor.h>
#include <vtkCubeAxesActor.h>
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
#include "vtkTransformFilter.h"
#include <vtkTextProperty.h>
#include <vtkBoundingBox.h>
#include "vtkMath.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QStringList>
#include <QDebug>
#include <QDockWidget>
#include <QProcess>
#include <QTemporaryFile>
#include <QSettings>
#include <QTimer>
#include <QMessageBox>

#include "cmbNucAssembly.h"
#include "cmbNucCore.h"
#include "cmbNucInputPropertiesWidget.h"
#include "cmbNucInputListWidget.h"
#include "cmbNucMaterialColors.h"
#include "cmbNucExportDialog.h"
#include "cmbNucPartsTreeItem.h"
#include "cmbNucExport.h"
#include "cmbNucPreferencesDialog.h"
#include "cmbNucMaterial.h"
#include "inpFileIO.h"

#include "vtkCmbLayeredConeSource.h"
#include "cmbNucLatticeWidget.h"

#ifdef BUILD_WITH_MOAB
#include "vtk_moab_reader/vtkMoabReader.h"
#include "cmbNucCoregen.h"
#endif

namespace
{
  void computeBounds(vtkMultiBlockDataSet * dataset, vtkBoundingBox * box)
  {
    if(dataset == NULL) return;
    // move the assembly to the correct position
    for(int idx=0; idx<dataset->GetNumberOfBlocks(); idx++)
    {
      // Brutal. I wish the SetDefaultExecutivePrototype had workd :(
      if(vtkDataObject* objBlock = dataset->GetBlock(idx))
      {
        if(vtkMultiBlockDataSet* assyPartBlock =
           vtkMultiBlockDataSet::SafeDownCast(objBlock))
        {
          computeBounds(assyPartBlock, box);
        }
        else
        {
          vtkDataSet* part = vtkDataSet::SafeDownCast(objBlock);
          double tmpb[6];
          part->GetBounds(tmpb);
          box->AddBounds(tmpb);
        }
      }
    }
  }
}

class NucMainInternal
{
public:
#ifdef BUILD_WITH_MOAB
  cmbNucCoregen *MoabSource;
#endif

  vtkSmartPointer<vtkMultiBlockDataSet> CurrentDataset;
  DuctCell* CurrentDuctCell;
  PinCell* CurrentPinCell;
  bool CamerasLinked;
  vtkSmartPointer<vtkCamera> UnlinkCameraModel;
  vtkSmartPointer<vtkCamera> UnlinkCameraMesh;
  vtkSmartPointer<vtkCamera> LinkCamera;
  bool MeshOpen;
  bool IsCoreView;
  bool IsFullMesh;
  double BoundsModel[6];
  double BoundsMesh[6];
};

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
  isCameraIsMoving = false;

  this->ui = new Ui_qNucMainWindow;
  this->setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
  this->setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
  this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
  this->ui->setupUi(this);
  this->NuclearCore = new cmbNucCore(false);
  setTitle();

  this->tabifyDockWidget(this->ui->Dock2D, this->ui->Dock3D);
  this->tabifyDockWidget(this->ui->Dock3D, this->ui->DockMesh);
  this->ui->Dock3D->raise();
  this->ui->DockMesh->setVisible(false);
  this->ui->meshSubcomponent->setVisible(false);
  this->ui->drawEdges->setVisible(false);
  //this->ui->meshControls->setVisible(false);

  LatticeDraw = new cmbNucLatticeWidget(this);
  this->ui->Dock2D->setWidget(LatticeDraw);

  this->ExportDialog = new cmbNucExportDialog(this);
  this->Preferences = new cmbNucPreferencesDialog(this);
  Internal = new NucMainInternal();
  this->Internal->IsCoreView = false;
  this->Internal->IsFullMesh = false;
  this->Internal->CamerasLinked = false;
#ifdef BUILD_WITH_MOAB
  this->Internal->MoabSource = NULL;
#endif

  // VTK/Qt wedded
  this->Renderer = vtkSmartPointer<vtkRenderer>::New();
  this->MeshRenderer = vtkSmartPointer<vtkRenderer>::New();
  vtkRenderWindow *renderWindow = this->ui->qvtkWidget->GetRenderWindow();
  vtkRenderWindow *meshRenderWindow = this->ui->qvtkMeshWidget->GetRenderWindow();
  renderWindow->AddRenderer(this->Renderer);
  meshRenderWindow->AddRenderer(this->MeshRenderer);
  this->VTKToQt = vtkSmartPointer<vtkEventQtSlotConnect>::New();

  // setup depth peeling
  renderWindow->SetAlphaBitPlanes(1);
  renderWindow->SetMultiSamples(0);
  meshRenderWindow->SetAlphaBitPlanes(1);
  meshRenderWindow->SetMultiSamples(0);
  this->Renderer->SetUseDepthPeeling(1);
  this->Renderer->SetMaximumNumberOfPeels(5);
  this->MeshRenderer->SetUseDepthPeeling(1);
  this->MeshRenderer->SetMaximumNumberOfPeels(5);

  this->Mapper = vtkSmartPointer<vtkCompositePolyDataMapper2>::New();
  this->Mapper->SetScalarVisibility(0);
  this->MeshMapper = vtkSmartPointer<vtkCompositePolyDataMapper2>::New();
  this->MeshMapper->SetScalarVisibility(0);
  this->Actor = vtkSmartPointer<vtkActor>::New();
  this->Actor->SetMapper(this->Mapper.GetPointer());
  this->Actor->GetProperty()->SetShading(1);
  this->Actor->GetProperty()->SetInterpolationToPhong();
  this->MeshActor = vtkSmartPointer<vtkActor>::New();
  this->MeshActor->SetMapper(this->MeshMapper.GetPointer());
  this->MeshActor->GetProperty()->SetShading(1);
  this->MeshActor->GetProperty()->SetInterpolationToPhong();
  this->Renderer->AddActor(this->Actor);
  this->MeshRenderer->AddActor(this->MeshActor);

  CubeAxesActor = vtkSmartPointer<vtkCubeAxesActor>::New();
  MeshCubeAxesActor = vtkSmartPointer<vtkCubeAxesActor>::New();
  CubeAxesActor->SetCamera(this->Renderer->GetActiveCamera());
  CubeAxesActor->GetTitleTextProperty(0)->SetColor(1.0, 0.0, 0.0);
  CubeAxesActor->GetLabelTextProperty(0)->SetColor(1.0, 0.0, 0.0);
  CubeAxesActor->GetLabelTextProperty(0)->SetOrientation(90);

  CubeAxesActor->GetTitleTextProperty(1)->SetColor(0.0, 1.0, 0.0);
  CubeAxesActor->GetLabelTextProperty(1)->SetColor(0.0, 1.0, 0.0);
  CubeAxesActor->GetLabelTextProperty(1)->SetOrientation(90);

  CubeAxesActor->GetTitleTextProperty(2)->SetColor(0.0, 0.5, 1.0);
  CubeAxesActor->GetLabelTextProperty(2)->SetColor(0.0, 0.5, 1.0);
  CubeAxesActor->GetLabelTextProperty(2)->SetOrientation(90);

  CubeAxesActor->DrawXGridlinesOn();
  CubeAxesActor->DrawYGridlinesOn();
  CubeAxesActor->DrawZGridlinesOn();

  CubeAxesActor->XAxisMinorTickVisibilityOn();
  CubeAxesActor->YAxisMinorTickVisibilityOn();
  CubeAxesActor->ZAxisMinorTickVisibilityOn();
  CubeAxesActor->SetRebuildAxes(true);

  CubeAxesActor->SetGridLineLocation(VTK_GRID_LINES_FURTHEST);

  MeshCubeAxesActor->SetCamera(this->Renderer->GetActiveCamera());
  MeshCubeAxesActor->GetTitleTextProperty(0)->SetColor(1.0, 0.0, 0.0);
  MeshCubeAxesActor->GetLabelTextProperty(0)->SetColor(1.0, 0.0, 0.0);
  MeshCubeAxesActor->GetLabelTextProperty(0)->SetOrientation(90);

  MeshCubeAxesActor->GetTitleTextProperty(1)->SetColor(0.0, 1.0, 0.0);
  MeshCubeAxesActor->GetLabelTextProperty(1)->SetColor(0.0, 1.0, 0.0);
  MeshCubeAxesActor->GetLabelTextProperty(1)->SetOrientation(90);

  MeshCubeAxesActor->GetTitleTextProperty(2)->SetColor(0.0, 0.5, 1.0);
  MeshCubeAxesActor->GetLabelTextProperty(2)->SetColor(0.0, 0.5, 1.0);
  MeshCubeAxesActor->GetLabelTextProperty(2)->SetOrientation(90);

  MeshCubeAxesActor->DrawXGridlinesOn();
  MeshCubeAxesActor->DrawYGridlinesOn();
  MeshCubeAxesActor->DrawZGridlinesOn();

  MeshCubeAxesActor->XAxisMinorTickVisibilityOn();
  MeshCubeAxesActor->YAxisMinorTickVisibilityOn();
  MeshCubeAxesActor->ZAxisMinorTickVisibilityOn();
  MeshCubeAxesActor->SetRebuildAxes(true);

  MeshCubeAxesActor->SetGridLineLocation(VTK_GRID_LINES_FURTHEST);

  this->Renderer->AddActor(CubeAxesActor);
  this->MeshRenderer->AddActor(MeshCubeAxesActor);

  this->Internal->UnlinkCameraMesh = MeshRenderer->GetActiveCamera();
  this->Internal->UnlinkCameraModel = Renderer->GetActiveCamera();
  this->Internal->LinkCamera = Renderer->MakeCamera();
  //this->Internal->UnlinkCamera = Renderer->MakeCamera();
  this->Renderer->AddObserver(vtkCommand::EndEvent, this, &cmbNucMainWindow::CameraMovedHandlerMesh);
  this->MeshRenderer->AddObserver(vtkCommand::EndEvent, this, &cmbNucMainWindow::CameraMovedHandlerModel);

  vtkCmbLayeredConeSource *cone = vtkCmbLayeredConeSource::New();
  cone->SetNumberOfLayers(3);
  cone->SetHeight(20.0);
  cone->Update();
  this->Internal->CurrentDataset = cone->GetOutput();
  vtkBoundingBox box;
  computeBounds(this->Internal->CurrentDataset, &box);
  box.GetBounds(this->Internal->BoundsModel);
  setScaledBounds();
  this->Mapper->SetInputDataObject(this->Internal->CurrentDataset);
  cone->Delete();

  vtkCompositeDataDisplayAttributes *attributes = vtkCompositeDataDisplayAttributes::New();
  this->Mapper->SetCompositeDataDisplayAttributes(attributes);
  attributes->Delete();
  attributes = vtkCompositeDataDisplayAttributes::New();
  this->MeshMapper->SetCompositeDataDisplayAttributes(attributes);

  // Set up action signals and slots
  connect(this->ui->actionExit, SIGNAL(triggered()), this, SLOT(onExit()));
  connect(this->ui->actionOpenFile, SIGNAL(triggered()), this, SLOT(onFileOpen()));
#ifndef BUILD_WITH_MOAB
  this->ui->actionOpenMOABFile->setVisible(false);
#else
  connect(this->ui->actionOpenMOABFile, SIGNAL(triggered()), this, SLOT(onFileOpenMoab()));
#endif
  connect(this->ui->actionSaveSelected,   SIGNAL(triggered()), this, SLOT(onSaveSelected()));
  connect(this->ui->actionSaveSelectedAs, SIGNAL(triggered()), this, SLOT(onSaveSelectedAs()));
  connect(this->ui->actionSaveProjectAs,  SIGNAL(triggered()), this, SLOT(onSaveProjectAs()));
  connect(this->ui->actionSaveAll,        SIGNAL(triggered()), this, SLOT(onSaveAll()));

  connect(this->ui->actionView_Axis,      SIGNAL(triggered(bool)), this, SLOT(setAxis(bool)));

  connect(this->ui->actionReloadAll,      SIGNAL(triggered()), this, SLOT(onReloadAll()));
  connect(this->ui->actionReloadSelected, SIGNAL(triggered()), this, SLOT(onReloadSelected()));

  connect( this->ui->action1_6_Symetric_Flat,    SIGNAL(triggered()),
           this,                                 SLOT(onNewCore()) );
  connect( this->ui->action1_6_Symetric_Vertex,  SIGNAL(triggered()),
           this,                                 SLOT(onNewCore()) );
  connect( this->ui->action1_12_Symetric,        SIGNAL(triggered()),
           this,                                 SLOT(onNewCore()) );
  connect( this->ui->actionFullHexagonal,        SIGNAL(triggered()),
           this,                                 SLOT(onNewCore()) );
  connect( this->ui->actionNew_Rectilinear_Core, SIGNAL(triggered()),
           this,                                 SLOT(onNewCore()) );
  this->ui->actionNew_Assembly->setEnabled(false);

  connect(this->ui->actionPreferences, SIGNAL(triggered()),
          this->Preferences, SLOT(setPreferences()));
  connect(this->ui->actionExport, SIGNAL(triggered()), this, SLOT(exportRGG()));
  connect(this->Preferences, SIGNAL(actionParallelProjection(bool)),
          this, SLOT(useParallelProjection(bool)));
  connect(this->ui->actionClearAll, SIGNAL(triggered()), this, SLOT(clearAll()));
  connect(this->ui->actionClear_Mesh, SIGNAL(triggered()), this, SLOT(onClearMesh()));

  //this->centralWidget()->hide();

  // Initial materials and  colors
  this->MaterialColors = new cmbNucMaterialColors();
  QString materialfile =
    QCoreApplication::applicationDirPath() + "/materialcolors.ini";
  this->MaterialColors->OpenFile(materialfile);

  connect(this->MaterialColors, SIGNAL(materialColorChanged()), this, SLOT(colorChange()));

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
  iStyle = meshRenderWindow->GetInteractor()->GetInteractorStyle();
  this->VTKToQt->Connect(iStyle, vtkCommand::StartInteractionEvent,
                         this, SLOT(onInteractionTransition( vtkObject*, unsigned long)));
  this->VTKToQt->Connect(iStyle, vtkCommand::EndInteractionEvent,
                         this, SLOT(onInteractionTransition( vtkObject*, unsigned long)));

  this->ui->toolBar->addWidget(this->ui->viewScaleWidget);
  this->ui->toolBar->addWidget(this->ui->meshControls);

  QTimer::singleShot(0, this, SLOT(ResetView()));
}

void cmbNucMainWindow::CameraMovedHandlerModel()
{
  if(isCameraIsMoving || !this->Internal->CamerasLinked) return;
  isCameraIsMoving = true;
  this->ui->qvtkWidget->GetRenderWindow()->Render();
  isCameraIsMoving = false;
}

void cmbNucMainWindow::CameraMovedHandlerMesh()
{
  if(isCameraIsMoving || !this->Internal->CamerasLinked || !isMeshTabVisible()) return;
  isCameraIsMoving = true;
  this->ui->qvtkMeshWidget->GetRenderWindow()->Render();
  isCameraIsMoving = false;
}

cmbNucMainWindow::~cmbNucMainWindow()
{
  this->ui->qvtkWidget->GetRenderWindow()->RemoveObserver(vtkCommand::AnyEvent);
  this->PropertyWidget->setObject(NULL, NULL);
  this->PropertyWidget->setAssembly(NULL);
  this->InputsWidget->setCore(NULL);
  delete this->NuclearCore;
  delete this->MaterialColors;
#ifdef BUILD_WITH_MOAB
  delete this->Internal->MoabSource;
#endif
  delete this->Internal;
  delete this->ui;
}

void cmbNucMainWindow::initPanels()
{
  if(this->InputsWidget == NULL)
  {
    this->InputsWidget = new cmbNucInputListWidget(this);
    this->InputsWidget->setCreateOptions(this->ui->menuCreate);

    this->ui->InputsDock->setWidget(this->InputsWidget);
    this->ui->InputsDock->setFeatures(QDockWidget::DockWidgetMovable |
                                      QDockWidget::DockWidgetFloatable);

    QObject::connect( this, SIGNAL(checkSave()),
                      this->InputsWidget, SIGNAL(checkSavedAndGenerate()) );
    QObject::connect( this->ui->actionNew_Assembly, SIGNAL(triggered()),
                      this->InputsWidget,           SLOT(onNewAssembly()));
    QObject::connect( this->InputsWidget, SIGNAL(objectSelected(AssyPartObj*, const char*)),
                      this, SLOT(onObjectSelected(AssyPartObj*, const char*)));
    QObject::connect( this->InputsWidget, SIGNAL(objectRemoved()),
                      this, SLOT(onObjectModified()));
    QObject::connect( this->InputsWidget, SIGNAL(deleteCore()),
                      this, SLOT(clearCore()));
  }
  this->InputsWidget->setEnabled(0);
  this->ui->actionExport->setEnabled(false);
  this->InputsWidget->setCore(this->NuclearCore);

#ifdef BUILD_WITH_MOAB
  //if(this->Internal->MoabSource == NULL)
  {
    delete(this->Internal->MoabSource);
    this->Internal->MoabSource = new cmbNucCoregen(this->ui->meshSubcomponent);
    this->Internal->MeshOpen = false;
    QObject::connect(this->ExportDialog, SIGNAL(finished(QString)),
            this->Internal->MoabSource, SLOT(openFile(QString)));
    QObject::connect(this->ExportDialog, SIGNAL(fileFinish()), this, SLOT(checkForNewCUBH5MFiles()));
    QObject::connect(this->ui->drawEdges, SIGNAL(clicked(bool)),
                     this, SLOT(onChangeMeshEdgeMode(bool)));
    QObject::connect(this->Internal->MoabSource, SIGNAL(update()),
                     this, SLOT(onSelectionChange()));
  }
#endif

  if(this->PropertyWidget == NULL)
  {
    this->PropertyWidget = new cmbNucInputPropertiesWidget(this);

    this->ui->PropertyDock->setWidget(this->PropertyWidget);
    this->ui->PropertyDock->setFeatures(QDockWidget::DockWidgetMovable |
                                        QDockWidget::DockWidgetFloatable);
    QObject::connect(this->InputsWidget, SIGNAL(pinsModified(cmbNucAssembly*)),
                     this->PropertyWidget, SLOT(resetAssemblyEditor(cmbNucAssembly*)));
    QObject::connect(this->InputsWidget, SIGNAL(pincellDeleted()),
                     this->PropertyWidget, SLOT(clearPincellEditor()));
    QObject::connect(this->InputsWidget, SIGNAL(assembliesModified(cmbNucCore*)),
                     this->PropertyWidget, SLOT(resetCore(cmbNucCore*)));
    QObject::connect(this->PropertyWidget, SIGNAL(valuesChanged()),
                     this->InputsWidget, SLOT(valueChanged()));
    QObject::connect(this->PropertyWidget, SIGNAL(checkSaveAndGenerate()),
                     this->InputsWidget, SIGNAL(checkSavedAndGenerate()));
    QObject::connect(this->PropertyWidget, SIGNAL(resetView()),
                     this, SLOT(ResetView()));
    QObject::connect(this->PropertyWidget, SIGNAL(sendLabelChange(const QString)),
                     this->InputsWidget, SLOT(labelChanged(QString)));
    QObject::connect(this->PropertyWidget, SIGNAL(objGeometryChanged(AssyPartObj*)),
                     this, SLOT(onObjectGeometryChanged(AssyPartObj*)));
    QObject::connect(this->PropertyWidget, SIGNAL(currentObjectNameChanged(const QString&)),
                     this, SLOT(updatePropertyDockTitle(const QString&)));
    QObject::connect(this->PropertyWidget, SIGNAL(sendLattice(LatticeContainer *)),
                     this->LatticeDraw,    SLOT(setLattice(LatticeContainer *)));
    QObject::connect(this->PropertyWidget, SIGNAL(apply()),
                     this->LatticeDraw,    SLOT(apply()));
    QObject::connect(this->PropertyWidget, SIGNAL(reset()),
                     this->LatticeDraw,    SLOT(reset()));
    QObject::connect(this->PropertyWidget, SIGNAL(sendXSize(int)),
                     this->LatticeDraw,    SLOT(setLatticeXorLayers(int)));
    QObject::connect(this->PropertyWidget, SIGNAL(sendYSize(int)),
                     this->LatticeDraw,    SLOT(setLatticeY(int)));
    QObject::connect(this->LatticeDraw, SIGNAL(valuesChanged()),
                     this->InputsWidget, SLOT(valueChanged()));
    QObject::connect(this->LatticeDraw, SIGNAL(objGeometryChanged(AssyPartObj*)),
                     this, SLOT(onObjectGeometryChanged(AssyPartObj*)));

    QObject::connect(this->PropertyWidget, SIGNAL(sendLatticeFullMode(cmbNucDraw2DLattice::CellDrawMode)),
                     this->LatticeDraw,    SLOT(set_full_mode(cmbNucDraw2DLattice::CellDrawMode)));

    QObject::connect(this->PropertyWidget, SIGNAL(select3DModelView()),
                     this->ui->Dock3D,     SLOT(raise()));
  }
  this->PropertyWidget->setEnabled(0);
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
    this->setCameras(true, this->Internal->IsFullMesh);
    }
  else if( selObj->GetType() == CMBNUC_ASSY_PINCELL ||
          selObj->GetType() == CMBNUC_ASSY_CYLINDER_PIN ||
          selObj->GetType() == CMBNUC_ASSY_FRUSTUM_PIN )
    {
    this->setCameras(false, this->Internal->IsFullMesh);
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
      this->ResetView();
      }
    }
  else if( selObj->GetType() == CMBNUC_ASSY_DUCTCELL)
    {
    this->setCameras(false, this->Internal->IsFullMesh);
    DuctCell* selDuct = dynamic_cast<DuctCell*>(selObj);
    if(selDuct)
    {
      this->ResetView();
    }
    }
  else // assemblies
    {
    this->setCameras(false, this->Internal->IsFullMesh);
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
        this->setCameras(false, this->Internal->IsFullMesh);
        this->Internal->CurrentPinCell = selPin;
        this->updatePinCellMaterialColors(selPin);
        }
      }
    else if(obj->GetType() == CMBNUC_ASSY_DUCTCELL)
    {
      DuctCell * dc = dynamic_cast<DuctCell*>(obj);
      if(dc)
      {
        this->setCameras(false, this->Internal->IsFullMesh);
        this->Internal->CurrentDuctCell = dc;
        this->updateDuctCellMaterialColors(dc);
      }
    }
    else if(assy)
      {
      this->updateAssyMaterialColors(assy);
      }
    //this->Render();
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
  if(checkFilesBeforePreceeding())
  {
    qApp->exit();
  }
}

void cmbNucMainWindow::closeEvent(QCloseEvent *event)
{
  if(checkFilesBeforePreceeding())
  {
    event->accept();
  }
  else
  {
    event->ignore();
  }
}

void cmbNucMainWindow::onNewCore()
{
  //Is saving needed for current?
  if(!checkFilesBeforePreceeding()) return;
  //Get the action that called this.
  QObject * sender = QObject::sender();
  QAction * act = dynamic_cast<QAction*>(sender);
  if(act != NULL)
  {
    QString type = act->text();
    std::string geoType = "HexFlat";;
    enumGeometryType geoTypeEnum = HEXAGONAL;
    int subtype = 1;
    if(type.contains("1/6 Symmetric Flat"))
    {
      subtype = 6;
    }
    else if(type.contains("1/6 Symmetric Vertex"))
    {
      subtype = 6;
      geoType = "HexVertex";
    }
    else if(type.contains("1/12 Symmetric"))
    {
      subtype = 12;
    }
    else if(type.contains("Full"))
    {
      subtype = 1;
    }
    else if(type.contains("Rectilinear"))
    {
      geoTypeEnum = RECTILINEAR;
      geoType = "Rectangular";
    }
    else
    {
      qDebug() << "New action is connected to: " << type << " and that action is not supported.";
      return;
    }
    this->doClearAll(true);
    this->InputsWidget->initMaterialsTree();
    this->NuclearCore->initDefaults();
    this->NuclearCore->setGeometryLabel(geoType);
    this->NuclearCore->setHexSymmetry(subtype);
    this->PropertyWidget->resetCore(this->NuclearCore);
    this->PropertyWidget->setObject(NULL, NULL);
    this->PropertyWidget->setAssembly(NULL);
    this->InputsWidget->onNewAssembly();
    this->NuclearCore->sendDefaults();
    this->ui->actionNew_Assembly->setEnabled(true);
    this->ui->actionExport->setEnabled(true);
    this->Renderer->ResetCamera();
    this->Renderer->Render();
  }
  else
  {
    qDebug() << "Currently only action hook senders are supported.";
  }
}

void cmbNucMainWindow::onFileOpen()
{
  // Use cached value for last used directory if there is one,
  // or default to the user's home dir if not.
  QSettings settings("CMBNuclear", "CMBNuclear");
  QDir dir = settings.value("cache/lastDir", QDir::homePath()).toString();

  QStringList fileNames =
    QFileDialog::getOpenFileNames(this,
                                 "Open File...",
                                 dir.path(),
                                 "INP Files (*.inp)");
  if(fileNames.count()==0)
    {
    return;
    }
  this->setCursor(Qt::BusyCursor);
  // Cache the directory for the next time the dialog is opened
  QFileInfo info(fileNames[0]);
  settings.setValue("cache/lastDir", info.dir().path());
  int numExistingAssy = this->NuclearCore->GetNumberOfAssemblies();
  bool need_to_use_assem = false;

  for(unsigned int i = 0; i < fileNames.count(); ++i)
  {
    inpFileReader freader;
    switch(freader.open(fileNames[i].toStdString()))
    {
      case inpFileReader::ASSEMBLY_TYPE:
      {
        if(!this->InputsWidget->isEnabled() || this->InputsWidget->onlyMeshLoaded())
        {
          doClearAll(true);
        }
        QFileInfo finfo(fileNames[i]);
        std::string label = finfo.completeBaseName().toStdString();
        cmbNucAssembly *assembly = new cmbNucAssembly();
        assembly->label = label;
        freader.read(*assembly);
        if(this->InputsWidget->isEnabled() &&
           assembly->IsHexType() != NuclearCore->IsHexType())
        {
          QMessageBox msgBox;
          msgBox.setText("Not the same type");
          msgBox.setInformativeText(fileNames[i]+" is not the same geometry type as current core.");
          int ret = msgBox.exec();
          delete assembly;
          this->unsetCursor();
          return;
        }
        int acolorIndex = numExistingAssy +
                          this->NuclearCore->GetNumberOfAssemblies() % numAssemblyDefaultColors;

        QColor acolor(defaultAssemblyColors[acolorIndex][0],
                      defaultAssemblyColors[acolorIndex][1],
                      defaultAssemblyColors[acolorIndex][2]);
        assembly->SetLegendColor(acolor);
        bool need_to_calc_defaults = this->NuclearCore->GetNumberOfAssemblies() == 0;
        this->NuclearCore->AddAssembly(assembly);
        if(need_to_calc_defaults) this->NuclearCore->calculateDefaults();
        else assembly->setFromDefaults( this->NuclearCore->GetDefaults() );
        need_to_use_assem = true;
        this->ui->actionNew_Assembly->setEnabled(true);
        this->ui->actionExport->setEnabled(true);
        break;
      }
      case inpFileReader::CORE_TYPE:
        // clear old assembly
        if(!checkFilesBeforePreceeding()) return;
        doClearAll();
        this->PropertyWidget->setObject(NULL, NULL);
        this->PropertyWidget->setAssembly(NULL);
        freader.read(*(this->NuclearCore));
        this->NuclearCore->setAndTestDiffFromFiles(false);
        this->NuclearCore->SetLegendColorToAssemblies(numAssemblyDefaultColors,
                                                      defaultAssemblyColors);
        this->ui->actionNew_Assembly->setEnabled(true);
        this->ui->actionExport->setEnabled(true);
        this->PropertyWidget->resetCore(this->NuclearCore);
        setTitle();
        break;
      default:
        qDebug() << "could not open" << fileNames[i];
    }
  }
  this->InputsWidget->setToModel();

  int numNewAssy = this->NuclearCore->GetNumberOfAssemblies() - numExistingAssy;
  if(numNewAssy && !need_to_use_assem)
  {
    this->PropertyWidget->resetCore(this->NuclearCore);
  }
  else if(numNewAssy)
  {
    enumGeometryType geoType =
        this->NuclearCore->GetAssembly(int(0))->getLattice().GetGeometryType();
    this->PropertyWidget->resetCore(this->NuclearCore);
    switch(geoType)
    {
      case RECTILINEAR:
        NuclearCore->setGeometryLabel("Rectangular");
        break;
      case HEXAGONAL:
        NuclearCore->setGeometryLabel("HexFlat");
        break;
    }
  }
  // update data colors
  this->updateCoreMaterialColors();

  // In case the loaded core adds new materials
  this->InputsWidget->updateUI(numNewAssy);
  this->unsetCursor();

  // update render view
  emit checkSave();
  this->Renderer->ResetCamera();
  this->Renderer->Render();
}

void cmbNucMainWindow::onReloadSelected()
{
  inpFileReader freader;
  //Get the selected core or assembly.
  AssyPartObj* part = InputsWidget->getSelectedCoreOrAssembly();
  if(part == NULL) return;
  //check for type
  switch(part->GetType())
  {
    case CMBNUC_ASSEMBLY:
      {
      cmbNucAssembly* assy = dynamic_cast<cmbNucAssembly*>(part);
      if(assy->FileName.empty()) return;
      freader.open(assy->FileName);
      freader.read(*assy);
      }
      break;
    case CMBNUC_CORE:
      {
      cmbNucCore* core = dynamic_cast<cmbNucCore*>(part);
      if(core->FileName.empty()) return;
      freader.open(core->FileName);
      freader.read(*core, false);
      }
      break;
    default:
      return;
  }
  emit checkSave();
  this->Renderer->ResetCamera();
  this->Renderer->Render();
}

void cmbNucMainWindow::onReloadAll()
{
  for(unsigned int i = 0; i < NuclearCore->GetNumberOfAssemblies();++i)
  {
    inpFileReader freader;
    cmbNucAssembly* assy = NuclearCore->GetAssembly(i);
    if(assy->FileName.empty()) continue;
    freader.open(assy->FileName);
    freader.read(*assy);
  }

  if(!NuclearCore->FileName.empty())
  {
    inpFileReader freader;
    freader.open(NuclearCore->FileName);
    freader.read(*NuclearCore, false);
  }

  emit checkSave();
}

void cmbNucMainWindow::onClearMesh()
{
#ifdef BUILD_WITH_MOAB
  Internal->MoabSource->clear();
  this->MeshMapper->SetInputDataObject(this->Internal->MoabSource->getData());
  this->ui->DockMesh->setVisible(false);
  this->ui->meshSubcomponent->setVisible(false);
  this->ui->drawEdges->setVisible(false);
  this->Internal->MeshOpen = false;
  this->setCameras(this->Internal->IsCoreView, false);
#endif
}

void cmbNucMainWindow::onFileOpenMoab()
{
#ifdef BUILD_WITH_MOAB
  QSettings settings("CMBNuclear", "CMBNuclear");
  QDir dir = settings.value("cache/lastDir", QDir::homePath()).toString();

  QStringList fileNames =
  QFileDialog::getOpenFileNames(this,
                                "Open MOAB File...",
                                dir.path(),
                                "H5M Files (*.h5m)");
  if(fileNames.count()==0)
  {
    return;
  }
  Internal->MoabSource->openFile(fileNames.first());
  this->ui->DockMesh->setVisible(true);
  this->ui->meshControls->setVisible(true);
  this->ui->meshSubcomponent->setVisible(true);
  this->ui->drawEdges->setVisible(true);
  this->Internal->MeshOpen = true;
#endif
}

void cmbNucMainWindow::setCameras(bool coreModel, bool fullMesh)
{
  if( coreModel && fullMesh && !this->Internal->CamerasLinked)
  {
    this->Internal->CamerasLinked = true;
    if(this->Internal->IsFullMesh != fullMesh && coreModel == this->Internal->IsCoreView)
      this->Internal->LinkCamera->DeepCopy(this->Renderer->GetActiveCamera());
    else if(this->Internal->IsFullMesh == fullMesh && coreModel != this->Internal->IsCoreView)
      this->Internal->LinkCamera->DeepCopy(this->MeshRenderer->GetActiveCamera());
    this->Renderer->SetActiveCamera(this->Internal->LinkCamera);
    this->MeshRenderer->SetActiveCamera(this->Internal->LinkCamera);
    setScaledBounds();
    if(this->isMeshTabVisible())
    {
      this->ui->qvtkMeshWidget->update();
    }
  }
  else if( this->Internal->CamerasLinked &&
           (coreModel != this->Internal->IsCoreView ||
            fullMesh != this->Internal->IsFullMesh) )
  {
    this->Internal->CamerasLinked = false;
    this->Internal->UnlinkCameraMesh->DeepCopy(this->MeshRenderer->GetActiveCamera());
    this->Internal->UnlinkCameraModel->DeepCopy(this->Renderer->GetActiveCamera());
    this->Renderer->SetActiveCamera(this->Internal->UnlinkCameraModel);
    this->MeshRenderer->SetActiveCamera(this->Internal->UnlinkCameraMesh);
    if(this->isMeshTabVisible())
    {
      this->ui->qvtkMeshWidget->update();
    }
  }
  this->Internal->IsCoreView = coreModel;
  this->Internal->IsFullMesh = fullMesh;
}

void cmbNucMainWindow::onSaveSelected()
{
  this->saveSelected(false, false);
}

void cmbNucMainWindow::onSaveAll()
{
  this->saveAll(false, false);
}

void cmbNucMainWindow::saveAll(bool requestFileName, bool force_save)
{
  for(unsigned int i = 0; i < NuclearCore->GetNumberOfAssemblies();++i)
    {
    this->save(NuclearCore->GetAssembly(i), requestFileName, force_save);
    }
  this->save(NuclearCore, requestFileName, force_save);
  emit checkSave();
}

void cmbNucMainWindow::onSaveProjectAs()
{
  QDir tdir = QSettings("CMBNuclear", "CMBNuclear").value("cache/lastDir",
                                                          QDir::homePath()).toString();
  QString	dir = QFileDialog::getExistingDirectory( this,
                                                   "Save Project To Single Directory",
                                                   tdir.path() );

  if(dir.isEmpty()) return;
  QSettings("CMBNuclear", "CMBNuclear").setValue("cache/lastDir", dir);
  for(unsigned int i = 0; i < NuclearCore->GetNumberOfAssemblies();++i)
  {
    QString label(NuclearCore->GetAssembly(i)->label.c_str());
    std::string tmpl = label.toLower().toStdString();
    NuclearCore->GetAssembly(i)->FileName = dir.toStdString() +
                                            "/assembly_" + tmpl + ".inp";
  }
  NuclearCore->FileName =dir.toStdString() + "/core.inp";
  this->saveAll(false, true);
  emit checkSave();
}

void cmbNucMainWindow::onSaveSelectedAs()
{
  this->saveSelected(true, true);
}

void cmbNucMainWindow::saveSelected(bool requestFileName, bool force_save)
{
  //Get the selected core or assembly.
  AssyPartObj* part = InputsWidget->getSelectedCoreOrAssembly();
  if(part == NULL) return;
  //check for type
  switch(part->GetType())
  {
    case CMBNUC_ASSEMBLY:
      this->save(dynamic_cast<cmbNucAssembly*>(part), requestFileName, force_save);
      break;
    case CMBNUC_CORE:
      this->save(dynamic_cast<cmbNucCore*>(part), requestFileName, force_save);
      break;
    default:
      return;
  }
  emit checkSave();
}

void cmbNucMainWindow::saveFile(cmbNucAssembly* a)
{
  save(a, false, false);
  emit checkSave();
}

void cmbNucMainWindow::saveFile(cmbNucCore* c)
{
  save(c, false, false);
  emit checkSave();
}

void cmbNucMainWindow::save(cmbNucAssembly* assembly, bool request_file_name, bool force_save)
{
  if(assembly == NULL) return;
  QString fileName = assembly->FileName.c_str();
  if(request_file_name || fileName.isEmpty())
  {
    QString label(assembly->label.c_str());
    std::string tmpl = label.toLower().toStdString();
    QString defaultName = (fileName.isEmpty())?(std::string("assembly_") + tmpl + ".inp").c_str():fileName;
    fileName = cmbNucMainWindow::requestInpFileName(defaultName, "Assembly");
  }
  if(fileName.isEmpty()) return;

  if(force_save || assembly->changeSinceLastSave())
  {
    assembly->WriteFile(fileName.toStdString());
  }
}

void cmbNucMainWindow::save(cmbNucCore* core, bool request_file_name, bool force_save)
{
  if(core == NULL) return;
  QString fileName = core->FileName.c_str();
  if(request_file_name || fileName.isEmpty())
  {
    fileName = cmbNucMainWindow::requestInpFileName("","Core");
  }
  if(fileName.isEmpty()) return;
  if(force_save || core->changeSinceLastSave())
  {
    inpFileWriter::write(fileName.toStdString(), *core);
  }
  setTitle();
}

void cmbNucMainWindow::checkForNewCUBH5MFiles()
{
  for(unsigned int i = 0; i < NuclearCore->GetNumberOfAssemblies();++i)
  {
    NuclearCore->GetAssembly(i)->setAndTestDiffFromFiles(NuclearCore->GetAssembly(i)->changeSinceLastSave());
  }
  NuclearCore->setAndTestDiffFromFiles(NuclearCore->changeSinceLastSave());
  emit checkSave();
}

QString cmbNucMainWindow::requestInpFileName(QString name,
                                             QString type)
{
  QString defaultName("");
  QString defaultLoc;
  if(!name.isEmpty())
  {
    QFileInfo fi(name);
    QDir dir = fi.dir();
    if(dir.path() == ".")
      {
      defaultName = fi.baseName();
      QDir tdir = QSettings("CMBNuclear", "CMBNuclear").value("cache/lastDir",
                                                              QDir::homePath()).toString();
      defaultLoc = tdir.path();
      }
    else
      {
      defaultLoc = dir.path();
      defaultName = fi.baseName();
      }
  }
  if(defaultLoc.isEmpty())
  {
    QDir dir = QSettings("CMBNuclear", "CMBNuclear").value("cache/lastDir",
                                                           QDir::homePath()).toString();
    defaultLoc = dir.path();
  }

  QFileDialog saveQD( this, "Save "+type+" File...", defaultLoc, "INP Files (*.inp)");
  saveQD.setOptions(QFileDialog::DontUseNativeDialog); //There is a bug on the mac were one does not seem to be able to set the default name.
  saveQD.setAcceptMode(QFileDialog::AcceptSave);
  saveQD.selectFile(defaultName);
  QString fileName;
  if(saveQD.exec()== QDialog::Accepted)
  {
    fileName = saveQD.selectedFiles().first();
  }
  else
  {
    return QString();
  }
  if( !fileName.endsWith(".inp") )
    fileName += ".inp";

  if(!fileName.isEmpty())
  {
    // Cache the directory for the next time the dialog is opened
    QFileInfo info(fileName);
    QSettings("CMBNuclear", "CMBNuclear").setValue("cache/lastDir",
                                                   info.dir().path());
  }
  return fileName;
}

void cmbNucMainWindow::saveFile(const QString &fileName)
{
  if(!this->InputsWidget->getCurrentAssembly())
    {
    qDebug() << "no assembly to save";
    return;
    }

  emit checkSave();
  this->InputsWidget->getCurrentAssembly()->WriteFile(fileName.toStdString());
}

void cmbNucMainWindow::saveCoreFile(const QString &fileName)
{
  inpFileWriter::write(fileName.toStdString(), *NuclearCore);
}

void cmbNucMainWindow::clearAll()
{
  if(this->checkFilesBeforePreceeding())
  {
    this->doClearAll();
  }
}

void cmbNucMainWindow::doClearAll(bool needSave)
{
  this->Internal->CurrentDataset = NULL;
  vtkBoundingBox box;
  computeBounds(this->Internal->CurrentDataset, &box);
  box.GetBounds(this->Internal->BoundsModel);
  setScaledBounds();

  this->Mapper->SetInputDataObject(NULL);

  this->LatticeDraw->clear();

  this->PropertyWidget->clear();
  this->InputsWidget->clear();

  delete this->NuclearCore;
  this->NuclearCore = new cmbNucCore(needSave);
  this->InputsWidget->setCore(this->NuclearCore);
  this->ui->actionNew_Assembly->setEnabled(false);
  this->ui->actionExport->setEnabled(false);

  if(this->Internal->MoabSource != NULL) this->onClearMesh();

  this->setCameras(false, false);

  this->MaterialColors->clear();
  QString materialfile =
     QCoreApplication::applicationDirPath() + "/materialcolors.ini";
  this->MaterialColors->OpenFile(materialfile);

  emit checkSave();
  onChangeMeshEdgeMode(false);

  this->ui->qvtkWidget->update();
  this->Renderer->ResetCamera();
  this->Renderer->Render();
}

void cmbNucMainWindow::clearCore()
{
  if(this->NuclearCore)
  {
    this->NuclearCore->clearExceptAssembliesAndGeom();
    this->updateCoreMaterialColors();
    this->PropertyWidget->resetCore(this->NuclearCore);
    this->InputsWidget->updateUI(true);
    emit checkSave();
    this->ui->qvtkWidget->update();
    this->Renderer->ResetCamera();
    this->Renderer->Render();
    setTitle();
  }
}

void cmbNucMainWindow::setScaledBounds()
{
  double * bounds1;
  double * bounds2;
  if(this->Internal->CamerasLinked)
  {
    bounds1 = bounds2 = this->Internal->BoundsMesh;
  }
  else
  {
    bounds1 = this->Internal->BoundsModel;
    bounds2 = this->Internal->BoundsMesh;
  }
  this->CubeAxesActor->SetBounds( bounds1[0], bounds1[1], bounds1[2], bounds1[3],
                                  bounds1[4]*this->ZScale, bounds1[5]*this->ZScale);
  this->CubeAxesActor->SetZAxisRange(bounds1[4], bounds1[5]);
  this->CubeAxesActor->SetCamera(this->Renderer->GetActiveCamera());
  this->MeshCubeAxesActor->SetBounds( bounds2[0], bounds2[1], bounds2[2], bounds2[3],
                                      bounds2[4]*this->ZScale, bounds2[5]*this->ZScale);
  this->MeshCubeAxesActor->SetZAxisRange( bounds2[4], bounds2[5]);
  this->MeshCubeAxesActor->SetCamera(this->MeshRenderer->GetActiveCamera());
}

void cmbNucMainWindow::updatePinCellMaterialColors(PinCell* pin)
{
  if(!pin)
    {
    return;
    }
  this->Internal->CurrentDataset = pin->CachedData;
  vtkBoundingBox box;
  computeBounds(this->Internal->CurrentDataset, &box);
  box.GetBounds(this->Internal->BoundsModel);
  setScaledBounds();

  this->Mapper->SetInputDataObject(this->Internal->CurrentDataset);
  vtkCompositeDataDisplayAttributes *attributes =
    this->Mapper->GetCompositeDataDisplayAttributes();

  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  matColorMap->clearDisplayed();
  unsigned int flat_index = 1; // start from first child
  for(unsigned int idx=0; idx<this->Internal->CurrentDataset->GetNumberOfBlocks(); idx++)
    {
    vtkMultiBlockDataSet* aSection = vtkMultiBlockDataSet::SafeDownCast(
      this->Internal->CurrentDataset->GetBlock(idx));
      {
      flat_index++;
      for(int k = 0; k < pin->GetNumberOfLayers(); k++)
        {
        QPointer<cmbNucMaterial> mat = pin->GetPart(idx)->GetMaterial(k);
        matColorMap->SetBlockMaterialColor(attributes, flat_index++,mat);
        mat->setDisplayed();
        }
      }
      if(pin->cellMaterialSet())
      {
        QPointer<cmbNucMaterial> mat = pin->getCellMaterial();
        matColorMap->SetBlockMaterialColor(attributes, flat_index++,mat);
        mat->setDisplayed();
      }
    }
  cmbNucMaterialColors::instance()->testShow();
}

void cmbNucMainWindow::updateDuctCellMaterialColors(DuctCell* dc)
{
  if(!dc)
  {
    return;
  }
  this->Internal->CurrentDataset = dc->CachedData;
  vtkBoundingBox box;
  computeBounds(this->Internal->CurrentDataset, &box);
  box.GetBounds(this->Internal->BoundsModel);
  setScaledBounds();

  this->Mapper->SetInputDataObject(this->Internal->CurrentDataset);
  vtkCompositeDataDisplayAttributes *attributes =
    this->Mapper->GetCompositeDataDisplayAttributes();

  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  matColorMap->clearDisplayed();
  unsigned int flat_index = 1; // start from first child
  for(unsigned int idx=0; idx<this->Internal->CurrentDataset->GetNumberOfBlocks(); idx++)
  {
    vtkMultiBlockDataSet* aSection = vtkMultiBlockDataSet::SafeDownCast(this->Internal->CurrentDataset->GetBlock(idx));
    Duct* d = dc->getDuct(idx);
    {
      flat_index++;
      for(int k = 0; k < d->NumberOfLayers(); k++)
      {
        QPointer<cmbNucMaterial> mat = d->getMaterial(k);
        matColorMap->SetBlockMaterialColor(attributes, flat_index++,mat);
        mat->setDisplayed();
      }
    }
  }
  cmbNucMaterialColors::instance()->testShow();
}

void cmbNucMainWindow::updateAssyMaterialColors(cmbNucAssembly* assy)
{
  if(!assy)
    {
    return;
    }

  // regenerate core and assembly view
  cmbAssyParameters* params = assy->GetParameters();
  vtkNew<vtkTransform> transform;
  if(assy->IsHexType()) transform->RotateZ(-60);
  this->Internal->CurrentDataset = assy->GetData()->New();
  cmbNucCore::transformData(assy->GetData(),this->Internal->CurrentDataset, transform.GetPointer());
  vtkBoundingBox box;
  computeBounds(this->Internal->CurrentDataset, &box);
  box.GetBounds(this->Internal->BoundsModel);
  setScaledBounds();

  this->Mapper->SetInputDataObject(this->Internal->CurrentDataset);
  if(!this->Internal->CurrentDataset)
    {
    return;
    }
  setScaledBounds();
  vtkCompositeDataDisplayAttributes *attributes =
    this->Mapper->GetCompositeDataDisplayAttributes();

  unsigned int realflatidx = 0;
  cmbNucMaterialColors::instance()->clearDisplayed();
  assy->updateMaterialColors(realflatidx, attributes);
  cmbNucMaterialColors::instance()->testShow();
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
  this->Internal->CurrentDataset = this->NuclearCore->GetData();
  this->Mapper->SetInputDataObject(this->Internal->CurrentDataset);
  vtkBoundingBox box;
  computeBounds(this->Internal->CurrentDataset, &box);
  box.GetBounds(this->Internal->BoundsModel);
  setScaledBounds();
  if(!this->Internal->CurrentDataset)
    {
    return;
    }
  unsigned int numCoreBlocks = this->Internal->CurrentDataset->GetNumberOfBlocks();
  vtkCompositeDataDisplayAttributes *attributes =
    this->Mapper->GetCompositeDataDisplayAttributes();

  cmbNucMaterialColors::instance()->clearDisplayed();

  unsigned int realflatidx = 0;
  for(unsigned int block = 0; block < numCoreBlocks; block++)
    {
    realflatidx++; // for assembly block
    if(!this->Internal->CurrentDataset->GetBlock(block))
      {
      continue;
      }
    vtkMultiBlockDataSet* data = vtkMultiBlockDataSet::SafeDownCast(this->Internal->CurrentDataset->GetBlock(block));
    if(!data)
      {
      continue;
      }
    // for each assembly
    if(this->Internal->CurrentDataset->GetMetaData(block)->Has(vtkCompositeDataSet::NAME()))
      {
      std::string assyLabel = this->Internal->CurrentDataset->GetMetaData(block)->Get(
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
  cmbNucMaterialColors::instance()->testShow();
}

void cmbNucMainWindow::exportRGG()
{
  this->ExportDialog->exportFile(NuclearCore);
}

void cmbNucMainWindow::zScaleChanged(int value)
{
  this->ZScale = 1.0 / value;
  this->Actor->SetScale(1, 1, this->ZScale);
  this->MeshActor->SetScale(1, 1, this->ZScale);
  this->setScaledBounds();
  this->CubeAxesActor->Modified();
  this->Renderer->Modified();
  this->Renderer->ResetCamera();
  this->ui->qvtkWidget->update();

  if(!this->Internal->CamerasLinked && this->isMeshTabVisible())
  {
    this->MeshRenderer->Modified();
    this->MeshRenderer->ResetCamera();
    this->ui->qvtkMeshWidget->update();
  }

  emit updateGlobalZScale(this->ZScale);
}

void cmbNucMainWindow::ResetView()
{
  this->Renderer->ResetCamera();

  this->ui->qvtkWidget->update();
}

void cmbNucMainWindow::colorChange()
{
  this->onChangeMeshColorMode(this->Internal->MoabSource->colorBlocks());
  AssyPartObj* cp = this->InputsWidget->getSelectedPart();
  switch(cp->GetType())
  {
    case CMBNUC_CORE:
      this->updateCoreMaterialColors();
      break;
    case CMBNUC_ASSEMBLY:
      this->updateAssyMaterialColors(dynamic_cast<cmbNucAssembly*>(cp));
      break;
    case CMBNUC_ASSY_DUCTCELL:
      this->updateDuctCellMaterialColors(this->Internal->CurrentDuctCell);
      break;
    case CMBNUC_ASSY_PINCELL:
      this->updatePinCellMaterialColors(this->Internal->CurrentPinCell);
      break;
    default:
      return; //nothing is changed
  }
  this->Mapper->Modified();
  this->ui->qvtkWidget->update();
}

void cmbNucMainWindow::Render()
{
  this->onChangeMeshColorMode(this->Internal->MoabSource->colorBlocks());

  //this->updateCoreMaterialColors();
  this->PropertyWidget->colorChanged();
  this->Mapper->Modified();
  this->Renderer->Render();
  this->ui->qvtkWidget->update();
}

void cmbNucMainWindow::onChangeToModelTab()
{
#ifdef BUILD_WITH_MOAB
  this->onSelectionChange();
  connect(this->Internal->MoabSource, SIGNAL(update()),
          this, SLOT(onSelectionChange()));
#endif
}

void cmbNucMainWindow::onSelectionChange()
{
#ifdef BUILD_WITH_MOAB
  this->onChangeMeshColorMode(this->Internal->MoabSource->colorBlocks());
  this->onChangeMeshEdgeMode(this->ui->drawEdges->isChecked());
  int sel = this->Internal->MoabSource->getSelectedType();
  this->setCameras(this->Internal->IsCoreView, sel == 0 || sel == 3 || sel == 5);
  this->MeshMapper->SetInputDataObject(this->Internal->MoabSource->getData());
  vtkBoundingBox box;
  computeBounds(vtkMultiBlockDataSet::SafeDownCast(this->Internal->MoabSource->getData()), &box);
  box.GetBounds(this->Internal->BoundsMesh);
  setScaledBounds();
#endif
  if( isMeshTabVisible() )
  {
    this->ui->qvtkMeshWidget->update();
  }
}

QString createMaterialLabel(const char * name)
{
  if(name == NULL) return QString();
  QString result(name);
  if(result.endsWith("_top_ss"))
  {
    return result.remove("_top_ss");
  }
  if(result.endsWith("_bot_ss"))
  {
    return result.remove("_bot_ss");
  }
  if(result.endsWith("_side_ss"))
  {
    return result.remove("_side_ss");
  }
  if(result.endsWith("_side1_ss"))
  {
    return result.remove("_side1_ss");
  }
  if(result.endsWith("_side2_ss"))
  {
    return result.remove("_side2_ss");
  }
  if(result.endsWith("_side3_ss"))
  {
    return result.remove("_side3_ss");
  }
  if(result.endsWith("_side4_ss"))
  {
    return result.remove("_side4_ss");
  }
  if(result.endsWith("_side5_ss"))
  {
    return result.remove("_side5_ss");
  }
  if(result.endsWith("_side6_ss"))
  {
    return result.remove("_side6_ss");
  }
  return QString(&(name[2]));

}

void add_color(vtkCompositeDataDisplayAttributes *att,
               unsigned int idx, QColor color, bool visible)
{
  double cd[] = { color.redF(), color.greenF(), color.blueF() };
  att->SetBlockColor(idx, cd);
  att->SetBlockOpacity(idx, color.alphaF());
  att->SetBlockVisibility(idx, visible);
}

void cmbNucMainWindow::onChangeMeshColorMode(bool b)
{
#ifdef BUILD_WITH_MOAB
  if(b)
  {
    vtkSmartPointer<vtkDataObject> data = this->Internal->MoabSource->getData();
    if(data == NULL) return;
    QColor color;
    bool visible;
    vtkCompositeDataDisplayAttributes *att = this->MeshMapper->GetCompositeDataDisplayAttributes();
    if(att == NULL) return;
    att->RemoveBlockVisibilites();
    att->RemoveBlockOpacities();
    att->RemoveBlockColors();
    vtkMultiBlockDataSet* sec = vtkMultiBlockDataSet::SafeDownCast(data);
    switch(this->Internal->MoabSource->getSelectedType())
    {
      case 5: //Material
      case 3:
      {
        int offset = sec->GetNumberOfBlocks()-1;
        for(unsigned int idx=0; idx < sec->GetNumberOfBlocks(); idx++)
        {
          const char * name = sec->GetMetaData((idx+offset)%sec->GetNumberOfBlocks())->Get(vtkCompositeDataSet::NAME());
          //qDebug() << name << createMaterialLabel(name);
          QPointer<cmbNucMaterial> m =
              this->MaterialColors->getMaterialByName(createMaterialLabel(name));
          add_color(att, idx, m->getColor(), m->isVisible());
         }
        }
        break;
      default:
      {
        if(sec!= NULL)
        {
          for(unsigned int idx=0; idx < sec->GetNumberOfBlocks(); idx++)
          {
            unsigned int cind = idx%(numAssemblyDefaultColors-1);
            color = QColor(defaultAssemblyColors[cind][0],
                           defaultAssemblyColors[cind][1],
                           defaultAssemblyColors[cind][2]);
            visible = true;
            add_color(att, idx, color, visible);
          }
        }
        else
        {
          color = QColor(defaultAssemblyColors[0][0],
                         defaultAssemblyColors[0][1],
                         defaultAssemblyColors[0][2]);
          visible = true;
          add_color(att, 0, color, visible);
        }
      }
    }
  }
  else
  {
    vtkCompositeDataDisplayAttributes *attributes = vtkCompositeDataDisplayAttributes::New();
    this->MeshMapper->SetCompositeDataDisplayAttributes(attributes);
    attributes->Delete();
  }
  //this->MeshRenderer->SetActiveCamera(Renderer->GetActiveCamera());
  this->MeshMapper->Modified();
  if( isMeshTabVisible() )
  {
    this->MeshRenderer->Render();
    this->Renderer->Render();
    this->ui->qvtkMeshWidget->update();
    this->ui->qvtkWidget->update();
  }
#endif
}

void cmbNucMainWindow::onChangeMeshEdgeMode(bool b)
{
  vtkProperty* property = this->MeshActor->GetProperty();
  if(b)
  {
    property->SetEdgeVisibility(1);
    property->SetEdgeColor(0,0,0.4);
  }
  else
  {
    property->SetEdgeVisibility(0);
  }
  this->MeshActor->Modified();
  this->MeshMapper->Modified();
  this->MeshRenderer->Modified();
  if( isMeshTabVisible() )
  {
    this->MeshRenderer->Render();
    this->ui->qvtkMeshWidget->update();
  }
}

void cmbNucMainWindow::onInteractionTransition(vtkObject * obj, unsigned long event)
{
  switch (event)
    {
    case vtkCommand::StartInteractionEvent:
      //this->Renderer->UseDepthPeelingOff();
      this->Renderer->SetMaximumNumberOfPeels(1);
      this->MeshRenderer->SetUseDepthPeeling(1);
      break;
    case vtkCommand::EndInteractionEvent:
      //this->Renderer->UseDepthPeelingOn();
      this->Renderer->SetMaximumNumberOfPeels(5);
      this->MeshRenderer->SetUseDepthPeeling(5);
      break;
    }
}

void cmbNucMainWindow::onInteractionMeshTransition(vtkObject *, unsigned long event)
{
  switch (event)
  {
    case vtkCommand::StartInteractionEvent:
      this->MeshRenderer->SetMaximumNumberOfPeels(3);
      break;
    case vtkCommand::EndInteractionEvent:
      this->MeshRenderer->SetMaximumNumberOfPeels(10);
      break;
  }
}

void cmbNucMainWindow::useParallelProjection(bool val)
{
  if (val)
    {
    this->Renderer->GetActiveCamera()->ParallelProjectionOn();
    this->MeshRenderer->GetActiveCamera()->ParallelProjectionOn();
    }
  else
    {
    this->Renderer->GetActiveCamera()->ParallelProjectionOff();
    this->MeshRenderer->GetActiveCamera()->ParallelProjectionOff();
    }
  this->ui->qvtkWidget->update();
  if(isMeshTabVisible()) this->ui->qvtkMeshWidget->update();
}

bool cmbNucMainWindow::isMeshTabVisible()
{
  return this->ui->DockMesh->isVisible() &&
         //It appears either how I am hiding things or a weirdness of
         //docking, is visible is not enough.  As such, if it is tabbed,
         //I make sure it is has a visible region
        (tabifiedDockWidgets(this->ui->DockMesh).isEmpty() ||
         !this->ui->DockMesh->visibleRegion().isEmpty());
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

bool cmbNucMainWindow::checkFilesBeforePreceeding()
{
  if(!this->InputsWidget->isEnabled()) return true;
  bool changed = false;
  changed |= NuclearCore->changeSinceLastSave();

  for(int i = 0; !changed && i < NuclearCore->GetNumberOfAssemblies(); ++i)
  {
    cmbNucAssembly* assy = NuclearCore->GetAssembly(i);
    if(assy && assy->changeSinceLastSave())
    {
      changed = true;
    }
  }
  if(!changed) return true;

  QMessageBox msgBox;
  msgBox.setText("This action will result in lost of changes since last save.");
  msgBox.setInformativeText("Do you want to continue?");
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No );
  msgBox.setDefaultButton(QMessageBox::No);
  int ret = msgBox.exec();
  switch (ret)
  {
    case QMessageBox::No:
      return false;
    default:
      break;
  }
  return true;
}

void cmbNucMainWindow::setTitle()
{
  std::string fname = NuclearCore->getFileName();
  setWindowTitle(("RGG Nuclear Energy Reactor Geometry Generator (" + fname +")").c_str());
}

void cmbNucMainWindow::setAxis(bool ison)
{
  if(ison)
  {
    this->CubeAxesActor->VisibilityOn();
    this->MeshCubeAxesActor->VisibilityOn();
  }
  else
  {
    this->CubeAxesActor->VisibilityOff();
    this->MeshCubeAxesActor->VisibilityOff();
  }
  this->Render();
  if(!this->Internal->CamerasLinked && this->isMeshTabVisible())
  {
    this->MeshRenderer->Modified();
    this->ui->qvtkMeshWidget->update();
  }
}
