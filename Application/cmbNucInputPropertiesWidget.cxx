
#include "cmbNucInputPropertiesWidget.h"
#include "ui_qInputPropertiesWidget.h"

#include "cmbNucAssembly.h"
#include "cmbNucAssemblyEditor.h"
#include "cmbNucCore.h"
#include "cmbNucPinCellEditor.h"
#include "cmbNucMaterialColors.h"
#include "cmbNucMainWindow.h"

#include <QLabel>
#include <QPointer>
#include <QtDebug>
#include <QColorDialog>

class cmbNucInputPropertiesWidgetInternal :
  public Ui::InputPropertiesWidget
{
public:

  QStringList DuctMaterials; // size is the number of layers
  QList<QPair<double, double> > DuctThicknesses; //size is twice the number of layers
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
  this->Internal->latticecontainerLayout->addWidget(
    this->AssemblyEditor);
  this->CoreEditor = new cmbNucAssemblyEditor(this, this->Assembly);
  this->Internal->coreLatticeLayout->addWidget(
    this->CoreEditor);

  this->Internal->colorSwatch->setFrameStyle(QFrame::Box | QFrame::Plain);

  QObject::connect(this->Internal->ApplyButton, SIGNAL(clicked()),
    this, SLOT(onApply()));
  QObject::connect(this->Internal->ResetButton, SIGNAL(clicked()),
    this, SLOT(onReset()));

  // duct related connections
  QObject::connect(this->Internal->NumOfDuctLayers, SIGNAL(valueChanged(int)),
    this, SLOT(onNumberOfDuctLayersChanged(int)));
  QObject::connect(this->Internal->DuctLayer, SIGNAL(currentIndexChanged(int)),
    this, SLOT(onCurrentDuctLayerChanged(int)));

  QObject::connect(this->Internal->DuctLayerMaterial, SIGNAL(currentIndexChanged(int)),
    this, SLOT(onCurrentDuctMaterialChanged()));
  QObject::connect(this->Internal->DuctThick1, SIGNAL(editingFinished()),
    this, SLOT(onDuctThicknessChanged()));
  QObject::connect(this->Internal->DuctThick2, SIGNAL(editingFinished()),
    this, SLOT(onDuctThicknessChanged()));
  QObject::connect(this->Internal->latticeX, SIGNAL(valueChanged(int)),
    this, SLOT(onLatticeDimensionChanged()));
  QObject::connect(this->Internal->latticeY, SIGNAL(valueChanged(int)),
    this, SLOT(onLatticeDimensionChanged()));
  QObject::connect(this->Internal->coreLatticeX, SIGNAL(valueChanged(int)),
    this, SLOT(onCoreDimensionChanged()));
  QObject::connect(this->Internal->coreLatticeY, SIGNAL(valueChanged(int)),
    this, SLOT(onCoreDimensionChanged()));

  // pincell related connections
  QObject::connect(this->Internal->advancedButton, SIGNAL(clicked()),
    this, SLOT(showPinCellEditor()));
  QObject::connect(this->Internal->colorSelectButton, SIGNAL(clicked()),
    this, SLOT(choosePinLegendColor()));
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::setObject(AssyPartObj* selObj, const char* name)
{
  if(this->CurrentObject == selObj)
    {
    return;
    }
  this->CurrentObject = selObj;
  this->Internal->labelInput->setText("");
  if(!selObj)
    {
    this->setEnabled(0);
    return;
    }
  this->setEnabled(true);
  this->Internal->labelInput->setText(name);

  this->onReset();
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::updateMaterials()
{
  // update materials
  this->Internal->DuctLayerMaterial->blockSignals(true);
  this->Internal->FrustumMaterial->blockSignals(true);
  this->Internal->CylinderMaterial->blockSignals(true);

  this->Internal->DuctLayerMaterial->clear();
  this->Internal->FrustumMaterial->clear();
  this->Internal->CylinderMaterial->clear();

  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  QString matLabel;
  foreach(QString material, matColorMap->MaterialColorMap().keys())
    {
    matLabel = matColorMap->MaterialColorMap()[material].first;
    this->Internal->DuctLayerMaterial->addItem(matLabel);
    this->Internal->FrustumMaterial->addItem(matLabel);
    this->Internal->CylinderMaterial->addItem(matLabel);
    }

  this->Internal->DuctLayerMaterial->blockSignals(false);
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
}
// Invoked when Apply button clicked
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::onApply()
{
  if(this->CurrentObject==NULL)
    {
    return;
    }
  AssyPartObj* selObj = this->CurrentObject;
  PinCell* pincell=NULL;
  Cylinder* cylin=NULL;
  Frustum* frust=NULL;
  Duct* duct=NULL;
  Lattice* lattice=NULL;
  cmbNucCore* nucCore=NULL;
  cmbNucAssembly* assy=NULL;
  switch(selObj->GetType())
    {
    case CMBNUC_CORE:
      nucCore = dynamic_cast<cmbNucCore*>(selObj);
      this->applyToCore(nucCore);
      break;
    case CMBNUC_ASSEMBLY:
      assy = dynamic_cast<cmbNucAssembly*>(selObj);
      this->applyToAssembly(assy);
      break;
    case CMBNUC_ASSY_LATTICE:
      lattice = dynamic_cast<Lattice*>(selObj);
      this->applyToLattice(lattice);
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
  if(this->CurrentObject==NULL)
    {
    return;
    }
  AssyPartObj* selObj = this->CurrentObject;
  PinCell* pincell=NULL;
  Cylinder* cylin=NULL;
  Frustum* frust=NULL;
  Duct* duct=NULL;
  Lattice* lattice=NULL;
  cmbNucCore* nucCore=NULL;
  cmbNucAssembly* assy=NULL;
  switch(selObj->GetType())
    {
    case CMBNUC_CORE:
      this->Internal->stackedWidget->setCurrentWidget(
        this->Internal->pageCore);
      nucCore = dynamic_cast<cmbNucCore*>(selObj);
      this->resetCore(nucCore);
      break;
    case CMBNUC_ASSEMBLY:
      this->Internal->stackedWidget->setCurrentWidget(
        this->Internal->pageAssembly);
      assy = dynamic_cast<cmbNucAssembly*>(selObj);
      this->resetAssembly(assy);
      break;
    case CMBNUC_ASSY_LATTICE:
      this->Internal->stackedWidget->setCurrentWidget(
        this->Internal->pageLattice);
      lattice = dynamic_cast<Lattice*>(selObj);
      this->resetLattice(lattice);
      break;
    case CMBNUC_ASSY_PINCELL:
      this->Internal->stackedWidget->setCurrentWidget(
        this->Internal->pagePinCell);
      pincell = dynamic_cast<PinCell*>(selObj);
      this->resetPinCell(pincell);

      // if the pincell is empty bring up the pin cell editor
      if (pincell->NumberOfSections() == 0)
        {this->showPinCellEditor();}
        
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
// reset property panel with given object
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetPinCell(PinCell* pincell)
{
  this->Internal->PinCellLabel->setText(pincell->label.c_str());

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

  // update the material and thickness cache.
  this->Internal->DuctMaterials.clear();
  this->Internal->DuctThicknesses.clear();
  for(size_t i = 0; i < duct->materials.size(); i++)
    {
    this->Internal->DuctMaterials.append(duct->materials[i].c_str());
    this->Internal->DuctThicknesses.append(
      qMakePair(duct->thicknesses[2*i],duct->thicknesses[2*i+1]));
    }
  //layers
  this->Internal->NumOfDuctLayers->setValue((int)duct->materials.size());
  this->onNumberOfDuctLayersChanged(this->Internal->NumOfDuctLayers->value());
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetLattice(Lattice* lattice)
{
  this->Internal->latticeX->blockSignals(true);
  this->Internal->latticeY->blockSignals(true);
  this->Internal->latticeX->setValue(lattice->GetDimensions().first);
  this->Internal->latticeY->setValue(lattice->GetDimensions().second);
  this->Internal->latticeX->blockSignals(false);
  this->Internal->latticeY->blockSignals(false);

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
    this->Assembly->UpdateGrid();
    this->AssemblyEditor->resetUI(
      this->Assembly->AssyLattice.Grid, actionList);
    }
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::onNumberOfDuctLayersChanged(int numLayers)
{
  this->Internal->DuctLayer->blockSignals(true);
  int previusLayer = this->Internal->DuctLayer->currentIndex();
  this->Internal->DuctLayer->clear();
  for(int i=0; i<numLayers; i++)
    {
    this->Internal->DuctLayer->addItem(QString::number(i));
    }
  // add new layers if the number is increased
  if(numLayers > this->Internal->DuctMaterials.count())
    {
    int nmaterials=this->Internal->DuctMaterials.count();
    for(int j=nmaterials; j<numLayers; j++)
      {
      this->Internal->DuctMaterials.append("");
      this->Internal->DuctThicknesses.append(
        qMakePair(0.0,0.0));
      }
    }
  // remove layers if the number is decreased
  else if(numLayers < this->Internal->DuctMaterials.count())
    {
    for(int j=this->Internal->DuctMaterials.count()-1; j>=numLayers; j--)
      {
      this->Internal->DuctMaterials.removeAt(j);
      this->Internal->DuctThicknesses.removeAt(j);
      }
    }

  this->Internal->DuctLayer->blockSignals(false);
  int currentLayer = previusLayer<numLayers ? previusLayer : 0;
  this->onCurrentDuctLayerChanged(currentLayer>=0 ? currentLayer : 0);
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::onCurrentDuctLayerChanged(int idx)
{
  if(idx < this->Internal->DuctMaterials.count())
    {
    this->Internal->DuctLayerMaterial->setCurrentIndex(
      this->Internal->DuctLayerMaterial->findText(
      this->Internal->DuctMaterials.value(idx)));
    this->Internal->DuctThick1->setText(QString::number(
      this->Internal->DuctThicknesses.value(idx).first));
    this->Internal->DuctThick2->setText(QString::number(
      this->Internal->DuctThicknesses.value(idx).second));
    }
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::onCurrentDuctMaterialChanged()
{
  int currentLayer = this->Internal->DuctLayer->currentIndex();
  this->Internal->DuctMaterials.replace(currentLayer,
    this->Internal->DuctLayerMaterial->currentText());
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::onDuctThicknessChanged()
{
  int currentLayer = this->Internal->DuctLayer->currentIndex();
  double dThick1 = this->Internal->DuctThick1->text().toDouble();
  double dThick2 = this->Internal->DuctThick2->text().toDouble();
  this->Internal->DuctThicknesses.replace(currentLayer,
    qMakePair(dThick1, dThick2));
}

// apply property panel to given object
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToPinCell(PinCell* pincell)
{
  pincell->label = this->Internal->PinCellLabel->text().toStdString();
  emit this->currentObjectModified(pincell);
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
  emit this->currentObjectModified(frust);
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
  emit this->currentObjectModified(cylin);
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToDuct(Duct* duct)
{
  if(this->Internal->NumOfDuctLayers->value() !=
     this->Internal->DuctMaterials.size())
    {
    qCritical() << "The duct layers and their materials are not set properly.";
    return;
    }

  duct->x = this->Internal->DuctXPos->text().toDouble();
  duct->y = this->Internal->DuctYPos->text().toDouble();
  duct->z1 = this->Internal->DuctZPos1->text().toDouble();
  duct->z2 = this->Internal->DuctZPos2->text().toDouble();

  // update the material and thickness for all layers.
  duct->materials.clear();
  duct->thicknesses.clear();
  for(int i = 0; i < this->Internal->DuctMaterials.count(); i++)
    {
    duct->materials.push_back(this->Internal->DuctMaterials.value(i).toStdString());
    duct->thicknesses.push_back(this->Internal->DuctThicknesses.value(i).first);
    duct->thicknesses.push_back(this->Internal->DuctThicknesses.value(i).second);
    }

  emit this->currentObjectModified(duct);
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
  this->AssemblyEditor->updateLatticeWithGrid(lattice->Grid);
  emit this->currentObjectModified(lattice);
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToAssembly(cmbNucAssembly* assy)
{
  assy->MeshSize = this->Internal->MeshSize->text().toDouble();
  emit this->currentObjectModified(assy);
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToCore(cmbNucCore* nucCore)
{
  this->CoreEditor->updateLatticeWithGrid(nucCore->Grid);
  emit this->currentObjectModified(nucCore);
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetAssembly(cmbNucAssembly* assy)
{
  this->Internal->MeshSize->setText(QString::number(assy->MeshSize));
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetCore(cmbNucCore* nucCore)
{
  this->Internal->coreLatticeX->blockSignals(true);
  this->Internal->coreLatticeY->blockSignals(true);
  this->Internal->coreLatticeX->setValue(nucCore->GetDimensions().first);
  this->Internal->coreLatticeY->setValue(nucCore->GetDimensions().second);
  this->Internal->coreLatticeX->blockSignals(false);
  this->Internal->coreLatticeY->blockSignals(false);
  if(nucCore)
    {
    QStringList actionList;
    actionList.append("xx");
    // assemblies
    for(int i = 0; i < nucCore->GetNumberOfAssemblies(); i++)
      {
      cmbNucAssembly *assy = nucCore->GetAssembly(i);
      actionList.append(assy->label.c_str());
      }

    this->CoreEditor->resetUI(nucCore->Grid, actionList);
    }
}

void cmbNucInputPropertiesWidget::showPinCellEditor()
{
  // get the current pincell
  PinCell* pincell = dynamic_cast<PinCell*>(this->CurrentObject);
  if(!pincell)
    {
    std::cerr << "Error: don't have pincell" << std::endl;
    return;
    }

  cmbNucPinCellEditor *editor = new cmbNucPinCellEditor(this);
  editor->SetPinCell(pincell);
  editor->setZScale(this->MainWindow->getZScale());
  editor->setWindowFlags(Qt::Dialog);
  connect(editor, SIGNAL(accepted()), this, SLOT(pinCellEditorAccepted()));
  connect(this->MainWindow, SIGNAL(updateGlobalZScale(double)),
          editor, SLOT(setZScale(double)));
  editor->show();
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

void cmbNucInputPropertiesWidget::pinCellEditorAccepted()
{
  cmbNucPinCellEditor *editor = qobject_cast<cmbNucPinCellEditor *>(sender());
  if(!editor)
    {
    return;
    }

  this->applyToPinCell(editor->GetPinCell());
  //emit this->currentObjectModified(editor->GetPinCell());
}
