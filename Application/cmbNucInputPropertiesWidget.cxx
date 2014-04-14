
#include "cmbNucInputPropertiesWidget.h"
#include "ui_qInputPropertiesWidget.h"

#include "cmbNucAssembly.h"
#include "cmbNucAssemblyEditor.h"
#include "cmbNucCore.h"
#include "cmbNucPinCellEditor.h"
#include "cmbNucMaterialColors.h"
#include "cmbNucMainWindow.h"
#include "cmbCoreParametersWidget.h"
#include "cmbAssyParametersWidget.h"

#include "cmbNucHexLattice.h"

#include <QLabel>
#include <QPointer>
#include <QtDebug>
#include <QColorDialog>
#include <QMessageBox>

class cmbNucInputPropertiesWidgetInternal :
  public Ui::InputPropertiesWidget
{
public:

  QStringList DuctMaterials; // size is the number of layers
  QList<QPair<double, double> > DuctThicknesses; //size is twice the number of layers
  QPointer <cmbNucPinCellEditor> PinCellEditor;
};

//-----------------------------------------------------------------------------
cmbNucInputPropertiesWidget::cmbNucInputPropertiesWidget(cmbNucMainWindow *mainWindow)
  : QWidget(mainWindow),
    MainWindow(mainWindow)
{
  this->Internal = new cmbNucInputPropertiesWidgetInternal;
  this->GeometryType = RECTILINEAR;
  this->Internal->setupUi(this);
  this->initUI();
  this->CurrentObject = NULL;
  this->RebuildCoreGrid = false;
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
  this->RectCoreProperties = new cmbCoreParametersWidget(this);
  this->Internal->rectCoreConfLayout->addWidget(this->RectCoreProperties);

  this->HexAssyConf = new cmbAssyParametersWidget(this);
  this->Internal->HexAssyConfLayout->addWidget(this->HexAssyConf);
  this->RectAssyConf = new cmbAssyParametersWidget(this);
  this->Internal->RectAssyConfLayout->addWidget(this->RectAssyConf);

  //this->CoreProperties->hide();
  //todo::add thins

  this->HexCore = new cmbNucHexLattice(HexLatticeItem::Hexagon, this);
  this->Internal->hexLatticeContainer->addWidget(this->HexCore);

  this->HexAssy = new cmbNucHexLattice(HexLatticeItem::Hexagon, this);
  this->Internal->hexLatticeAssyContainer->addWidget(this->HexAssy);

  this->Internal->colorSwatch->setFrameStyle(QFrame::Box | QFrame::Plain);
  this->Internal->assyColorSwatch->setFrameStyle(QFrame::Box | QFrame::Plain);
  this->Internal->hexAssyColorSwatch->setFrameStyle(QFrame::Box | QFrame::Plain);

  this->Internal->DuctLayers->setColumnCount(3);
  this->Internal->DuctLayers->horizontalHeader()->setStretchLastSection(true);
  this->Internal->DuctLayers->setHorizontalHeaderLabels( QStringList() << "Material"
                                                        << "Normalized Thickness 1"
                                                        << "Normalized Thickness 2");

  QObject::connect(this->Internal->ApplyButton, SIGNAL(clicked()),
    this, SLOT(onApply()));
  QObject::connect(this->Internal->ResetButton, SIGNAL(clicked()),
    this, SLOT(onReset()));

  // duct related connections
  //QObject::connect(this->Internal->NumOfDuctLayers, SIGNAL(valueChanged(int)),
  //  this, SLOT(onNumberOfDuctLayersChanged(int)));
  //QObject::connect(this->Internal->DuctLayer, SIGNAL(currentIndexChanged(int)),
  //  this, SLOT(onCurrentDuctLayerChanged(int)));

  //QObject::connect(this->Internal->DuctLayerMaterial, SIGNAL(currentIndexChanged(int)),
  //  this, SLOT(onCurrentDuctMaterialChanged()));
  //QObject::connect(this->Internal->DuctThick1, SIGNAL(editingFinished()),
  //  this, SLOT(onDuctThicknessChanged()));
  //QObject::connect(this->Internal->DuctThick2, SIGNAL(editingFinished()),
  //  this, SLOT(onDuctThicknessChanged()));
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
  QObject::connect(this->Internal->hexLatticeAssy, SIGNAL(valueChanged(int)),
    this, SLOT(onAssyLayersChanged()));

  // pincell related connections
  QObject::connect(this->Internal->colorSelectButton, SIGNAL(clicked()),
    this, SLOT(choosePinLegendColor()));
  QObject::connect(this->Internal->assyColorSelectButton, SIGNAL(clicked()),
    this, SLOT(chooseAssyLegendColor()));
  QObject::connect(this->Internal->hexAssyColorSelectButton, SIGNAL(clicked()),
    this, SLOT(chooseHexAssyLegendColor()));

  // Connect the layer buttons
  QObject::connect(this->Internal->AddDuctMaterialBefore, SIGNAL(clicked()),
                   this, SLOT(addDuctLayerBefore()));
  QObject::connect(this->Internal->AddDuctMaterialAfter, SIGNAL(clicked()),
                    this, SLOT(addDuctLayerAfter()));
  QObject::connect(this->Internal->DeleteDuctMaterial, SIGNAL(clicked()),
                   this, SLOT(deleteDuctLayer()));
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::setGeometryType(enumGeometryType g)
{
  this->GeometryType = g;
}

//-----------------------------------------------------------------------------
enumGeometryType cmbNucInputPropertiesWidget::getGeometryType()
{
  return this->GeometryType;
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
  emit currentObjectNameChanged(name);

  this->onReset();
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::updateMaterials()
{
  // update materials
  //this->Internal->DuctLayerMaterial->blockSignals(true);
  this->Internal->FrustumMaterial->blockSignals(true);
  this->Internal->CylinderMaterial->blockSignals(true);

  //this->Internal->DuctLayerMaterial->clear();
  this->Internal->FrustumMaterial->clear();
  this->Internal->CylinderMaterial->clear();

  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  QString matLabel;
  foreach(QString material, matColorMap->MaterialColorMap().keys())
    {
//    matLabel = matColorMap->MaterialColorMap()[material].Label;
    matLabel = material;
    //this->Internal->DuctLayerMaterial->addItem(matLabel);
    this->Internal->FrustumMaterial->addItem(matLabel);
    this->Internal->CylinderMaterial->addItem(matLabel);
    }

  //this->Internal->DuctLayerMaterial->blockSignals(false);
  this->Internal->FrustumMaterial->blockSignals(false);
  this->Internal->CylinderMaterial->blockSignals(false);
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
  HexAssyConf->setAssembly(assyObj);
  RectAssyConf->setAssembly(assyObj);
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
      frust = dynamic_cast<Frustum*>(selObj);
      this->applyToFrustum(frust);
      break;
    case CMBNUC_ASSY_CYLINDER_PIN:
      cylin = dynamic_cast<Cylinder*>(selObj);
      this->applyToCylinder(cylin);
      break;
    case CMBNUC_ASSY_RECT_DUCT:
    case CMBNUC_ASSY_HEX_DUCT:
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
      if(this->GeometryType == RECTILINEAR)
        {
        this->Internal->stackedWidget->setCurrentWidget(
          this->Internal->pageCore);
        }
      else if(this->GeometryType == HEXAGONAL)
        {
        this->Internal->stackedWidget->setCurrentWidget(
          this->Internal->pageHexCore);
        this->HexCore->setCore(nucCore);
        }
      this->resetCore(nucCore);
      break;
    case CMBNUC_ASSEMBLY:
      assy = dynamic_cast<cmbNucAssembly*>(selObj);
      if(this->GeometryType == RECTILINEAR)
        {
        this->Internal->stackedWidget->setCurrentWidget(
          this->Internal->pageAssembly);
        }
      else if(this->GeometryType == HEXAGONAL)
        {
        this->Internal->stackedWidget->setCurrentWidget(
          this->Internal->pageHexAssy);
        }
      this->resetAssembly(assy);
      this->resetLattice(&assy->AssyLattice);
      break;
    case CMBNUC_ASSY_PINCELL:
      pincell = dynamic_cast<PinCell*>(selObj);
      this->resetPinCell(pincell);
      this->Internal->stackedWidget->setCurrentWidget(
        this->Internal->pagePinCell);
      this->showPinCellEditor();
      // if the pincell is empty bring up the pin cell editor
      // if (pincell->NumberOfSections() == 0)
      //  {this->showPinCellEditor();}

      break;
    case CMBNUC_ASSY_FRUSTUM_PIN:
      this->Internal->stackedWidget->setCurrentWidget(
        this->Internal->pageFrustumPin);
      frust = dynamic_cast<Frustum*>(selObj);
      this->resetFrustum(frust);
      break;
    case CMBNUC_ASSY_CYLINDER_PIN:
      this->Internal->stackedWidget->setCurrentWidget(
        this->Internal->pageCylinderPin);
      cylin = dynamic_cast<Cylinder*>(selObj);
      this->resetCylinder(cylin);
      break;
    case CMBNUC_ASSY_RECT_DUCT:
    case CMBNUC_ASSY_HEX_DUCT:
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
void cmbNucInputPropertiesWidget::pinLabelChanged(PinCell* pincell,
                                                  QString previous,
                                                  QString current)
{
  if(this->CurrentObject == NULL && this->Assembly)
  {
    return;
  }
  //Check to make sure new label is unique
  for(unsigned int i = 0; i < this->Assembly->PinCells.size(); ++i)
  {
    PinCell * tpc = this->Assembly->PinCells[i];
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
  emit currentObjectNameChanged(current);
  emit sendLabelChange(current);
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
  for(unsigned int i = 0; i < this->Assembly->PinCells.size(); ++i)
  {
    PinCell * tpc = this->Assembly->PinCells[i];
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
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetFrustum(Frustum* frust)
{
  this->Internal->FrustumXPos->setText(QString::number(frust->x));
  this->Internal->FrustumYPos->setText(QString::number(frust->y));
  this->Internal->FrustumZPos1->setText(QString::number(frust->z1));
  this->Internal->FrustumZPos2->setText(QString::number(frust->z2));

  this->Internal->FrustumRadius1->setText(QString::number(frust->r1));
  this->Internal->FrustumRadius2->setText(QString::number(frust->r2));

  this->Internal->FrustumMaterial->setVisible(false); // should be a table for layers
  this->Internal->labelFruMaterial->setVisible(false);

  //this->Internal->FrustumMaterial->setCurrentIndex(
  //  this->Internal->FrustumMaterial->findText(frust->material.c_str()));
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetCylinder(Cylinder* cylin)
{
  this->Internal->CylinderXPos->setText(QString::number(cylin->x));
  this->Internal->CylinderYPos->setText(QString::number(cylin->y));
  this->Internal->CylinderZPos1->setText(QString::number(cylin->z1));
  this->Internal->CylinderZPos2->setText(QString::number(cylin->z2));

  this->Internal->CylinderRadius->setText(QString::number(cylin->r));
  this->Internal->CylinderMaterial->setVisible(false); // should be a table for layers
  this->Internal->labelCylMaterial->setVisible(false);

  //this->Internal->CylinderMaterial->setCurrentIndex(
  //  this->Internal->CylinderMaterial->findText(cylin->material.c_str()));
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

  this->Internal->DuctThickY->setVisible(this->GeometryType != HEXAGONAL);

  this->setUpDuctTable(this->GeometryType == HEXAGONAL, duct);
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetLattice(Lattice* lattice)
{
  if(this->GeometryType == RECTILINEAR)
    {
    this->Internal->latticeX->blockSignals(true);
    this->Internal->latticeY->blockSignals(true);
    this->Internal->latticeX->setValue(lattice->GetDimensions().first);
    this->Internal->latticeY->setValue(lattice->GetDimensions().second);
    this->Internal->latticeX->blockSignals(false);
    this->Internal->latticeY->blockSignals(false);
    }
  else if(this->GeometryType == HEXAGONAL)
    {
    this->Internal->hexLatticeAssy->blockSignals(true);
    this->Internal->hexLatticeAssy->setValue(lattice->GetDimensions().first);
    this->Internal->hexLatticeAssy->blockSignals(false);
  }

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
    for(size_t i = 0; i < this->Assembly->PinCells.size(); i++)
      {
      PinCell *pincell = this->Assembly->PinCells[i];
      actionList.append(pincell->label.c_str());
      }
    if(this->GeometryType == RECTILINEAR)
      {
      this->Assembly->UpdateGrid();
      this->AssemblyEditor->resetUI(
        this->Assembly->AssyLattice.Grid, actionList);
      }
    else if(this->GeometryType == HEXAGONAL)
      {
      this->HexAssy->setActions(actionList);
      this->HexAssy->resetWithGrid(this->Assembly->AssyLattice.Grid);
      }
    }
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::onNumberOfDuctLayersChanged(int numLayers)
{
/*  this->Internal->DuctLayers->blockSignals(true);
  int previusLayer = this->Internal->DuctLayer->currentIndex();
  this->Internal->DuctLayers->clear();
  for(int i=0; i<numLayers; i++)
    {
    this->Internal->DuctLayers->addItem(QString::number(i));
    }
  // add new layers if the number is increased
  if(numLayers > this->Internal->DuctMaterials.count())
    {
    int nmaterials = this->Internal->DuctMaterials.count();
    for(int j = nmaterials; j<numLayers; j++)
      {
      this->Internal->DuctMaterials.append("");
      this->Internal->DuctThicknesses.append(
        qMakePair(0.0,0.0));
      }
    }
  // remove layers if the number is decreased
  else if(numLayers < this->Internal->DuctMaterials.count())
    {
    for(int j = this->Internal->DuctMaterials.count() - 1; j >= numLayers; j--)
      {
      this->Internal->DuctMaterials.removeAt(j);
      this->Internal->DuctThicknesses.removeAt(j);
      }
    }

  this->Internal->DuctLayers->blockSignals(false);
  int currentLayer = previusLayer < numLayers ? previusLayer : 0;
  this->onCurrentDuctLayerChanged(currentLayer >= 0 ? currentLayer : 0);*/
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::onCurrentDuctLayerChanged(int idx)
{
/*  if(idx < this->Internal->DuctMaterials.count())
    {
    this->Internal->DuctLayerMaterial->
      setCurrentIndex(this->Internal->DuctLayerMaterial->findText(this->Internal->DuctMaterials.value(idx).toLower()));
    this->Internal->DuctThick1->setText(QString::number(
      this->Internal->DuctThicknesses.value(idx).first));
    this->Internal->DuctThick2->setText(QString::number(
      this->Internal->DuctThicknesses.value(idx).second));
    }*/
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::onCurrentDuctMaterialChanged()
{
/*  int currentLayer = this->Internal->DuctLayer->currentIndex();
  this->Internal->DuctMaterials.replace(currentLayer,
    this->Internal->DuctLayerMaterial->currentText());*/
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::onDuctThicknessChanged()
{
/*  int currentLayer = this->Internal->DuctLayer->currentIndex();
  double dThick1 = this->Internal->DuctThick1->text().toDouble();
  double dThick2 = this->Internal->DuctThick2->text().toDouble();
  this->Internal->DuctThicknesses.replace(currentLayer,
    qMakePair(dThick1, dThick2));*/
}

// apply property panel to given object
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToPinCell(PinCell* pincell)
{
  this->Internal->PinCellEditor->Apply();
  emit this->objGeometryChanged(pincell);
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToFrustum(Frustum* frust)
{
  frust->x = this->Internal->FrustumXPos->text().toDouble();
  frust->y = this->Internal->FrustumYPos->text().toDouble();
  frust->z1 = this->Internal->FrustumZPos1->text().toDouble();
  frust->z2 = this->Internal->FrustumZPos2->text().toDouble();
  frust->r1 = this->Internal->FrustumRadius1->text().toDouble();
  frust->r2 = this->Internal->FrustumRadius2->text().toDouble();

  //frust->material = this->Internal->FrustumMaterial->currentText().toStdString();
  emit this->objGeometryChanged(frust);
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToCylinder(Cylinder* cylin)
{
  cylin->x = this->Internal->CylinderXPos->text().toDouble();
  cylin->y = this->Internal->CylinderYPos->text().toDouble();
  cylin->z1 = this->Internal->CylinderZPos1->text().toDouble();
  cylin->z2 = this->Internal->CylinderZPos2->text().toDouble();
  cylin->r = this->Internal->CylinderRadius->text().toDouble();

  //cylin->material = this->Internal->CylinderMaterial->currentText().toStdString();
  emit this->objGeometryChanged(cylin);
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToDuct(Duct* duct)
{
  duct->x = this->Internal->DuctXPos->text().toDouble();
  duct->y = this->Internal->DuctYPos->text().toDouble();
  duct->z1 = this->Internal->DuctZPos1->text().toDouble();
  duct->z2 = this->Internal->DuctZPos2->text().toDouble();

  duct->thickness[0] = this->Internal->DuctThickX->text().toDouble();
  duct->thickness[1] = this->Internal->DuctThickY->text().toDouble();

  this->setDuctValuesFromTable(duct);

  emit this->objGeometryChanged(duct);
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::onLatticeDimensionChanged()
{
  this->AssemblyEditor->clearUI(false);
  this->AssemblyEditor->updateLatticeView(this->Internal->latticeX->value(),
    this->Internal->latticeY->value());
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::onCoreDimensionChanged()
{
  this->CoreEditor->clearUI(false);
  this->CoreEditor->updateLatticeView(this->Internal->coreLatticeX->value(),
    this->Internal->coreLatticeY->value());
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToLattice(Lattice* lattice)
{
  if(this->GeometryType == RECTILINEAR)
    {
    this->AssemblyEditor->updateLatticeWithGrid(lattice->Grid);
    }
  else if(this->GeometryType == HEXAGONAL)
    {
    this->HexAssy->applyToGrid(lattice->Grid);
    }
  emit this->objGeometryChanged(lattice);
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToAssembly(cmbNucAssembly* assy)
{
  //assy->MeshSize = this->Internal->MeshSize->text().toDouble();
  if(this->GeometryType == RECTILINEAR)
  {
    this->RectAssyConf->applyToAssembly(assy);
  }
  else if(this->GeometryType == HEXAGONAL)
  {
    this->HexAssyConf->applyToAssembly(assy);
  }

  emit this->objGeometryChanged(assy);
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToCore(cmbNucCore* nucCore)
{
  if(this->GeometryType == RECTILINEAR)
    {
    this->RectCoreProperties->applyToCore(nucCore);
    this->CoreEditor->updateLatticeWithGrid(nucCore->CoreLattice.Grid);
    }
  else if(this->GeometryType == HEXAGONAL)
    {
    this->HexCoreProperties->applyToCore(nucCore);
    this->HexCore->applyToGrid(nucCore->CoreLattice.Grid);
    }

  emit this->objGeometryChanged(nucCore);
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetAssembly(cmbNucAssembly* assy)
{
  //this->Internal->MeshSize->setText(QString::number(assy->MeshSize));
  this->HexAssyConf->resetAssembly(assy);
  this->RectAssyConf->resetAssembly(assy);

  // Show color swatch with legendColor
  QLabel* swatch = this->GeometryType == RECTILINEAR ?
    this->Internal->assyColorSwatch : this->Internal->hexAssyColorSwatch;

  QPalette palette = swatch->palette();
  palette.setColor(swatch->backgroundRole(), assy->GetLegendColor());
  swatch->setPalette(palette);
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetCore(cmbNucCore* nucCore)
{
  if(nucCore)
    {
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
    if(this->GeometryType == RECTILINEAR)
      {
      this->CoreEditor->resetUI(nucCore->CoreLattice.Grid, actionList);
      }
    else if(this->GeometryType == HEXAGONAL)
      {
      this->Internal->hexLattice->blockSignals(true);
      this->Internal->hexLattice->setValue(nucCore->GetDimensions().first);
      this->Internal->hexLattice->blockSignals(false);
      this->HexCore->setActions(actionList);
      this->HexCore->resetWithGrid(nucCore->CoreLattice.Grid);
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
    }
  this->Internal->PinCellEditor->SetPinCell(pincell,this->GeometryType == HEXAGONAL);
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

void cmbNucInputPropertiesWidget::chooseHexAssyLegendColor()
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
    QPalette palette = this->Internal->hexAssyColorSwatch->palette();
    palette.setColor(this->Internal->hexAssyColorSwatch->backgroundRole(), selected);
    this->Internal->hexAssyColorSwatch->setPalette(palette);

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
  this->HexAssy->setLayers(this->Internal->hexLatticeAssy->value());
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
  tmpTable->setRowCount(duct->materials.size());

  for(size_t i = 0; i < duct->materials.size(); i++)
  {
    // Private helper method to create the UI for a duct layer
    QComboBox* comboBox = new QComboBox;
    size_t row = i;
    Duct::Material m = duct->materials[i];

    cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
    foreach(QString material, matColorMap->MaterialColorMap().keys())
    {
      QString mat = matColorMap->MaterialColorMap()[material].Label;
      comboBox->addItem(mat);
      if (mat.toStdString() == m.material)
      {
        comboBox->setCurrentIndex(comboBox->count() - 1);
      }
    }
    tmpTable->setCellWidget(row, 0, comboBox);

    QTableWidgetItem* thick1Item = new DuctLayerThicknessEditor;
    QTableWidgetItem* thick2Item = new DuctLayerThicknessEditor;

    thick1Item->setText(QString::number(m.normThickness[0]));
    thick2Item->setText(QString::number(m.normThickness[1]));

    tmpTable->setItem(row, 1, thick1Item);
    tmpTable->setItem(row, 2, thick2Item);
  }
  tmpTable->resizeColumnsToContents();
  tmpTable->blockSignals(false);
}

void cmbNucInputPropertiesWidget::setDuctValuesFromTable(Duct* duct)
{
  if(duct == NULL) return;
  duct->materials.clear();
  QTableWidget * table = this->Internal->DuctLayers;
  for(int i = 0; i < table->rowCount(); i++)
  {
    Duct::Material m = duct->materials[i];
    QString mat = qobject_cast<QComboBox *>(table->cellWidget(i, 0))->currentText();
    m.material = mat.toStdString();
    m.normThickness[0] = table->item(i, 1)->data(Qt::DisplayRole).toDouble();
    m.normThickness[1] = table->item(i, 2)->data(Qt::DisplayRole).toDouble();
    duct->materials.push_back(m);
  }
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
  QTableWidgetItem* selItem =
  table->selectedItems().value(0);
  table->removeRow(selItem->row());
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
  foreach(QString material, matColorMap->MaterialColorMap().keys())
  {
    QString mat = matColorMap->MaterialColorMap()[material].Label;
    comboBox->addItem(mat);
  }
  table->setCellWidget(row, 0, comboBox);

  QTableWidgetItem* thick1Item = new DuctLayerThicknessEditor;
  QTableWidgetItem* thick2Item = new DuctLayerThicknessEditor;

  thick1Item->setText(QString::number(thickness[0]));
  thick2Item->setText(QString::number(thickness[1]));

  table->setItem(row, 1, thick1Item);
  table->setItem(row, 2, thick2Item);
}
