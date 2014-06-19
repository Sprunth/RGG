
#include "cmbNucInputPropertiesWidget.h"
#include "ui_qInputPropertiesWidget.h"

#include "cmbNucAssembly.h"
#include "cmbNucAssemblyEditor.h"
#include "cmbNucCore.h"
#include "cmbNucPinCellEditor.h"
#include "cmbNucPinCell.h"
#include "cmbNucMaterialColors.h"
#include "cmbNucMainWindow.h"
#include "cmbCoreParametersWidget.h"
#include "cmbAssyParametersWidget.h"
#include "cmbNucDefaultWidget.h"
#include "cmbNucDefaults.h"

#include "cmbNucHexLattice.h"

#include <QLabel>
#include <QPointer>
#include <QtDebug>
#include <QColorDialog>
#include <QMessageBox>
#include <QComboBox>

#define set_and_test_for_change(X, Y)\
   change |= ((Y) != (X)); \
   X = (Y)

class cmbNucInputPropertiesWidgetInternal :
  public Ui::InputPropertiesWidget
{
public:
  QPointer <cmbNucPinCellEditor> PinCellEditor;
};

//-----------------------------------------------------------------------------
cmbNucInputPropertiesWidget::cmbNucInputPropertiesWidget(cmbNucMainWindow *mainWindow)
  : QWidget(mainWindow),
    MainWindow(mainWindow)
{
  this->Internal = new cmbNucInputPropertiesWidgetInternal;
  this->Internal->setupUi(this);
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
  this->AssemblyEditor = new cmbNucAssemblyEditor(this, this->Assembly);
  this->Internal->assemblyLatticeContainer->setWidget(this->AssemblyEditor);

  this->CoreEditor = new cmbNucAssemblyEditor(this, NULL);
  this->Internal->coreLatticeContainer->setWidget(this->CoreEditor);

  this->HexCoreProperties = new cmbCoreParametersWidget(this);
  this->Internal->hexCoreConfLayout->addWidget(this->HexCoreProperties);
  connect(this->HexCoreProperties, SIGNAL(valuesChanged()),
          this, SIGNAL(valuesChanged()));
  this->RectCoreProperties = new cmbCoreParametersWidget(this);
  this->Internal->rectCoreConfLayout->addWidget(this->RectCoreProperties);
  connect(this->RectCoreProperties, SIGNAL(valuesChanged()),
          this, SIGNAL(valuesChanged()));

  this->assyConf = new cmbAssyParametersWidget(this);
  this->Internal->AssyConfLayout->addWidget(this->assyConf);
  connect(this->assyConf, SIGNAL(valuesChanged()),
          this, SIGNAL(valuesChanged()));

  //this->CoreProperties->hide();
  //todo::add thins

  this->HexCore = new cmbNucHexLattice(HexLatticeItem::Hexagon, this);
  this->Internal->hexLatticeContainer->addWidget(this->HexCore);

  this->HexAssy = new cmbNucHexLattice(HexLatticeItem::Hexagon, this);
  this->Internal->hexLatticeAssyContainer_2->layout()->addWidget(this->HexAssy);

  this->Internal->colorSwatch->setFrameStyle(QFrame::Box | QFrame::Plain);
  this->Internal->assyColorSwatch->setFrameStyle(QFrame::Box | QFrame::Plain);

  this->Internal->DuctLayers->setColumnCount(3);
  this->Internal->DuctLayers->horizontalHeader()->setStretchLastSection(true);
  this->Internal->DuctLayers->setHorizontalHeaderLabels( QStringList() << "Material"
                                                        << "Normalized Thickness 1"
                                                        << "Normalized Thickness 2");

  QObject::connect(this->Internal->ApplyButton, SIGNAL(clicked()),
    this, SLOT(onApply()));
  QObject::connect(this->Internal->ResetButton, SIGNAL(clicked()),
    this, SLOT(onReset()));

  QObject::connect(this->Internal->latticeX, SIGNAL(valueChanged(int)),
    this, SLOT(onLatticeDimensionChanged()));
  QObject::connect(this->Internal->latticeY, SIGNAL(valueChanged(int)),
    this, SLOT(onLatticeDimensionChanged()));
  QObject::connect(this->Internal->coreLatticeX, SIGNAL(valueChanged(int)),
    this, SLOT(onCoreDimensionChanged()));
  QObject::connect(this->Internal->coreLatticeY, SIGNAL(valueChanged(int)),
    this, SLOT(onCoreDimensionChanged()));
  QObject::connect(this->Internal->hexLattice, SIGNAL(valueChanged(int)),
    this, SLOT(onCoreLayersChanged()));
  QObject::connect(this->Internal->latticeX, SIGNAL(valueChanged(int)),
    this, SLOT(onAssyLayersChanged()));

  // pincell related connections
  QObject::connect(this->Internal->colorSelectButton, SIGNAL(clicked()),
    this, SLOT(choosePinLegendColor()));
  QObject::connect(this->Internal->assyColorSelectButton, SIGNAL(clicked()),
    this, SLOT(chooseAssyLegendColor()));

  // Connect the layer buttons
  QObject::connect(this->Internal->AddDuctMaterialBefore, SIGNAL(clicked()),
                   this, SLOT(addDuctLayerBefore()));
  QObject::connect(this->Internal->AddDuctMaterialAfter, SIGNAL(clicked()),
                    this, SLOT(addDuctLayerAfter()));
  QObject::connect(this->Internal->DeleteDuctMaterial, SIGNAL(clicked()),
                   this, SLOT(deleteDuctLayer()));

  hexCoreDefaults = new cmbNucDefaultWidget();
  this->Internal->hexCoreDefaults->addWidget(hexCoreDefaults);
  rectCoreDefaults = new cmbNucDefaultWidget();
  this->Internal->rectCoreDefaults->addWidget(rectCoreDefaults);
  assyDefaults = new cmbNucDefaultWidget();
  this->Internal->AssyDefaults->addWidget(assyDefaults);
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
  this->AssemblyEditor->setAssembly(assyObj);
  this->CoreEditor->setAssembly(assyObj);
  this->HexAssy->setAssembly(assyObj);
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
  PinCell* pincell = NULL;
  Cylinder* cylin = NULL;
  Frustum* frust = NULL;
  Duct* duct = NULL;
  Lattice* lattice = NULL;
  cmbNucCore* nucCore = NULL;
  cmbNucAssembly* assy = NULL;
  switch(selObj->GetType())
    {
    case CMBNUC_CORE:
      nucCore = dynamic_cast<cmbNucCore*>(selObj);
      this->applyToCore(nucCore);
      break;
    case CMBNUC_ASSEMBLY:
      assy = dynamic_cast<cmbNucAssembly*>(selObj);
      this->applyToAssembly(assy);
      this->applyToLattice(&assy->AssyLattice);
      break;
    case CMBNUC_ASSY_PINCELL:
      pincell = dynamic_cast<PinCell*>(selObj);
      this->applyToPinCell(pincell);
      break;
    case CMBNUC_ASSY_FRUSTUM_PIN:
    case CMBNUC_ASSY_CYLINDER_PIN:
      /*handled in the pin editor*/
      break;
    case CMBNUC_ASSY_DUCT:
      duct = dynamic_cast<Duct*>(selObj);
      this->applyToDuct(duct);
      break;
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
  PinCell* pincell = NULL;
  Cylinder* cylin = NULL;
  Frustum* frust = NULL;
  Duct* duct = NULL;
  Lattice* lattice = NULL;
  cmbNucCore* nucCore = NULL;
  cmbNucAssembly* assy = NULL;
  switch(selObj->GetType())
    {
    case CMBNUC_CORE:
      nucCore = dynamic_cast<cmbNucCore*>(selObj);
      if(nucCore->IsHexType())
        {
          this->hexCoreDefaults->set(nucCore->GetDefaults(), true, true);
          this->Internal->stackedWidget->setCurrentWidget(this->Internal->pageHexCore);
          this->HexCore->setCore(nucCore);
        }
      else
        {
        this->rectCoreDefaults->set(nucCore->GetDefaults(), true, false);
        this->Internal->stackedWidget->setCurrentWidget(this->Internal->pageCore);
        }
      this->resetCore(nucCore);
      break;
    case CMBNUC_ASSEMBLY:
      assy = dynamic_cast<cmbNucAssembly*>(selObj);
      this->Internal->AssemblyLabelY->setVisible(!assy->IsHexType());
      this->Internal->latticeY->setVisible(!assy->IsHexType());
      this->Internal->latticecontainerLayout_2->setVisible(!assy->IsHexType()); //rect
      this->Internal->hexLatticeAssyContainer_2->setVisible(assy->IsHexType());
      this->assyDefaults->set(assy->Defaults, false, assy->IsHexType());
      if(assy->IsHexType())
        {
        this->Internal->AssemblyLabelX->setText("Number Of Layers");
        }
      else
        {
        this->Internal->AssemblyLabelX->setText("X:");
        }
      this->Internal->stackedWidget->setCurrentWidget(this->Internal->pageAssembly);
      this->resetAssembly(assy);
      this->resetLattice(&assy->AssyLattice);
      break;
    case CMBNUC_ASSY_PINCELL:
      pincell = dynamic_cast<PinCell*>(selObj);
      this->resetPinCell(pincell);
      this->Internal->stackedWidget->setCurrentWidget(
        this->Internal->pagePinCell);
      this->showPinCellEditor();
      break;
    case CMBNUC_ASSY_FRUSTUM_PIN:
    case CMBNUC_ASSY_CYLINDER_PIN:
      /*handled in pin editor*/
      break;
    case CMBNUC_ASSY_DUCT:
      this->Internal->stackedWidget->setCurrentWidget(
        this->Internal->pageRectDuct);
      duct = dynamic_cast<Duct*>(selObj);
      this->resetDuct(duct);
      break;
    default:
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
  this->HexCore->clear();
  this->HexAssy->clear();
  this->AssemblyEditor->clearUI(true);
  this->CoreEditor->clearUI(true);
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::pinLabelChanged(PinCell* pincell,
                                                  QString previous,
                                                  QString current)
{
  if(this->CurrentObject == NULL && this->Assembly)
  {
    return;
  }
  //Check to make sure new label is unique
  for(unsigned int i = 0; i < this->Assembly->GetNumberOfPinCells(); ++i)
  {
    PinCell * tpc = this->Assembly->GetPinCell(i);
    if(tpc != NULL && pincell != tpc)
    {
      if(tpc->label == current.toStdString())
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
    }
  }
  AssyPartObj* selObj = this->CurrentObject;
  if(selObj->GetType() == CMBNUC_ASSY_PINCELL)
  {
    this->Assembly->AssyLattice.replaceLabel(previous.toStdString(),
                                             current.toStdString());
  }
  emit currentObjectNameChanged( selObj->getTitle().c_str() );
  emit sendLabelChange(current);
}

void cmbNucInputPropertiesWidget::colorChanged()
{
  if(this->CurrentObject == NULL)
  {
    return;
  }
  AssyPartObj* selObj = this->CurrentObject;
  PinCell* pincell = NULL;
  switch(selObj->GetType())
  {
    case CMBNUC_CORE:
    case CMBNUC_ASSEMBLY:
    case CMBNUC_ASSY_DUCT:
      emit objGeometryChanged(selObj);
      break;
    case CMBNUC_ASSY_PINCELL:
    case CMBNUC_ASSY_FRUSTUM_PIN:
    case CMBNUC_ASSY_CYLINDER_PIN:
      this->Internal->PinCellEditor->UpdateData();
      break;
    case CMBNUC_ASSY_DUCTCELL:
    case CMBNUC_ASSY_LATTICE:
    case CMBNUC_ASSY_BASEOBJ:
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
  //Check to make sure new label is unique
  for(unsigned int i = 0; i < this->Assembly->GetNumberOfPinCells(); ++i)
  {
    PinCell * tpc = this->Assembly->GetPinCell(i);
    if(tpc != NULL && pincell != tpc)
    {
      if(tpc->name == current.toStdString())
      {
        //ERROR!  Should be unique, revert
        QMessageBox msgBox;
        msgBox.setText(current +
                       QString(" is already use as a pin name, reverting to ")+
                       previous);
        msgBox.exec();
        emit(badPinName(previous));
        return;
      }
    }
  }
}

// reset property panel with given object
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetPinCell(PinCell* pincell)
{
  // Show color swatch with legendColor
  QPalette palette = this->Internal->colorSwatch->palette();
  palette.setColor(this->Internal->colorSwatch->backgroundRole(), pincell->GetLegendColor());
  this->Internal->colorSwatch->setPalette(palette);
  if(this->Internal->PinCellEditor)
  {
    this->Internal->PinCellEditor->Reset();
  }
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetDuct(Duct* duct)
{
  this->Internal->DuctXPos->setText(QString::number(duct->x));
  this->Internal->DuctYPos->setText(QString::number(duct->y));
  this->Internal->DuctZPos1->setText(QString::number(duct->z1));
  this->Internal->DuctZPos2->setText(QString::number(duct->z2));

  this->Internal->DuctThickX->setText(QString::number(duct->thickness[0]));
  this->Internal->DuctThickY->setText(QString::number(duct->thickness[1]));

  this->Internal->DuctThickY->setVisible(!Assembly->IsHexType());

  this->setUpDuctTable(Assembly->IsHexType(), duct);
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
      PinCell *pincell = this->Assembly->GetPinCell(i);
      actionList.append(pincell->label.c_str());
      }
    if(this->Assembly->IsHexType())
      {
      this->HexAssy->setActions(actionList);
      this->HexAssy->resetWithGrid(this->Assembly->AssyLattice.Grid,
                                   0);

      }
    else
      {
      this->Assembly->UpdateGrid();
      this->AssemblyEditor->resetUI(this->Assembly->AssyLattice.Grid,
                                    actionList);
      }
    }
}

// apply property panel to given object
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToPinCell(PinCell* pincell)
{
  this->Internal->PinCellEditor->Apply();
  emit this->objGeometryChanged(pincell);
  this->Internal->PinCellEditor->UpdateData();
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToDuct(Duct* duct)
{
  bool change = false;
  double z1 = this->Internal->DuctZPos1->text().toDouble();
  double z2 = this->Internal->DuctZPos2->text().toDouble();
  double length;
  if(!this->getAssembly()->getDefaults()->getHeight(length))
    length = this->getAssembly()->AssyDuct.getLength();
  if(z2>length)
  {
    this->Internal->DuctZPos1->setText(QString::number(duct->z1));
    this->Internal->DuctZPos2->setText(QString::number(duct->z2));
    z1 = duct->z1; z2 = duct->z2;
  }

  set_and_test_for_change(duct->x, this->Internal->DuctXPos->text().toDouble());
  set_and_test_for_change(duct->y, this->Internal->DuctYPos->text().toDouble());
  set_and_test_for_change(duct->z1,z1);
  set_and_test_for_change(duct->z2,z2);

  set_and_test_for_change(duct->thickness[0],
                          this->Internal->DuctThickX->text().toDouble());
  set_and_test_for_change(duct->thickness[1],
                          this->Internal->DuctThickY->text().toDouble());

  change |= this->setDuctValuesFromTable(duct);

  if(change) emit valuesChanged();

  emit this->objGeometryChanged(duct);
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::onLatticeDimensionChanged()
{
  if(this->getAssembly() == NULL || this->getAssembly()->IsHexType()) return;
  this->AssemblyEditor->clearUI(false);
  this->AssemblyEditor->updateLatticeView(this->Internal->latticeX->value(),
                                          this->Internal->latticeY->value());
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::onCoreDimensionChanged()
{
  if(this->Core == NULL || this->Core->IsHexType()) return;
  this->CoreEditor->clearUI(false);
  this->CoreEditor->updateLatticeView(this->Internal->coreLatticeX->value(),
    this->Internal->coreLatticeY->value());
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToLattice(Lattice* lattice)
{
  bool change = false;
  if(lattice->GetGeometryType() == RECTILINEAR)
    {
    change = this->AssemblyEditor->updateLatticeWithGrid(lattice->Grid);
    }
  else if(lattice->GetGeometryType() == HEXAGONAL)
    {
    change = this->HexAssy->applyToGrid(lattice->Grid);
    }
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
  this->assyConf->applyToAssembly(assy);
  this->assyDefaults->apply();
  double px, py;
  assy->Defaults->getPitch(px,py);
  bool checked = this->Internal->CenterPins->isChecked();
  this->assyDefaults->setPitchAvail(!checked);
  if(!checked)
  {
    assy->setPitch(px,py);
  }
  assy->setCenterPins(checked);
  emit this->objGeometryChanged(assy);
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToCore(cmbNucCore* nucCore)
{
  bool changed = false;
  if(nucCore->IsHexType())
    {
    this->HexCoreProperties->applyToCore(nucCore);
    this->hexCoreDefaults->apply();
    changed = this->HexCore->applyToGrid(nucCore->CoreLattice.Grid);
    }
  else
    {
    this->RectCoreProperties->applyToCore(nucCore);
    this->rectCoreDefaults->apply();
    changed = this->CoreEditor->updateLatticeWithGrid(nucCore->CoreLattice.Grid);
    }
  nucCore->sendDefaults();
  if(changed) emit valuesChanged();
  emit this->objGeometryChanged(nucCore);
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetAssembly(cmbNucAssembly* assy)
{
  this->assyConf->resetAssembly(assy);
  this->Internal->CenterPins->setChecked(assy->isPinsAutoCentered());

  // Show color swatch with legendColor
  QLabel* swatch = this->Internal->assyColorSwatch;

  QPalette palette = swatch->palette();
  palette.setColor(swatch->backgroundRole(), assy->GetLegendColor());
  swatch->setPalette(palette);
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetCore(cmbNucCore* nucCore)
{
  if(nucCore)
    {
    this->Core = nucCore;
    this->CoreEditor->setCore(nucCore);
    this->HexCoreProperties->setCore(nucCore);
    this->HexCoreProperties->resetCore(nucCore);
    this->RectCoreProperties->setCore(nucCore);
    this->RectCoreProperties->resetCore(nucCore);
    this->Internal->coreLatticeX->blockSignals(true);
    this->Internal->coreLatticeY->blockSignals(true);
    this->Internal->coreLatticeX->setValue(nucCore->GetDimensions().first);
    this->Internal->coreLatticeY->setValue(nucCore->GetDimensions().second);
    this->Internal->coreLatticeX->blockSignals(false);
    this->Internal->coreLatticeY->blockSignals(false);

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
      actionList.append(assy->label.c_str());
      }
    if(nucCore->IsHexType())
      {
      this->Internal->hexLattice->blockSignals(true);
      this->Internal->hexLattice->setValue(nucCore->GetDimensions().first);
      this->Internal->hexLattice->blockSignals(false);
      this->HexCore->setActions(actionList);
      this->HexCore->resetWithGrid(nucCore->CoreLattice.Grid,
                                   nucCore->CoreLattice.subType);
      }
    else
      {
      this->CoreEditor->resetUI(nucCore->CoreLattice.Grid, actionList);
      }
    }
}

void cmbNucInputPropertiesWidget::showPinCellEditor()
{
  // get the current pincell
  PinCell* pincell = dynamic_cast<PinCell*>(this->CurrentObject);
  if(!pincell)
    {
//    std::cerr << "Error: don't have pincell" << std::endl;
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
  this->Internal->PinCellEditor->SetPinCell(pincell, this->getAssembly()->IsHexType());
  this->Internal->PinCellEditor->SetAssembly(this->getAssembly());
}

void cmbNucInputPropertiesWidget::choosePinLegendColor()
{
  PinCell* pincell = dynamic_cast<PinCell*>(this->CurrentObject);
  if(!pincell)
    {
    std::cerr << "Error: don't have pincell" << std::endl;
    return;
    }
  QColor selected = QColorDialog::getColor(pincell->GetLegendColor(), this,
    "Select key color for pin cell type");
  if(selected.isValid())
    {
    pincell->SetLegendColor(selected);
    QPalette palette = this->Internal->colorSwatch->palette();
    palette.setColor(this->Internal->colorSwatch->backgroundRole(), selected);
    this->Internal->colorSwatch->setPalette(palette);

    this->resetAssemblyLattice();
    }
}

void cmbNucInputPropertiesWidget::chooseAssyLegendColor()
{
  cmbNucAssembly* assy = dynamic_cast<cmbNucAssembly*>(this->CurrentObject);
  if(!assy)
    {
    std::cerr << "Error: don't have assy" << std::endl;
    return;
    }
  QColor selected = QColorDialog::getColor(assy->GetLegendColor(), this,
    "Select key color for assembly type");
  if(selected.isValid())
    {
    assy->SetLegendColor(selected);
    QPalette palette = this->Internal->assyColorSwatch->palette();
    palette.setColor(this->Internal->assyColorSwatch->backgroundRole(), selected);
    this->Internal->assyColorSwatch->setPalette(palette);

    // We set this flag to denote that the grid should be rebuilt the
    // next time we select the core
    this->RebuildCoreGrid = true;
    }
}

void cmbNucInputPropertiesWidget::onCoreLayersChanged()
{
  this->HexCore->setLayers(this->Internal->hexLattice->value());
}

void cmbNucInputPropertiesWidget::onAssyLayersChanged()
{
  if(this->getAssembly() == NULL || !this->getAssembly()->IsHexType()) return;
  this->HexAssy->setLayers(this->Internal->latticeX->value());
}

// We use this class to validate the input to the radius fields for layers
class DuctLayerThicknessEditor : public QTableWidgetItem
{
public:
  virtual void setData(int role, const QVariant& value)
  {
    if (this->tableWidget() != NULL && role == Qt::EditRole)
    {
      bool ok;
      double dval = value.toDouble(&ok);

      // Make sure value is in [0, 1]
      if (!ok || dval < 0. || dval > 1.)
      {
        return;
      }
      // Make sure value is greater than previous row
      if (this->row() > 0)
      {
        double prev = this->tableWidget()->item(this->row() - 1, 1)
        ->data(Qt::DisplayRole).toDouble();
        if (dval <= prev)
        {
          return;
        }
      }
      // Make sure value is less than next row
      if (this->row() < this->tableWidget()->rowCount() - 1)
      {
        double next = this->tableWidget()->item(this->row() + 1, 1)
        ->data(Qt::DisplayRole).toDouble();
        if (dval >= next)
        {
          return;
        }
      }
      if(this->row() == this->tableWidget()->rowCount() - 1 && dval != 1.0)
      {
        return;
      }
    }
    QTableWidgetItem::setData(role, value);
  }
};

void cmbNucInputPropertiesWidget::setUpDuctTable(bool isHex, Duct* duct)
{
  QTableWidget * tmpTable = this->Internal->DuctLayers;
  tmpTable->clear();
  tmpTable->setRowCount(0);
  tmpTable->setColumnCount(3);
  tmpTable->setColumnHidden(2, isHex);
  if(isHex)
  {
    tmpTable->setHorizontalHeaderLabels( QStringList() << "Material"
                                         << "Normalized\nThickness");
  }
  else
  {
    tmpTable->setHorizontalHeaderLabels( QStringList() << "Material"
                                         << "Normalized\nThickness 1"
                                         << "Normalized\nThickness 2");
  }
  tmpTable->horizontalHeader()->setStretchLastSection(true);
  if(duct == NULL) return;
  tmpTable->blockSignals(true);
  tmpTable->setRowCount(duct->NumberOfLayers());

  for(size_t i = 0; i < duct->NumberOfLayers(); i++)
  {
    // Private helper method to create the UI for a duct layer
    QComboBox* comboBox = new QComboBox;
    size_t row = i;
    double* thick = duct->getNormThick(i);

    cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
    matColorMap->setUp(comboBox);
    matColorMap->selectIndex(comboBox, duct->getMaterial(i));

    tmpTable->setCellWidget(row, 0, comboBox);

    QTableWidgetItem* thick1Item = new DuctLayerThicknessEditor;
    QTableWidgetItem* thick2Item = new DuctLayerThicknessEditor;

    thick1Item->setText(QString::number(thick[0]));
    thick2Item->setText(QString::number(thick[1]));

    tmpTable->setItem(row, 1, thick1Item);
    tmpTable->setItem(row, 2, thick2Item);
  }
  tmpTable->resizeColumnsToContents();
  tmpTable->blockSignals(false);
}

bool cmbNucInputPropertiesWidget::setDuctValuesFromTable(Duct* duct)
{
  if(duct == NULL) return false;
  QTableWidget * table = this->Internal->DuctLayers;
  bool change = duct->NumberOfLayers() != table->rowCount();
  duct->SetNumberOfLayers(table->rowCount());
  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  for(int i = 0; i < table->rowCount(); i++)
  {
    double* nthick = duct->getNormThick(i);
    QPointer<cmbNucMaterial> mat =
      matColorMap->getMaterial(qobject_cast<QComboBox *>(table->cellWidget(i, 0)));
    duct->setMaterial(i, mat);
    set_and_test_for_change(nthick[0],
                            table->item(i, 1)->data(Qt::DisplayRole).toDouble());
    set_and_test_for_change(nthick[1],
                            table->item(i, 2)->data(Qt::DisplayRole).toDouble());
  }
  return change;
}

void cmbNucInputPropertiesWidget::addDuctLayerBefore()
{
  QTableWidget * table = this->Internal->DuctLayers;
  table->blockSignals(true);
  int row = 0;
  if(table->selectedItems().count() > 0)
  {
    row = table->selectedItems().value(0)->row();
  }

  table->insertRow(row);
  this->addDuctLayerSetRowValue(row);
  table->blockSignals(false);
}

void cmbNucInputPropertiesWidget::addDuctLayerAfter()
{
  QTableWidget * table = this->Internal->DuctLayers;
  table->blockSignals(true);
  int row = table->rowCount();
  if(table->selectedItems().count() > 0)
  {
    row = table->selectedItems().value(0)->row() + 1;
  }

  table->insertRow(row);
  this->addDuctLayerSetRowValue(row);
  table->blockSignals(false);
}

void cmbNucInputPropertiesWidget::deleteDuctLayer()
{
  QTableWidget * table = this->Internal->DuctLayers;
  table->blockSignals(true);
  if(table->selectedItems().count() == 0)
  {
    return;
  }
  QTableWidgetItem* selItem = table->selectedItems().value(0);
  int row = selItem->row();
  table->removeRow(row);
  if( row != 0 &&
      row == table->rowCount() )
  {
    table->item(row-1,1)->setText("1.0");
    table->item(row-1,2)->setText("1.0");
  }
  table->blockSignals(false);
}

void cmbNucInputPropertiesWidget::addDuctLayerSetRowValue(int row)
{
  QTableWidget * table = this->Internal->DuctLayers;
  // Private helper method to create the UI for a duct layer
  QComboBox* comboBox = new QComboBox;
  double thickness[] = {0,0};
  if(row == table->rowCount()-1) // end
  {
    thickness[0] = 1.0;
    thickness[1] = 1.0;
    if(row > 0)
    {
      QTableWidgetItem  * before1 = table->item(row-1, 1);
      QTableWidgetItem  * before2 = table->item(row-1, 2);
      double tmp[] = {0,0};
      if(row > 1)
      {
        tmp[0] = table->item(row-2, 1)->data(Qt::DisplayRole).toDouble();
        tmp[1] = table->item(row-2, 2)->data(Qt::DisplayRole).toDouble();
      }
      before1->setText(QString::number(tmp[0] + (1-tmp[0])*0.5));
      before2->setText(QString::number(tmp[1] + (1-tmp[1])*0.5));
    }
  }
  else
  {
    double tmpB[] = {0,0};
    double tmpA[] = {table->item(row+1, 1)->data(Qt::DisplayRole).toDouble(),
                     table->item(row+1, 2)->data(Qt::DisplayRole).toDouble()};
    if(row > 0)
    {
      tmpB[0] = table->item(row-1, 1)->data(Qt::DisplayRole).toDouble();
      tmpB[1] = table->item(row-1, 2)->data(Qt::DisplayRole).toDouble();
    }
    thickness[0] = tmpB[0] + (tmpA[0]-tmpB[0])*0.5;
    thickness[1] = tmpB[1] + (tmpA[1]-tmpB[1])*0.5;
  }


  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  matColorMap->setUp(comboBox);
  table->setCellWidget(row, 0, comboBox);

  QTableWidgetItem* thick1Item = new DuctLayerThicknessEditor;
  QTableWidgetItem* thick2Item = new DuctLayerThicknessEditor;

  thick1Item->setText(QString::number(thickness[0]));
  thick2Item->setText(QString::number(thickness[1]));

  table->setItem(row, 1, thick1Item);
  table->setItem(row, 2, thick2Item);
}
