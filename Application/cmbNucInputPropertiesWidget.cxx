
#include "cmbNucInputPropertiesWidget.h"
#include "ui_qInputPropertiesWidget.h"

#include "cmbNucAssembly.h"
#include "cmbNucAssemblyLink.h"
#include "cmbNucCore.h"
#include "cmbNucPinCellEditor.h"
#include "cmbNucDuctCellEditor.h"
#include "cmbNucPinCell.h"
#include "cmbNucMaterialColors.h"
#include "cmbNucMainWindow.h"
#include "cmbCoreParametersWidget.h"
#include "cmbAssyParametersWidget.h"
#include "cmbNucDefaultWidget.h"
#include "cmbNucDefaults.h"
#include "cmbNucPinLibrary.h"
#include "cmbNucDuctLibrary.h"

#include <QLabel>
#include <QPointer>
#include <QtDebug>
#include <QColorDialog>
#include <QMessageBox>
#include <QComboBox>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QFileDialog>

#include <cmath>

static const int degreesHex[6] = {-120, -60, 0, 60, 120, 180};
static const int degreesRec[4] = {-90, 0, 90, 180};

extern bool IS_IN_TESTING_MODE;

#define set_and_test_for_change(X, Y)\
   change |= ((Y) != (X)); \
   X = (Y)

class cmbNucInputPropertiesWidgetInternal :
  public Ui::InputPropertiesWidget
{
public:
  QPointer <cmbNucPinCellEditor> PinCellEditor;
  QPointer <cmbNucDuctCellEditor> DuctCellEditor;
  std::string background_full_path;
};

//-----------------------------------------------------------------------------
cmbNucInputPropertiesWidget::cmbNucInputPropertiesWidget(cmbNucMainWindow *mainWindow)
  : QWidget(mainWindow),
    MainWindow(mainWindow)
{
  this->Internal = new cmbNucInputPropertiesWidgetInternal;
  this->Internal->setupUi(this);
  this->Internal->FileName->setVisible(false);
  this->Internal->GenerateControls->setVisible(false);
  this->initUI();
  this->CurrentObject = NULL;
  this->RebuildCoreGrid = false;
  this->Assembly = NULL;
  this->Core = NULL;
}

//-----------------------------------------------------------------------------
cmbNucInputPropertiesWidget::~cmbNucInputPropertiesWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::initUI()
{
  this->CoreProperties = new cmbCoreParametersWidget(this);
  this->Internal->CoreConfLayout->addWidget(this->CoreProperties);
  connect(this->CoreProperties, SIGNAL(valuesChanged()),
          this, SIGNAL(valuesChanged()));

  this->assyConf = new cmbAssyParametersWidget(this);
  this->Internal->AssyConfLayout->addWidget(this->assyConf);
  connect(this->assyConf, SIGNAL(valuesChanged()),
          this, SIGNAL(valuesChanged()));

  //this->Internal->colorSwatch->setFrameStyle(QFrame::Box | QFrame::Plain);
  //this->Internal->assyColorSwatch->setFrameStyle(QFrame::Box | QFrame::Plain);

  QObject::connect(this->Internal->ApplyButton, SIGNAL(clicked()),
    this, SLOT(onApply()));
  QObject::connect(this->Internal->ResetButton, SIGNAL(clicked()),
    this, SLOT(onReset()));

  QObject::connect(this->Internal->ResetButton, SIGNAL(clicked()),
                   this, SIGNAL(reset()));

  QObject::connect(this->Internal->latticeX, SIGNAL(valueChanged(int)),
                   this, SLOT(xSizeChanged(int)));
  QObject::connect(this->Internal->latticeY, SIGNAL(valueChanged(int)),
                   this, SLOT(ySizeChanged(int)));
  QObject::connect(this->Internal->coreLatticeX, SIGNAL(valueChanged(int)),
                   this, SLOT(coreXSizeChanged(int)));
  QObject::connect(this->Internal->coreLatticeY, SIGNAL(valueChanged(int)),
                   this, SLOT(coreYSizeChanged(int)));

  QObject::connect(this->Internal->OuterEdgeInterval, SIGNAL(valueChanged(int)),
                   this, SLOT(onIntervalChanged(int)));
  QObject::connect(this->Internal->RadiusBox, 	SIGNAL(valueChanged(double)),
                   this, SLOT(onRadiusChanged(double)));
  QObject::connect(this->Internal->FindMinCylinder, SIGNAL(clicked()),
                   this, SLOT(onCalculateCylinderDefaults()));
  QObject::connect(this->Internal->JacketMode, SIGNAL(currentIndexChanged(int)),
                   this, SLOT(onDrawCylinder()));
  QObject::connect(this->Internal->JacketMode, SIGNAL(currentIndexChanged(int)),
                   this, SLOT(displayBackgroundControls(int)));
  QObject::connect( this->Internal->BackgroundSetting, SIGNAL(clicked()),
                    this, SLOT(onSetBackgroundMesh()) );
  QObject::connect( this->Internal->BackgroundClear, SIGNAL(clicked()),
                    this, SLOT(onClearBackgroundMesh()));

  QObject::connect(this->Internal->pincellColorButton, SIGNAL(clicked()),
                   this, SLOT(chooseLegendColor()));
  QObject::connect(this->Internal->assyColorButton, SIGNAL(clicked()),
                   this, SLOT(chooseLegendColor()));
  QObject::connect(this->Internal->assyLinkColorButton, SIGNAL(clicked()),
                   this, SLOT(chooseLegendColor()));

  CoreDefaults = new cmbNucDefaultWidget();
  this->Internal->CoreDefaults->addWidget(CoreDefaults);

  connect( CoreDefaults, SIGNAL(commonChanged()),
           this,         SIGNAL(valuesChanged()));

  connect( this->Internal->computePitch, SIGNAL(clicked()), this, SLOT(computePitch()));
  connect( this->Internal->CenterPins, SIGNAL(clicked(bool)), this, SLOT(setAutoPitch(bool)) );
}

//-----------------------------------------------------------------------------
bool cmbNucInputPropertiesWidget::ductCellIsCrossSectioned()
{
  return this->Internal->DuctCellEditor->isCrossSectioned();
}

//-----------------------------------------------------------------------------
bool cmbNucInputPropertiesWidget::pinCellIsCrossSectioned()
{
  return this->Internal->PinCellEditor->isCrossSectioned();
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::setObject(AssyPartObj* selObj, const char* name)
{
  if(this->CurrentObject == selObj)
    {
    return;
    }
  this->CurrentObject = selObj;
  if(!selObj)
    {
    emit currentObjectNameChanged("");
    this->setEnabled(0);
    return;
    }
  this->setEnabled(true);
  if(name != NULL)
    {
    emit currentObjectNameChanged(selObj->getTitle().c_str());
    }
  else
    {
    emit currentObjectNameChanged("");
    }

  this->onReset();
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::setAssembly(cmbNucAssembly *assyObj)
{
  if(this->Assembly == assyObj)
    {
    return;
    }
  this->Assembly = assyObj;
  assyConf->setAssembly(assyObj);
}

// Invoked when Apply button clicked
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::onApply()
{
  if(this->CurrentObject == NULL)
    {
    return;
    }
  AssyPartObj* selObj = this->CurrentObject;
  switch(selObj->GetType())
  {
    case CMBNUC_CORE:
    {
      emit(apply());
      this->applyToCore(this->Core);
      break;
    }
    case CMBNUC_ASSEMBLY:
    {
      emit(apply());
      cmbNucAssembly* assy = dynamic_cast<cmbNucAssembly*>(selObj);
      this->applyToAssembly(assy);
      break;
    }
    case CMBNUC_ASSY_PINCELL:
    {
      PinCell* pincell = dynamic_cast<PinCell*>(selObj);
      this->applyToPinCell(pincell);
      break;
    }
    case CMBNUC_ASSY_FRUSTUM_PIN:
    case CMBNUC_ASSY_CYLINDER_PIN:
      /*handled in the pin editor*/
      break;
    case CMBNUC_ASSY_DUCTCELL:
      this->Internal->DuctCellEditor->Apply();
      break;
    case CMBNUC_ASSEMBLY_LINK:
    {
      cmbNucAssemblyLink * link= dynamic_cast<cmbNucAssemblyLink*>(selObj);
      this->applyToAssemblyLink(link);
      break;
    }
    default:
      this->setEnabled(0);
      break;
  }
}

// Invoked when Reset button clicked
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::onReset()
{
  if(this->CurrentObject == NULL)
    {
    return;
    }
  AssyPartObj* selObj = this->CurrentObject;
  switch(selObj->GetType())
  {
    case CMBNUC_CORE:
    {
      cmbNucCore* nucCore = dynamic_cast<cmbNucCore*>(selObj);
      this->Internal->stackedWidget->setCurrentWidget(this->Internal->pageCore);
      this->CoreDefaults->set(nucCore->GetDefaults(), nucCore->IsHexType());
      this->Internal->coreLabelY->setVisible(!nucCore->IsHexType());
      this->Internal->coreLatticeY->setVisible(!nucCore->IsHexType());
      if(nucCore->IsHexType())
      {
        this->Internal->coreLabelX->setText("Number Of Layers:");
      }
      else
      {
        this->Internal->coreLabelX->setText("X:");
      }
      this->resetCore(nucCore);
      emit(sendLattice(nucCore));
      break;
    }
    case CMBNUC_ASSEMBLY:
    {
      QStringList list;
      cmbNucAssembly* assy = dynamic_cast<cmbNucAssembly*>(selObj);
      QString tmp(assy->getAssyDuct().getName().c_str());
      assy->getDuctLibrary()->fillList(list);
      int i = list.indexOf(tmp);
      this->Internal->Ducts->clear();
      this->Internal->Ducts->addItems(list);
      this->Internal->Ducts->setCurrentIndex(i);
      this->Internal->AssemblyLabelY->setVisible(!assy->IsHexType());
      this->Internal->latticeY->setVisible(!assy->IsHexType());
      this->Internal->pitchY->setVisible(!assy->IsHexType());
      this->Internal->xlabel->setVisible(!assy->IsHexType());
      this->Internal->ylabel->setVisible(!assy->IsHexType());

      if(assy->IsHexType())
      {
        this->Internal->AssemblyLabelX->setText("Number Of Layers:");
      }
      else
      {
        this->Internal->AssemblyLabelX->setText("X:");
      }
      this->Internal->stackedWidget->setCurrentWidget(this->Internal->pageAssembly);
      this->setAssembly(assy);
      this->resetAssembly(assy);
      emit(sendLattice(assy));
      break;
    }
    case CMBNUC_ASSEMBLY_LINK:
    {
      QStringList list;
      cmbNucAssemblyLink* link = dynamic_cast<cmbNucAssemblyLink*>(selObj);
      this->resetAssemblyLink(link);
      this->Internal->stackedWidget->setCurrentWidget(this->Internal->pageLink);
      emit(sendLattice(link->getLink()));
      break;
    }
    case CMBNUC_ASSY_PINCELL:
    {
      PinCell* pincell = dynamic_cast<PinCell*>(selObj);
      this->resetPinCell(pincell);
      this->Internal->stackedWidget->setCurrentWidget(
        this->Internal->pagePinCell);
      this->showPinCellEditor();
      emit(select3DModelView());
      emit(sendLattice(NULL));
      break;
    }
    case CMBNUC_ASSY_DUCTCELL:
      this->Internal->stackedWidget->setCurrentWidget(this->Internal->pageDuctCell);
      this->showDuctCellEditor();
      emit(sendLattice(NULL));
      break;
    case CMBNUC_ASSY_FRUSTUM_PIN:
    case CMBNUC_ASSY_CYLINDER_PIN:
      /*handled in pin editor*/
    default:
      emit(sendLattice(NULL));
      this->setEnabled(0);
      break;
    }
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::clearPincellEditor()
{
  if(this->Internal->PinCellEditor)
  {
    this->Internal->PinCellEditor->clear();
  }
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::clear()
{
  clearPincellEditor();
  this->setObject(NULL,NULL);
  this->Internal->stackedWidget->setCurrentWidget(this->Internal->pageCore);
  this->setAssembly(NULL);
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::pinLabelChanged(PinCell* pincell,
                                                  QString previous,
                                                  QString current)
{
  if(this->Core->getPinLibrary()->labelConflicts(current.toStdString()))
  {
    //ERROR!  Should be unique, revert
    QMessageBox msgBox;
    msgBox.setText(current +
                   QString(" is already use as a pin label, reverting to ")+
                   previous);
    msgBox.exec();
    emit(badPinLabel(previous));
    return;
  }

  this->Core->getPinLibrary()->replaceLabel(previous.toStdString(),
                                            current.toStdString());

  for( int i = 0; i < this->Core->GetNumberOfAssemblies(); ++i )
  {
    cmbNucAssembly * assy = this->Core->GetAssembly(i);
    assy->getLattice().replaceLabel(previous.toStdString(),
                                    current.toStdString());
  }

  emit currentObjectNameChanged( pincell->getTitle().c_str() );
  emit sendLabelChange((pincell->getName() + " (" + pincell->getLabel() + ")").c_str());
}

void cmbNucInputPropertiesWidget::colorChanged()
{
  if(this->CurrentObject == NULL)
  {
    return;
  }
  AssyPartObj* selObj = this->CurrentObject;
  switch(selObj->GetType())
  {
    case CMBNUC_ASSEMBLY_LINK:
      selObj = dynamic_cast<cmbNucAssemblyLink*>(selObj)->getLink();
    case CMBNUC_CORE:
    case CMBNUC_ASSEMBLY:
      emit objGeometryChanged(selObj);
      break;
    case CMBNUC_ASSY_PINCELL:
    case CMBNUC_ASSY_FRUSTUM_PIN:
    case CMBNUC_ASSY_CYLINDER_PIN:
      this->Internal->PinCellEditor->UpdateData();
      break;
    case CMBNUC_ASSY_DUCTCELL:
      this->Internal->DuctCellEditor->update();
      break;
    case CMBNUC_ASSY_LATTICE:
    case CMBNUC_ASSY_BASEOBJ:
    case CMBNUC_ASSY_DUCT:
      this->setEnabled(0);
      break;
  }
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::pinNameChanged(PinCell* pincell,
                                                 QString previous,
                                                 QString current)
{
  if(this->CurrentObject == NULL && this->Assembly)
  {
    return;
  }
  if(this->Core->getPinLibrary()->nameConflicts(current.toStdString()))
  {
    QMessageBox msgBox;
    msgBox.setText(current +
                   QString(" is already use as a pin name, reverting to ")+
                   previous);
    msgBox.exec();
    emit(badPinName(previous));
    return;
  }
  this->Core->getPinLibrary()->replaceName(previous.toStdString(), current.toStdString());
  emit sendLabelChange((pincell->getName() + " (" + pincell->getLabel() + ")").c_str());
}

// reset property panel with given object
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetPinCell(PinCell* pincell)
{
  // Show color swatch with legendColor
  QPalette c_palette = this->Internal->pincellColorButton->palette();
  c_palette.setColor(this->Internal->pincellColorButton->backgroundRole(),
                     pincell->GetLegendColor());
  this->Internal->pincellColorButton->setAutoFillBackground(true);
  this->Internal->pincellColorButton->setPalette(c_palette);
  if(this->Internal->PinCellEditor)
  {
    this->Internal->PinCellEditor->Reset();
  }
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetLattice(Lattice* lattice)
{
  this->Internal->latticeX->blockSignals(true);
  this->Internal->latticeX->setValue(lattice->GetDimensions().first);
  if(lattice->GetGeometryType() == RECTILINEAR)
    {
    this->Internal->latticeY->blockSignals(true);
    this->Internal->latticeY->setValue(lattice->GetDimensions().second);
    this->Internal->latticeY->blockSignals(false);
    }
  this->Internal->latticeX->blockSignals(false);
  this->resetAssemblyLattice();
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetAssemblyEditor(cmbNucAssembly* assy)
{
  this->Assembly = assy;
  this->resetAssemblyLattice();
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetAssemblyLattice()
{
  if(this->Assembly)
    {
    QStringList actionList;
    actionList.append("xx");
    // pincells
    for(size_t i = 0; i < this->Assembly->GetNumberOfPinCells(); i++)
      {
      PinCell *pincell = this->Assembly->GetPinCell(static_cast<int>(i));
      actionList.append(pincell->getLabel().c_str());
      }
    }
}

// apply property panel to given object
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToPinCell(PinCell* pincell)
{
  QPalette c_palette = this->Internal->pincellColorButton->palette();
  QColor c =
      c_palette.color(this->Internal->pincellColorButton->backgroundRole());
  if(c != pincell->GetLegendColor())
  {
    pincell->SetLegendColor(c);
    pincell->GetConnection()->EmitChangeSignal();
    this->resetAssemblyLattice();
  }
  this->Internal->PinCellEditor->Apply();
  emit this->objGeometryChanged(pincell);
  this->Internal->PinCellEditor->UpdateData();
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToLattice(Lattice* lattice)
{
  bool change = false;
  if(change)
  {
    if(this->Assembly->isPinsAutoCentered()) this->Assembly->centerPins();
    emit valuesChanged();
  }
  emit this->objGeometryChanged(lattice);
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToAssembly(cmbNucAssembly* assy)
{
  std::string selected = this->Internal->Ducts->currentText().toStdString();
  DuctCell * dc = assy->getDuctLibrary()->GetDuctCell(selected);
  bool v = assy->setDuctCell(dc);
  if(v)
  {
    this->Core->setAndTestDiffFromFiles(v);
    assy->setAndTestDiffFromFiles(v);
    emit checkSaveAndGenerate();
  }
  std::string new_label = this->Internal->AssyLabel->text().toStdString();
  std::string old_label = assy->getLabel();
  std::replace(new_label.begin(), new_label.end(), ' ', '_');
  if( old_label != new_label)
  {
    if(this->Core->label_unique(new_label))
    {
      //change label
      if(this->Core->getLattice().replaceLabel(old_label,new_label))
      {
        this->Core->setAndTestDiffFromFiles(true);
      }
      assy->setLabel(new_label);
      this->Internal->AssyLabel->setText(new_label.c_str());
      emit currentObjectNameChanged( assy->getTitle().c_str() );
      emit sendLabelChange( QString(new_label.c_str()) );
      emit checkSaveAndGenerate();
    }
    else
    {
      this->Internal->AssyLabel->setText(old_label.c_str());
    }
  }
  QPalette c_palette = this->Internal->assyColorButton->palette();
  QColor c = c_palette.color(this->Internal->assyColorButton->backgroundRole());
  if(c != assy->GetLegendColor())
  {
    assy->SetLegendColor(c);
    this->Core->setAndTestDiffFromFiles(true);
    this->RebuildCoreGrid = true;
  }
  this->assyConf->applyToAssembly(assy);
  double px, py;
  px = this->Internal->pitchX->value();
  py = this->Internal->pitchY->value();
  bool checked = this->Internal->CenterPins->isChecked();
  this->setAutoPitch(checked);
  if(!checked)
  {
    assy->setPitch(px,py);
  }
  else
  {
    assy->calculatePitch(px, py);
    this->Internal->pitchX->setValue(px);
    this->Internal->pitchY->setValue(py);
  }

  int ind = this->Internal->rotationDegree->currentIndex();
  if (assy->IsHexType())
  {
    if(ind >= 0 && ind < 6)
      assy->setZAxisRotation(degreesHex[ind]);
  }
  else
  {
    if(ind >= 0 && ind < 4)
      assy->setZAxisRotation(degreesRec[ind]);
  }

  assy->setCenterPins(checked);
  emit this->objGeometryChanged(assy);
}

void cmbNucInputPropertiesWidget::applyToAssemblyLink(cmbNucAssemblyLink* link)
{
  bool changed = false;
  std::string new_label = this->Internal->AssyLinkLabel->text().toStdString();
  std::string old_label = link->getLabel();
  std::replace(new_label.begin(), new_label.end(), ' ', '_');
  if( old_label != new_label)
  {
    if(this->Core->label_unique(new_label))
    {
      //change label
      if(this->Core->getLattice().replaceLabel(old_label,new_label))
      {
        changed = true;
      }
      link->setLabel(new_label);
      this->Internal->AssyLabel->setText(new_label.c_str());
      emit currentObjectNameChanged( link->getTitle().c_str() );
      emit sendLabelChange( QString(new_label.c_str()) );
    }
    else
    {
      this->Internal->AssyLinkLabel->setText(old_label.c_str());
    }
  }

  QPalette c_palette = this->Internal->assyLinkColorButton->palette();
  QColor c = c_palette.color(this->Internal->assyLinkColorButton->backgroundRole());
  if(c != link->GetLegendColor())
  {
    link->SetLegendColor(c);
    this->RebuildCoreGrid = true;
    changed = true;
  }

  std::string tmp = this->Internal->MaterialSetId->text().toStdString();
  if(tmp != link->getMaterialStartID())
  {
    changed = true;
    link->setMaterialStartID(tmp);
  }

  tmp = this->Internal->NeumannSetId->text().toStdString();
  if(tmp != link->getNeumannStartID())
  {
    changed = true;
    link->setNeumannStartID(tmp);
  }

  if(changed)
  {
    this->Core->setAndTestDiffFromFiles(true);
    checkSaveAndGenerate();
  }
}

void cmbNucInputPropertiesWidget::setAutoPitch(bool v)
{
  if(v)
  {
    double x, y;
    this->Assembly->calculatePitch( this->Internal->latticeX->value(),
                                    this->Internal->latticeY->value(),
                                    x, y );
    this->Internal->pitchX->setValue(x);
    this->Internal->pitchY->setValue(y);
  }
  this->Internal->pitchX->setEnabled( !v );
  this->Internal->pitchY->setEnabled( !v );
  this->Internal->computePitch->setEnabled( !v);
}

void cmbNucInputPropertiesWidget::xSizeChanged(int i)
{
  bool checked = this->Internal->CenterPins->isChecked();
  if(checked)
  {
    double x,y;
    this->Assembly->calculatePitch( i, this->Internal->latticeY->value(),
                                    x, y );
    this->Internal->pitchX->setValue(x);
    this->Internal->pitchY->setValue(y);
  }
  emit sendXSize(i);
}

void cmbNucInputPropertiesWidget::ySizeChanged(int i)
{
  bool checked = this->Internal->CenterPins->isChecked();
  if(checked)
  {
    double x,y;
    this->Assembly->calculatePitch( this->Internal->latticeX->value(),
                                    i,
                                    x, y );
    this->Internal->pitchX->setValue(x);
    this->Internal->pitchY->setValue(y);
  }
  emit sendYSize(i);
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToCore(cmbNucCore* nucCore)
{
  bool changed = false;
  if(this->CoreDefaults->assyPitchChanged())
  {
    changed = true;
    nucCore->setAndTestDiffFromFiles(true);
  }
  this->CoreProperties->applyToCore(nucCore);
  //double thickness[4];
  //nucCore->GetDefaults()->getDuctThickness(thickness[0],thickness[1]);
  if(this->CoreDefaults->apply())
  {
    nucCore->sendDefaults();
    onCalculateCylinderDefaults(true);
  }
  changed |= nucCore->getParams().Background != Internal->Background->text().toStdString();
  nucCore->getParams().Background = Internal->Background->text().toStdString();
  if(nucCore->getParams().BackgroundMode != this->Internal->JacketMode->currentIndex())
  {
    switch(this->Internal->JacketMode->currentIndex())
    {
      case cmbNucCoreParams::None:
        nucCore->getParams().BackgroundMode =cmbNucCoreParams::None;
        break;
      case cmbNucCoreParams::External:
        nucCore->getParams().BackgroundMode =cmbNucCoreParams::External;
        break;
      case cmbNucCoreParams::Generate:
        nucCore->getParams().BackgroundMode =cmbNucCoreParams::Generate;
        break;
      default:
        break;
    }
    changed = true;
  }

  if(nucCore->getParams().BackgroundMode == cmbNucCoreParams::External &&
     nucCore->getParams().BackgroundFullPath != Internal->background_full_path)
  {
    nucCore->getParams().BackgroundFullPath = Internal->background_full_path;
    changed = true;
  }

  if(this->previousRadius != this->currentRadius &&
     nucCore->getParams().BackgroundIsSet() )
  {
    nucCore->setCylinderRadius(this->currentRadius);
    this->previousRadius = this->currentRadius;
    changed = true;
  }

  if(this->previousInterval != this->currentInterval &&
     nucCore->getParams().BackgroundIsSet())
  {
    nucCore->setCylinderOuterSpacing(this->currentInterval);
    this->previousInterval = this->currentInterval;
    changed = true;
  }

  if(Internal->boundaryLayer->isChecked())
  {
    cmbNucCore::boundaryLayer * bl = NULL;
    if(nucCore->getNumberOfBoundaryLayers() != 0)
    {
      bl = nucCore->getBoundaryLayer(0);
    }
    else
    {
      bl = new cmbNucCore::boundaryLayer();
      nucCore->addBoundaryLayer(bl);
      nucCore->boundaryLayerChanged();
    }
    double tmpbias = this->Internal->bias->value();
    double tmpthichness = this->Internal->thickness->value();
    int tmpinterval = this->Internal->interval->value();
    QPointer<cmbNucMaterial> tmpmat =
       cmbNucMaterialColors::instance()->getMaterial(this->Internal->boundaryMaterials);
    if(tmpbias != bl->Bias || tmpthichness != bl->Thickness ||
       tmpinterval != bl->Intervals || tmpmat != bl->interface_material)
    {
      bl->Bias = tmpbias;
      bl->Thickness = tmpthichness;
      bl->Intervals = tmpinterval;
      bl->interface_material = tmpmat;
      nucCore->boundaryLayerChanged();
    }
  }
  else if(nucCore->getNumberOfBoundaryLayers() != 0)
  {
    nucCore->clearBoundaryLayer();
    nucCore->boundaryLayerChanged();
  }

  if(changed)
  {
    emit valuesChanged();
  }
  emit this->objGeometryChanged(nucCore);
  if(this->CoreDefaults->needCameraReset())
  {
    emit resetView();
  }
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetAssembly(cmbNucAssembly* assy)
{
  this->assyConf->resetAssembly(assy);
  this->Internal->CenterPins->setChecked(assy->isPinsAutoCentered());
  this->Internal->AssyLabel->setText(assy->getLabel().c_str());

  int degree = static_cast<int>(assy->getZAxisRotation());

  this->Internal->rotationDegree->clear();
  if (assy->IsHexType())
  {
    for(unsigned int i = 0; i < 6; ++i)
    {
      this->Internal->rotationDegree->addItem(QString::number(degreesHex[i]));
    }
  }
  else
  {
    for(unsigned int i = 0; i < 4; ++i)
    {
      this->Internal->rotationDegree->addItem(QString::number(degreesRec[i]));
    }
  }

  this->Internal->rotationDegree->setCurrentIndex(this->Internal->rotationDegree->findText(QString::number(degree)));

  if(assy->isPinsAutoCentered())
  {
    double px, py;
    assy->calculatePitch(px,py);
    this->Internal->pitchX->setValue(px);
    this->Internal->pitchY->setValue(py);
    this->Internal->pitchX->setEnabled( false );
    this->Internal->pitchY->setEnabled( false );
    this->Internal->computePitch->setEnabled(false);
  }
  else
  {
    this->Internal->pitchX->setValue(assy->getPinPitchX());
    this->Internal->pitchY->setValue(assy->getPinPitchY());
    this->Internal->pitchX->setEnabled( true );
    this->Internal->pitchY->setEnabled( true );
    this->Internal->computePitch->setEnabled(true);
  }

  // Show color swatch with legendColor
  QPushButton* swatch = this->Internal->assyColorButton;

  QPalette::ColorRole role = swatch->backgroundRole();
  //swatch->setBackgroundRole(QPalette::Window);

  QPalette c_palette = swatch->palette();
  c_palette.setColor(swatch->backgroundRole(), assy->GetLegendColor());
  swatch->setAutoFillBackground(true);
  swatch->setPalette(c_palette);
  this->resetLattice(&assy->getLattice());
}

void cmbNucInputPropertiesWidget::resetAssemblyLink(cmbNucAssemblyLink* link)
{
  QStringList list;
  QString tmp(link->getLink()->getLabel().c_str());
  for(int i = 0; i < this->Core->GetNumberOfAssemblies(); ++i)
  {
    list.append(this->Core->GetAssembly(i)->getLabel().c_str());
  }

  QPushButton* swatch = this->Internal->assyLinkColorButton;
  QPalette c_palette = swatch->palette();
  c_palette.setColor(swatch->backgroundRole(), link->GetLegendColor());
  swatch->setAutoFillBackground(true);
  swatch->setPalette(c_palette);

  this->Internal->AssyLinkLabel->setText(link->getLabel().c_str());
  this->Internal->MaterialSetId->setText(link->getMaterialStartID().c_str());
  this->Internal->NeumannSetId->setText(link->getNeumannStartID().c_str());
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetCore(cmbNucCore* nucCore)
{
  if(nucCore)
  {
    this->Core = nucCore;
    this->CoreProperties->setCore(nucCore);
    this->CoreProperties->resetCore(nucCore);
    this->Internal->coreLatticeX->blockSignals(true);
    this->Internal->coreLatticeY->blockSignals(true);
    this->Internal->coreLatticeX->setValue(nucCore->GetDimensions().first);
    this->Internal->coreLatticeY->setValue(nucCore->GetDimensions().second);
    this->Internal->coreLatticeX->blockSignals(false);
    this->Internal->coreLatticeY->blockSignals(false);

    this->Internal->Background->setText(nucCore->getParams().Background.c_str());
    this->Internal->background_full_path =
          nucCore->getParams().BackgroundFullPath;

    this->currentRadius = this->previousRadius = nucCore->getCylinderRadius();
    this->currentInterval = this->previousInterval =
          nucCore->getCylinderOuterSpacing();

    this->Internal->OuterEdgeInterval->setValue(this->previousInterval);
    this->Internal->RadiusBox->setValue(this->previousRadius);

    this->Internal->JacketMode->setCurrentIndex(nucCore->getParams().BackgroundMode);

    if(nucCore->getParams().BackgroundMode == cmbNucCoreParams::Generate)
    {
      onCalculateCylinderDefaults(true);
    }

    QStringList actionList;
    actionList.append("xx");

    if(this->RebuildCoreGrid)
    {
      nucCore->RebuildGrid();
      this->RebuildCoreGrid = false;
    }

    // build the action list
    for(int i = 0; i < nucCore->GetNumberOfAssemblies(); i++)
    {
      cmbNucAssembly *assy = nucCore->GetAssembly(i);
      actionList.append(assy->getLabel().c_str());
    }
    {
      cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
      matColorMap->setUp(this->Internal->boundaryMaterials);
    }
    if(nucCore->getNumberOfBoundaryLayers() != 0)
    {
      Internal->boundaryLayer->setChecked(true);
      cmbNucCore::boundaryLayer * bl = nucCore->getBoundaryLayer(0);
      this->Internal->bias->setValue(bl->Bias);
      this->Internal->thickness->setValue(bl->Thickness);
      this->Internal->interval->setValue(bl->Intervals);
      cmbNucMaterialColors::instance()->selectIndex(this->Internal->boundaryMaterials,
                                                    bl->interface_material);
    }
    else
    {
      Internal->boundaryLayer->setChecked(false);
    }
  }
}

void cmbNucInputPropertiesWidget::showDuctCellEditor()
{
  DuctCell* ductcell = dynamic_cast<DuctCell*>(this->CurrentObject);
  if(!ductcell)
  {
    return;
  }
  if(this->Internal->DuctCellEditor == NULL)
  {
    this->Internal->DuctCellEditor = new cmbNucDuctCellEditor(this);
    this->Internal->ductEditorContainer->addWidget(this->Internal->DuctCellEditor);
    //Setup connections
    QObject::connect(this->Internal->DuctCellEditor,
                     SIGNAL(ductcellModified(AssyPartObj*)),
                     this, SIGNAL(objGeometryChanged(AssyPartObj*)));
    QObject::connect(this->Internal->DuctCellEditor,
                     SIGNAL(nameChanged(DuctCell*, QString, QString)),
                     this, SLOT(ductNameChanged(DuctCell*, QString, QString)));
  }
  this->Internal->DuctCellEditor->SetDuctCell(ductcell, this->Core->IsHexType());
}

void cmbNucInputPropertiesWidget::showPinCellEditor()
{
  // get the current pincell
  PinCell* pincell = dynamic_cast<PinCell*>(this->CurrentObject);
  if(!pincell)
    {
    return;
    }
  if(!this->Internal->PinCellEditor)
    {
    this->Internal->PinCellEditor = new cmbNucPinCellEditor(this);
    this->Internal->pinEditorContainer->addWidget(
      this->Internal->PinCellEditor);
    QObject::connect(this->Internal->PinCellEditor,
                     SIGNAL(pincellModified(AssyPartObj*)),
                     this, SIGNAL(objGeometryChanged(AssyPartObj*)));
    QObject::connect( this->Internal->PinCellEditor,
                      SIGNAL(resetView()),
                      this, SIGNAL(resetView()));
    QObject::connect( this->Internal->PinCellEditor,
                      SIGNAL(labelChanged(PinCell*, QString, QString)),
                      this, SLOT(pinLabelChanged(PinCell*, QString, QString)));
    QObject::connect( this->Internal->PinCellEditor,
                      SIGNAL(nameChanged(PinCell*, QString, QString)),
                      this, SLOT(pinNameChanged(PinCell*, QString, QString)));
    QObject::connect( this,
                      SIGNAL(badPinLabel(QString)),
                      this->Internal->PinCellEditor,
                      SLOT(badLabel(QString)));
    QObject::connect( this,
                      SIGNAL(badPinName(QString)),
                      this->Internal->PinCellEditor,
                      SLOT(badName(QString)));
    QObject::connect( this->Internal->PinCellEditor, SIGNAL(valueChange()),
                      this, SIGNAL(valuesChanged()) );
    }
  this->Internal->PinCellEditor->SetPinCell(pincell, this->Core->IsHexType());
}

void cmbNucInputPropertiesWidget::chooseLegendColor()
{
  QPushButton * swatch = NULL;
  QColor oc = this->CurrentObject->GetLegendColor();
  if(dynamic_cast<PinCell*>(this->CurrentObject) != NULL)
  {
    swatch = this->Internal->pincellColorButton;
  }
  else if(dynamic_cast<cmbNucAssembly*>(this->CurrentObject) != NULL)
  {
    swatch = this->Internal->assyColorButton;
  }
  else if(dynamic_cast<cmbNucAssemblyLink*>(this->CurrentObject) != NULL)
  {
    swatch = this->Internal->assyLinkColorButton;
  }
  if(swatch == NULL) return;
  QColorDialog::ColorDialogOptions options;
  if(IS_IN_TESTING_MODE)
  {
    options = QColorDialog::DontUseNativeDialog;
  }
  QColor selected = QColorDialog::getColor(oc, this,
                                           "Select key color", options);
  if(selected.isValid())
  {
    QPalette c_palette = swatch->palette();
    c_palette.setColor(swatch->backgroundRole(), selected);
    swatch->setPalette(c_palette);
  }
}

void cmbNucInputPropertiesWidget::ductNameChanged(DuctCell* dc,
                                                  QString previous,
                                                  QString current)
{
  if(this->CurrentObject == NULL && this->Assembly)
  {
    return;
  }
  if(this->Core->getDuctLibrary()->nameConflicts(current.toStdString()))
  {
    QMessageBox msgBox;
    msgBox.setText(current +
                   QString(" is already use as a duct name, reverting to ")+
                   previous);
    msgBox.exec();
    dc->setName(previous.toStdString());
    return;
  }
  this->Core->getDuctLibrary()->replaceName(previous.toStdString(),
                                            current.toStdString());
  emit currentObjectNameChanged(current);
  emit sendLabelChange(current);
}

void cmbNucInputPropertiesWidget::computePitch()
{
  double px, py;
  cmbNucAssembly* assy = dynamic_cast<cmbNucAssembly*>(this->CurrentObject);
  assy->calculatePitch( this->Internal->latticeX->value(),
                        this->Internal->latticeY->value(),
                        px, py );
  this->Internal->pitchX->setValue(px);
  this->Internal->pitchY->setValue(py);
}

void cmbNucInputPropertiesWidget::onCalculateCylinderDefaults(bool checkOld)
{
  double initRadius;
  int ei = 0;
  Core->GetDefaults()->getEdgeInterval(ei);
  if(!Core->GetDefaults()->getEdgeInterval(ei)) ei = 10;
  double ductsize[2];
  Core->GetDefaults()->getDuctThickness(ductsize[0],ductsize[1]);
  if(Core->IsHexType())
  {
    double a = ductsize[0] * 0.2;
    double tmp = (this->Internal->coreLatticeX->value()-0.5)*ductsize[0]+a;
    double tmp2 = ((ductsize[0]+a)/std::sqrt(3.0))*0.5;
    double h = std::sqrt(tmp*tmp + tmp2*tmp2);
    initRadius = h;
    this->Internal->OuterEdgeInterval->setValue(this->Internal->coreLatticeX->value()*ei*12);
  }
  else
  {
    double ti = this->Internal->coreLatticeY->value() * ductsize[0];
    double tj = this->Internal->coreLatticeX->value() * ductsize[1];
    double tmpr = std::sqrt(ti*ti+tj*tj);
    initRadius = tmpr*0.5 + std::sqrt( ductsize[0]*ductsize[0]+
                                      ductsize[1]*ductsize[1])*0.5*0.2;
    this->Internal->OuterEdgeInterval->setValue(std::max(this->Internal->coreLatticeX->value(),
                                                         this->Internal->coreLatticeY->value())
                                                *ei*4);
  }

  double r = this->Internal->RadiusBox->value();
  this->Internal->RadiusBox->setMinimum(initRadius);
  this->Internal->RadiusBox->setValue((checkOld) ? (r>initRadius) ? r : initRadius :initRadius);
}

void cmbNucInputPropertiesWidget::onDrawCylinder()
{
  if(this->Internal->JacketMode->currentIndex() == cmbNucCoreParams::Generate)
  {
    emit drawCylinder(this->currentRadius, this->currentInterval);
  }
  else
  {
    emit clearCylinder();
  }
}

void cmbNucInputPropertiesWidget::displayBackgroundControls(int index)
{
  this->Internal->FileName->setVisible(index != cmbNucCoreParams::None);
  this->Internal->GenerateControls->setVisible(index ==
                                               cmbNucCoreParams::Generate);
  if(index == cmbNucCoreParams::Generate)
  {
    onCalculateCylinderDefaults();
  }
  else
  {
    emit clearCylinder();
  }
}

void cmbNucInputPropertiesWidget::onRadiusChanged(double v)
{
  this->currentRadius = v;
  this->onDrawCylinder();
}

void cmbNucInputPropertiesWidget::onIntervalChanged(int v)
{
  this->currentInterval = v;
  this->onDrawCylinder();
}

void cmbNucInputPropertiesWidget::onSetBackgroundMesh()
{
  if(this->Core == NULL)
  {
    return;
  }
  QString fileName;
  if(this->Internal->JacketMode->currentIndex() == cmbNucCoreParams::Generate)
  {
    QString defaultLoc;
    QString name(Core->getExportFileName().c_str());
    if(name.isEmpty()) name = QString(Core->getFileName().c_str());
    if(!name.isEmpty())
    {
      QFileInfo fi(name);
      QDir dir = fi.dir();
      if(dir.path() == ".")
      {
        QDir tdir = QSettings("CMBNuclear",
                              "CMBNuclear").value("cache/lastDir",
                                                  QDir::homePath()).toString();
        defaultLoc = tdir.path();
      }
      else
      {
        defaultLoc = dir.path();
      }
    }
    else
    {
      QDir tdir = QSettings("CMBNuclear",
                            "CMBNuclear").value("cache/lastDir",
                                                QDir::homePath()).toString();
      defaultLoc = tdir.path();
    }

    fileName = QFileDialog::getSaveFileName( this,
                                            "Save Outer Cylinder File...",
                                            defaultLoc, "cub Files (*.cub)" );
  }
  else
  {
    // Use cached value for last used directory if there is one,
    // or default to the user's home dir if not.
    QSettings settings("CMBNuclear", "CMBNuclear");
    QDir dir = settings.value("cache/lastDir", QDir::homePath()).toString();

    QStringList fileNames =
    QFileDialog::getOpenFileNames(this,
                                  "Open File...",
                                  dir.path(),
                                  "cub Files (*.cub)");
    if(fileNames.count()==0)
    {
      return;
    }
    fileName =fileNames[0];
  }
  // Cache the directory for the next time the dialog is opened
  QFileInfo info(fileName);
  this->Internal->background_full_path = info.absoluteFilePath().toStdString();
  Internal->Background->setText(info.fileName());
}

void cmbNucInputPropertiesWidget::onClearBackgroundMesh()
{
  if(this->Core == NULL)
  {
    return;
  }
  this->Internal->background_full_path = "";
  Internal->Background->setText("");
}

void cmbNucInputPropertiesWidget::coreXSizeChanged(int i)
{
  emit sendXSize(i);
  if(this->Internal->JacketMode->currentIndex() == cmbNucCoreParams::Generate)
  {
    this->onCalculateCylinderDefaults(true);
  }
}

void cmbNucInputPropertiesWidget::coreYSizeChanged(int i)
{
  emit sendYSize(i);
  if(this->Internal->JacketMode->currentIndex() == cmbNucCoreParams::Generate)
  {
    this->onCalculateCylinderDefaults(true);
  }
}
