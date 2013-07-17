
#include "cmbNucInputPropertiesWidget.h"
#include "ui_qInputPropertiesWidget.h"

#include "cmbNucAssemblyEditor.h"

#include <QLabel>
#include <QPointer>
#include <QtDebug>

class cmbNucInputPropertiesWidgetInternal : 
  public Ui::InputPropertiesWidget
{
public:

  QStringList DuctMaterials; // size is the number of layers
  QList<QPair<double, double> > DuctThicknesses; //size is twice the number of layers
};

//-----------------------------------------------------------------------------
cmbNucInputPropertiesWidget::cmbNucInputPropertiesWidget(
  QWidget* _p): QWidget(_p)
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
  this->AssemblyEditor = new cmbNucAssemblyEditor(this);
  this->Internal->latticecontainerLayout->addWidget(
    this->AssemblyEditor);
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
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::setObject(AssyPartObj* selObj,
  const char* name, const QStringList& materials)
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
  // update materials
  this->Internal->DuctLayerMaterial->blockSignals(true);
  this->Internal->FrustumMaterial->blockSignals(true);
  this->Internal->CylinderMaterial->blockSignals(true);
  
  this->Internal->DuctLayerMaterial->clear();
  this->Internal->FrustumMaterial->clear();
  this->Internal->CylinderMaterial->clear();
  foreach(QString material, materials)
    {
    this->Internal->DuctLayerMaterial->addItem(material);
    this->Internal->FrustumMaterial->addItem(material);
    this->Internal->CylinderMaterial->addItem(material);
    }
  this->Internal->DuctLayerMaterial->blockSignals(false);
  this->Internal->FrustumMaterial->blockSignals(false);
  this->Internal->CylinderMaterial->blockSignals(false);

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
  Material* material=NULL;
  Lattice* lattice=NULL;
  switch(selObj->GetType())
    {
    case ASSY_LATTICE:
      lattice = dynamic_cast<Lattice*>(selObj);
      this->applyToLattice(lattice);
      break;
    case ASSY_MATERIAL:
      material = dynamic_cast<Material*>(selObj);
      this->applyToMaterial(material);
      break;
    case ASSY_PINCELL:
      pincell = dynamic_cast<PinCell*>(selObj);
      this->applyToPinCell(pincell);
      break;
    case ASSY_FRUSTUM_PIN:
      frust = dynamic_cast<Frustum*>(selObj);
      this->applyToFrustum(frust);
      break;
    case ASSY_CYLINDER_PIN:
      cylin = dynamic_cast<Cylinder*>(selObj);
      this->applyToCylinder(cylin);
      break;
    case ASSY_RECT_DUCT:
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
  Material* material=NULL;
  Lattice* lattice=NULL;
  switch(selObj->GetType())
    {
    case ASSY_LATTICE:
      this->Internal->stackedWidget->setCurrentWidget(
        this->Internal->pageLattice);
      lattice = dynamic_cast<Lattice*>(selObj);
      this->resetLattice(lattice);
      break;
    case ASSY_MATERIAL:
      this->Internal->stackedWidget->setCurrentWidget(
        this->Internal->pageMaterial);
      material = dynamic_cast<Material*>(selObj);
      this->resetMaterial(material);
      break;
    case ASSY_PINCELL:
      this->Internal->stackedWidget->setCurrentWidget(
        this->Internal->pagePinCell);
      pincell = dynamic_cast<PinCell*>(selObj);
      this->resetPinCell(pincell);
      break;
    case ASSY_FRUSTUM_PIN:
      this->Internal->stackedWidget->setCurrentWidget(
        this->Internal->pageFrustumPin);
      frust = dynamic_cast<Frustum*>(selObj);
      this->resetFrustum(frust);
      break;
    case ASSY_CYLINDER_PIN:
      this->Internal->stackedWidget->setCurrentWidget(
        this->Internal->pageCylinderPin);
      cylin = dynamic_cast<Cylinder*>(selObj);
      this->resetCylinder(cylin);
      break;
    case ASSY_RECT_DUCT:
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
void cmbNucInputPropertiesWidget::resetMaterial(Material* material)
{
  this->Internal->MaterialLabel->setText(material->label.c_str());
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetPinCell(PinCell* pincell)
{
  this->Internal->PinCellLabel->setText(pincell->label.c_str());
  this->Internal->CellPitchX->setText(QString::number(pincell->pitchX));
  this->Internal->CellPitchY->setText(QString::number(pincell->pitchY));
  this->Internal->CellPitchZ->setText(QString::number(pincell->pitchZ));
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

  this->Internal->FrustumMaterial->setCurrentIndex(
    this->Internal->FrustumMaterial->findText(frust->material.c_str()));
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetCylinder(Cylinder* cylin)
{
  this->Internal->CylinderXPos->setText(QString::number(cylin->x));
  this->Internal->CylinderYPos->setText(QString::number(cylin->y));
  this->Internal->CylinderZPos1->setText(QString::number(cylin->z1));
  this->Internal->CylinderZPos2->setText(QString::number(cylin->z2));

  this->Internal->CylinderRadius->setText(QString::number(cylin->r));
  this->Internal->CylinderMaterial->setCurrentIndex(
    this->Internal->CylinderMaterial->findText(cylin->material.c_str()));
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
  this->AssemblyEditor->resetUI();
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
  if(numLayers > this->Internal->DuctMaterials.size())
    {
    for(int j=numLayers; j<this->Internal->DuctMaterials.size(); j++)
      {
      // duplicate the last layer
      this->Internal->DuctMaterials.append(
        this->Internal->DuctMaterials.value(numLayers));
      this->Internal->DuctThicknesses.append(
        qMakePair(this->Internal->DuctThicknesses.value(numLayers).first,
                  this->Internal->DuctThicknesses.value(numLayers).second));
      }
    }
  // remove layers if the number is decreased
  else if(numLayers < this->Internal->DuctMaterials.size())
    {
    for(int j=this->Internal->DuctMaterials.size()-1; j>=numLayers; j--)
      {
      // duplicate the last layer
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
  if(idx < this->Internal->DuctMaterials.size())
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
  this->Internal->DuctThicknesses.replace(
    currentLayer, qMakePair(dThick1, dThick2));
}

// apply property panel to given object
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToMaterial(Material* material)
{
  material->label = this->Internal->MaterialLabel->text().toStdString();
  emit this->currentObjectModified(material);
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToPinCell(PinCell* pincell)
{
  pincell->label = this->Internal->PinCellLabel->text().toStdString();
  pincell->pitchX = this->Internal->CellPitchX->text().toDouble();
  pincell->pitchY = this->Internal->CellPitchY->text().toDouble();
  pincell->pitchZ = this->Internal->CellPitchZ->text().toDouble();
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

  frust->material = this->Internal->FrustumMaterial->currentText().toStdString();
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

  cylin->material = this->Internal->CylinderMaterial->currentText().toStdString();
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
  for(int i = 0; i < this->Internal->DuctMaterials.size(); i++)
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
void cmbNucInputPropertiesWidget::applyToLattice(Lattice* lattice)
{
  this->AssemblyEditor->updateLatticeWithGrid(lattice);
  emit this->currentObjectModified(lattice);
}
