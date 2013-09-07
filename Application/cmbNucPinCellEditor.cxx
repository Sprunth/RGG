
#include "cmbNucPinCellEditor.h"

#include <QComboBox>
#include <QTableWidgetItem>

#include <vtkRenderWindow.h>
#include <vtkClipClosedSurface.h>
#include "vtkCompositeDataDisplayAttributes.h"

#include "cmbNucPartDefinition.h"
#include "cmbNucAssembly.h"
#include "cmbNucMaterialColors.h"

class PinCellComponent
{
public:
  enum Type {
    CylinderType,
    FrustumType
  };

  explicit PinCellComponent(const Cylinder *cylinder)
    : type(CylinderType),
      z1(cylinder->z1),
      z2(cylinder->z2),
      r1(cylinder->r),
      r2(cylinder->r),
      PartObj(cylinder)
  {
  }

  explicit PinCellComponent(const Frustum *frustum)
    : type(FrustumType),
      z1(frustum->z1),
      z2(frustum->z2),
      r1(frustum->r1),
      r2(frustum->r2),
      PartObj(frustum)
  {
  }

  double length() const { return z2 - z1; }
  double radius1() const { return r1; }
  double radius2() const { return r2; }
  const AssyPartObj* obj() const { return PartObj; }

  enum Type type;
  double z1;
  double z2;
  double r1;
  double r2;
  const AssyPartObj* PartObj;
};

bool sort_by_z1(const PinCellComponent &a, const PinCellComponent &b)
{
  return a.z1 < b.z1;
}

cmbNucPinCellEditor::cmbNucPinCellEditor(QWidget *parent)
  : QWidget(parent),
    Ui(new Ui::cmbNucPinCellEditor),
    AssemblyObject(0)
{
  this->Ui->setupUi(this);

  this->resize(1200, 700);
  this->Ui->splitter->setSizes(QList<int>() << 400 << 1200 - 400);

  this->Renderer = vtkSmartPointer<vtkRenderer>::New();
  this->Ui->qvtkWidget->GetRenderWindow()->AddRenderer(this->Renderer);
  this->Actor = vtkSmartPointer<vtkActor>::New();
  this->Mapper = vtkSmartPointer<vtkCompositePolyDataMapper2>::New();
  this->Actor->SetMapper(this->Mapper);
  this->Renderer->AddActor(this->Actor);

  vtkCompositeDataDisplayAttributes *attributes = vtkCompositeDataDisplayAttributes::New();
  this->Mapper->SetCompositeDataDisplayAttributes(attributes);
  attributes->Delete();

  this->Ui->layersTable->setRowCount(0);
  this->Ui->layersTable->setColumnCount(2);
  this->Ui->layersTable->setHorizontalHeaderLabels(
    QStringList() << "Material" << "Radius (normalized)"
  );

  connect(this->Ui->acceptButton, SIGNAL(clicked()), this, SLOT(Apply()));
  connect(this->Ui->rejectButton, SIGNAL(clicked()), this, SLOT(close()));

  connect(this->Ui->addButton, SIGNAL(clicked()), this, SLOT(addComponent()));
  connect(this->Ui->deleteButton, SIGNAL(clicked()), this, SLOT(deleteComponent()));

  connect(this->Ui->piecesTable, SIGNAL(cellChanged(int, int)),
          this, SLOT(tableCellChanged(int, int)));
  connect(this->Ui->piecesTable, SIGNAL(itemSelectionChanged()),
    this, SLOT(onPieceSelected()));
  connect(this->Ui->layersSpinBox, SIGNAL(valueChanged(int)),
          this, SLOT(numberOfLayersChanged(int)));
  connect(this->Ui->layersTable, SIGNAL(cellChanged(int, int)),
          this, SLOT(layerTableCellChanged(int, int)));
}

cmbNucPinCellEditor::~cmbNucPinCellEditor()
{
  delete this->Ui;
}

void cmbNucPinCellEditor::SetPinCell(PinCell *pincell)
{
  this->PinCellObject = pincell;

  this->Ui->nameLineEdit->setText(pincell->name.c_str());
  this->Ui->labelLineEdit->setText(pincell->label.c_str());
  this->Ui->originX->setText("0");
  this->Ui->originY->setText("0");
  this->Ui->pitchX->setText(QString::number(pincell->pitchX));
  this->Ui->pitchY->setText(QString::number(pincell->pitchY));
  this->Ui->pitchZ->setText(QString::number(pincell->pitchZ));

  this->Ui->piecesTable->blockSignals(true);

  this->Ui->piecesTable->setColumnCount(4);
  this->Ui->piecesTable->setHorizontalHeaderLabels(
    QStringList() << "Type" << "Length" << "Radius (base)" << "Radius (top)"
  );

  std::vector<PinCellComponent> components;
  for(size_t i = 0; i < pincell->cylinders.size(); i++){
    components.push_back(PinCellComponent(pincell->cylinders[i]));
  }
  for(size_t i = 0; i < pincell->frustums.size(); i++){
    components.push_back(PinCellComponent(pincell->frustums[i]));
  }
  std::sort(components.begin(), components.end(), sort_by_z1);

  this->Ui->piecesTable->setRowCount(components.size());
  for(size_t i = 0; i < components.size(); i++){
    const PinCellComponent &component = components[i];
    this->createComponentItem(i, component.length(),
      component.radius1(), component.radius2());
    if(component.obj())
      {
      QComboBox *comboBox = qobject_cast<QComboBox *>(
        this->Ui->piecesTable->cellWidget(i, 0));
      if(component.type == PinCellComponent::FrustumType)
        {
        comboBox->blockSignals(true);
        comboBox->setCurrentIndex(1);
        comboBox->blockSignals(false);
        }
      QVariant vdata;
      vdata.setValue((void*)(component.obj()));
      comboBox->setItemData(0, vdata);
      vdata.setValue(i);
      comboBox->setItemData(1, vdata);
      }
  }
  this->Ui->piecesTable->resizeColumnsToContents();

  this->Ui->piecesTable->blockSignals(false);

  // Select the first row
  if(this->Ui->piecesTable->rowCount()>0)
    {
    QTableWidgetItem* selItem = this->Ui->piecesTable->item(0, 0);
    this->Ui->piecesTable->setCurrentItem(selItem);
    selItem->setSelected(true); 
    }

  this->UpdatePolyData();
}

PinCell* cmbNucPinCellEditor::GetPinCell()
{
  return this->PinCellObject;
}

void cmbNucPinCellEditor::Apply()
{
  this->UpdatePinCell();

  emit this->accepted();
  this->close();
}

void cmbNucPinCellEditor::UpdatePinCell()
{
  // apply pincell attributes
  this->PinCellObject->name = this->Ui->nameLineEdit->text().toStdString();
  this->PinCellObject->label = this->Ui->labelLineEdit->text().toStdString();
  this->PinCellObject->pitchX = this->Ui->pitchX->text().toDouble();
  this->PinCellObject->pitchY = this->Ui->pitchY->text().toDouble();
  this->PinCellObject->pitchZ = this->Ui->pitchZ->text().toDouble();

  // update components
  double z = 0;
  for(int i = 0; i < this->Ui->piecesTable->rowCount(); i++){
    this->updateComponentObject(i, z);
  }
}

void cmbNucPinCellEditor::updateComponentObject(int i, double& z)
{
  this->Ui->piecesTable->blockSignals(true);

  QTableWidgetItem *item = this->Ui->piecesTable->item(i, 0);
  QComboBox *comboBox = qobject_cast<QComboBox *>(this->Ui->piecesTable->cellWidget(i, 0));
  AssyPartObj* obj =
    static_cast<AssyPartObj*>(comboBox->itemData(0).value<void *>());
  if(!obj)
    {
    return;
    }
  double x = this->Ui->originX->text().toDouble();
  double y = this->Ui->originY->text().toDouble();
  double l = this->Ui->piecesTable->item(i, 1)->text().toDouble();
  double r1 = this->Ui->piecesTable->item(i, 2)->text().toDouble();
  double r2 = this->Ui->piecesTable->item(i, 3)->text().toDouble();

  if(obj->GetType() == CMBNUC_ASSY_CYLINDER_PIN){
      Cylinder *cylinder = dynamic_cast<Cylinder*>(obj);
      cylinder->x = x;
      cylinder->y = y;
      cylinder->z1 = z;
      cylinder->z2 = z + l;
      cylinder->r = r1;
  }
  else if(obj->GetType() == CMBNUC_ASSY_FRUSTUM_PIN){
      Frustum *frustum =  dynamic_cast<Frustum*>(obj);;
      frustum->x = x;
      frustum->y = y;
      frustum->z1 = z;
      frustum->z2 = z + l;
      frustum->r1 = r1;
      frustum->r2 = r2;
  }
  z += l;
  this->Ui->piecesTable->blockSignals(false);
}

AssyPartObj* cmbNucPinCellEditor::createComponentObject(int i, double& z)
{
  this->Ui->piecesTable->blockSignals(true);

  QTableWidgetItem *item = this->Ui->piecesTable->item(i, 0);

  QComboBox *comboBox = qobject_cast<QComboBox *>(this->Ui->piecesTable->cellWidget(i, 0));

  double x = this->Ui->originX->text().toDouble();
  double y = this->Ui->originY->text().toDouble();
  double l = this->Ui->piecesTable->item(i, 1)->text().toDouble();
  double r1 = this->Ui->piecesTable->item(i, 2)->text().toDouble();
  double r2 = this->Ui->piecesTable->item(i, 3)->text().toDouble();

  AssyPartObj* retObj = NULL;
  if(comboBox->currentText() == "Cylinder"){
      Cylinder *cylinder = new Cylinder;
      cylinder->SetNumberOfLayers(this->Ui->layersTable->rowCount());
      cylinder->x = x;
      cylinder->y = y;
      cylinder->z1 = z;
      cylinder->z2 = z + l;
      cylinder->r = r1;
      this->PinCellObject->cylinders.push_back(cylinder);
      retObj = cylinder;
  }
  else if(comboBox->currentText() == "Frustum"){
      Frustum *frustum = new Frustum;
      frustum->SetNumberOfLayers(this->Ui->layersTable->rowCount());
      frustum->x = x;
      frustum->y = y;
      frustum->z1 = z;
      frustum->z2 = z + l;
      frustum->r1 = r1;
      frustum->r2 = r2;
      this->PinCellObject->frustums.push_back(frustum);
      retObj = frustum;
  }
  z += l;
  if(retObj)
    {
    QVariant vdata;
    vdata.setValue((void*)(retObj));
    comboBox->setItemData(0, vdata);
    vdata.setValue(i); // row
    comboBox->setItemData(1, vdata);
    }
  this->Ui->piecesTable->blockSignals(false);

  return retObj;
}

void cmbNucPinCellEditor::UpdateLayerMaterials(AssyPartObj* objPart)
{
  // setup materials
  int materials = this->Ui->layersTable->rowCount();
  for(int i = 0; i < materials; i++)
    {
    QComboBox *comboBox = qobject_cast<QComboBox *>(this->Ui->layersTable->cellWidget(i, 0));
    if(comboBox && objPart)
      {
      if(objPart->GetType() == CMBNUC_ASSY_FRUSTUM_PIN)
        {
        dynamic_cast<Frustum*>(objPart)->SetMaterial(i, comboBox->currentText().toStdString());
        }
      else if(objPart->GetType() == CMBNUC_ASSY_CYLINDER_PIN)
        {
        dynamic_cast<Cylinder*>(objPart)->SetMaterial(i, comboBox->currentText().toStdString());
        }
      }
    }
}

void cmbNucPinCellEditor::UpdatePolyData()
{
  vtkMultiBlockDataSet *data_set = cmbNucAssembly::CreatePinCellMultiBlock(this->PinCellObject);

  this->Mapper->SetInputDataObject(data_set);
  vtkCompositeDataDisplayAttributes *attributes =
    this->Mapper->GetCompositeDataDisplayAttributes();

  size_t numCyls = this->PinCellObject->cylinders.size();
  size_t numFrus = this->PinCellObject->frustums.size();
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
      for(int k = 0; k < this->PinCellObject->GetNumberOfLayers(); k++)
        {
        pinMaterial = this->PinCellObject->cylinders[idx]->materials[k];
        matColorMap->SetBlockMaterialColor(attributes, flat_index++, pinMaterial);
        }
      }
    else
      {
      flat_index++; // increase one for this frustum
      for(int k = 0; k < this->PinCellObject->GetNumberOfLayers(); k++)
        {
        pinMaterial = this->PinCellObject->frustums[idx-numCyls]->materials[k];
        matColorMap->SetBlockMaterialColor(attributes, flat_index++, pinMaterial);
        }
      }
    }
  data_set->Delete();
  this->UpdateRenderView();
}

void cmbNucPinCellEditor::UpdateRenderView()
{
  this->Renderer->ResetCamera();
  this->Ui->qvtkWidget->update();
}

void cmbNucPinCellEditor::addComponent()
{
  int row = this->Ui->piecesTable->rowCount();
  this->Ui->piecesTable->setRowCount(row + 1);

  double default_length = 2.0;
  double default_radius = 0.5;

  if(row >= 1){
      // use radius from top of previous component
      default_radius = this->Ui->piecesTable->item(row - 1, 3)->text().toDouble();
  }

  this->createComponentItem(row, default_length, default_radius, default_radius);

  // Add the new component
  double z=0;
  for(int i = 0; i < row; i++)
    {
    double l = this->Ui->piecesTable->item(i, 1)->text().toDouble();
    z += l;
    }
  AssyPartObj* objPart = this->createComponentObject(row, z);

  // Select this row
  QTableWidgetItem* selItem = this->Ui->piecesTable->item(row, 0);
  this->Ui->piecesTable->setCurrentItem(selItem);
  selItem->setSelected(true);

  // update view
  this->UpdatePinCell();
  this->UpdatePolyData();
}

void cmbNucPinCellEditor::createComponentItem(
  int row, double default_length, double default_radius1, double default_radius2)
{
  this->Ui->piecesTable->blockSignals(true);
  // type
  QTableWidgetItem *item = new QTableWidgetItem;
  QComboBox *comboBox = new QComboBox;
  comboBox->addItem("Cylinder");
  comboBox->addItem("Frustum");

  connect(comboBox, SIGNAL(currentIndexChanged(QString)),
          this, SLOT(sectionTypeComboBoxChanged(QString)));
  this->Ui->piecesTable->setCellWidget(row, 0, comboBox);
  item = new QTableWidgetItem;
  this->Ui->piecesTable->setItem(row, 0, item);

  // length
  item = new QTableWidgetItem;
  item->setText(QString::number(default_length));
  this->Ui->piecesTable->setItem(row, 1, item);

  // radius (base)
  item = new QTableWidgetItem;
  item->setText(QString::number(default_radius1));
  this->Ui->piecesTable->setItem(row, 2, item);

  // radius (top)
  item = new QTableWidgetItem;
  item->setText(QString::number(default_radius2));
  this->Ui->piecesTable->setItem(row, 3, item);

  this->Ui->piecesTable->blockSignals(false);
}

void cmbNucPinCellEditor::deleteComponent()
{
  this->Ui->piecesTable->blockSignals(true);
  AssyPartObj* obj = this->getSelectedPiece();
  this->PinCellObject->RemoveSection(obj);
  int row = this->Ui->piecesTable->currentRow();
  this->Ui->piecesTable->removeRow(row);

  this->Ui->piecesTable->blockSignals(false);

  // update view
  this->UpdatePinCell();
  this->UpdatePolyData();
}

void cmbNucPinCellEditor::tableCellChanged(int row, int col)
{
  QTableWidgetItem *item = this->Ui->piecesTable->item(row, col);

  if(col == 2 || col == 3){
    // propogate radius changes
    this->Ui->piecesTable->blockSignals(true);

    bool is_cylinder =
      qobject_cast<QComboBox *>(this->Ui->piecesTable->cellWidget(row, 0))->currentText() == "Cylinder";

    // if the component is a cylinder and one of the radii was
    // changed update the other to match
    if(is_cylinder){
      int other_col = col == 2 ? 3 : 2;
      QTableWidgetItem *other_item = this->Ui->piecesTable->item(row, other_col);
      other_item->setText(item->text());
    }

    if(col == 3 || is_cylinder){
        for(int i = row + 1; i < this->Ui->piecesTable->rowCount(); i++){
          bool next_is_cylinder =
            qobject_cast<QComboBox *>(this->Ui->piecesTable->cellWidget(i, 0))->currentText() == "Cylinder";
          if(next_is_cylinder){
              // set both top and base radii and continue propagating
              this->Ui->piecesTable->item(i, 2)->setText(item->text());
              this->Ui->piecesTable->item(i, 3)->setText(item->text());
          }
          else {
              // set base radius only and stop propogating
              this->Ui->piecesTable->item(i, 2)->setText(item->text());
              break;
          }
        }
    }
    if(col == 2 || is_cylinder){
      for(int i = row - 1; i >= 0; i--){
          bool next_is_cylinder =
              qobject_cast<QComboBox *>(this->Ui->piecesTable->cellWidget(i, 0))->currentText() == "Cylinder";
          if(next_is_cylinder){
              // set both top and base radii and continue propagating
              this->Ui->piecesTable->item(i, 2)->setText(item->text());
              this->Ui->piecesTable->item(i, 3)->setText(item->text());
          }
          else {
              // set top radius only and stop propogating
              this->Ui->piecesTable->item(i, 3)->setText(item->text());
              break;
          }
      }
    }

    this->Ui->piecesTable->blockSignals(false);
  }

  // update pin cell and render view
  this->UpdatePinCell();
  this->UpdatePolyData();
}

void cmbNucPinCellEditor::numberOfLayersChanged(int layers)
{
  this->Ui->layersTable->blockSignals(true);
  int current = this->Ui->layersTable->rowCount();
  this->Ui->layersTable->setRowCount(layers);
  this->PinCellObject->SetNumberOfLayers(layers);

  // new rows
  for(int row=current; row<layers; row++)
    {
    QTableWidgetItem *item = new QTableWidgetItem;
    QComboBox *comboBox = new QComboBox;
    this->setupMaterialComboBox(comboBox);
    this->Ui->layersTable->setCellWidget(row, 0, comboBox);
    QObject::connect(comboBox, SIGNAL(currentIndexChanged(int)),
      this, SLOT(onUpdateLayerMaterial()));
    item = new QTableWidgetItem;
    item->setText(QString::number(1.0));
    this->Ui->layersTable->setItem(row, 1, item);
    this->PinCellObject->SetMaterial(row,
      comboBox->currentText().toStdString());
    }
  this->Ui->layersTable->blockSignals(false);
  this->UpdatePolyData();
}

void cmbNucPinCellEditor::sectionTypeComboBoxChanged(const QString &type)
{
  QComboBox *comboBox = qobject_cast<QComboBox *>(sender());
  if(!comboBox){
    return;
  }

  AssyPartObj* obj =
    static_cast<AssyPartObj*>(comboBox->itemData(0).value<void *>());
  if(!obj || (obj->GetType() != CMBNUC_ASSY_FRUSTUM_PIN &&
    obj->GetType() != CMBNUC_ASSY_CYLINDER_PIN))
    {
    return;
    }
 
  bool ok;
  int row = comboBox->itemData(1).toInt(&ok);
  if(ok)
    {
    // Add the new component
    double z= (obj->GetType() == CMBNUC_ASSY_FRUSTUM_PIN) ?
      dynamic_cast<Frustum*>(obj)->z1 :
    dynamic_cast<Cylinder*>(obj)->z1;
    AssyPartObj* objPart = this->createComponentObject(row, z);
    this->UpdateLayerMaterials(objPart);
    this->UpdatePolyData();
    this->PinCellObject->RemoveSection(obj);
    }
}

void cmbNucPinCellEditor::setupMaterialComboBox(QComboBox *comboBox)
{
  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  comboBox->addItems(matColorMap->MaterialColorMap().keys());
}

void cmbNucPinCellEditor::layerTableCellChanged(int row, int col)
{
  if(col == 1)
    {
    QTableWidgetItem *item = this->Ui->layersTable->item(row, col);
    if(item)
      {
      this->PinCellObject->radii[row] = item->text().toDouble();
      }
    }
  this->UpdatePolyData();
}

void cmbNucPinCellEditor::onUpdateLayerMaterial()
{
  this->UpdateLayerMaterials(this->getSelectedPiece());
  this->UpdatePolyData();
}

void cmbNucPinCellEditor::onPieceSelected()
{
  AssyPartObj* obj = this->getSelectedPiece();
  bool pieceSelected = (obj != NULL);
  this->Ui->layersSpinBox->setEnabled(pieceSelected);
  this->Ui->layersTable->setEnabled(pieceSelected);
  if(!pieceSelected)
    {
    return;
    }
  PinCell *pincell = this->PinCellObject;
  int layers = pincell->GetNumberOfLayers();
  if(layers < 1)
    {
    this->numberOfLayersChanged(1);
    }
  else
    {
    this->Ui->layersSpinBox->blockSignals(true);
    this->Ui->layersSpinBox->setValue(layers);
    this->Ui->layersSpinBox->blockSignals(false);
    this->Ui->layersTable->blockSignals(true);
    this->Ui->layersTable->setRowCount(layers);
    for(int i = 0; i < layers; i++)
      {
      QComboBox *comboBox = new QComboBox;
      this->setupMaterialComboBox(comboBox);
      this->Ui->layersTable->setCellWidget(i, 0, comboBox);
      this->Ui->layersTable->setItem(i, 0, new QTableWidgetItem);

      std::string strSelMat;
      if(obj)
        {
        if(obj->GetType() == CMBNUC_ASSY_FRUSTUM_PIN)
          {
          strSelMat = dynamic_cast<Frustum*>(obj)->GetMaterial(i);
          }
        else if(obj->GetType() == CMBNUC_ASSY_CYLINDER_PIN)
          {
          strSelMat = dynamic_cast<Cylinder*>(obj)->GetMaterial(i);
          }
        }

      int idx = -1;
      for(int j = 0; j < comboBox->count(); j++)
        {
        if(comboBox->itemText(j).toStdString() == strSelMat)
          {
          idx = j;
          break;
          }
        }
      comboBox->setCurrentIndex(idx);
      //if(!found && obj)
      //  {
        //if(obj->GetType() == CMBNUC_ASSY_FRUSTUM_PIN)
        //  {
        //  dynamic_cast<Frustum*>(obj)->SetMaterial(i,comboBox->currentText().toStdString());
        //  }
        //else if(obj->GetType() == CMBNUC_ASSY_CYLINDER_PIN)
        //  {
        //  dynamic_cast<Cylinder*>(obj)->SetMaterial(i, comboBox->currentText().toStdString());
        //  }
      //  }
      QTableWidgetItem *item = new QTableWidgetItem;
      item->setText(QString::number(pincell->radii[i]));
      this->Ui->layersTable->setItem(i, 1, item);

      QObject::connect(comboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(onUpdateLayerMaterial()));
      }
    this->Ui->layersTable->blockSignals(false);
    }
}
//-----------------------------------------------------------------------------
AssyPartObj *cmbNucPinCellEditor::getSelectedPiece()
{
  if(this->Ui->piecesTable->selectedItems().count()==0)
    {
    return NULL;
    }
  QTableWidgetItem* selItem = this->Ui->piecesTable->selectedItems().value(0);
  QComboBox *comboBox = qobject_cast<QComboBox *>(
    this->Ui->piecesTable->cellWidget(selItem->row(), 0));
  if(!comboBox){
    return NULL;
    }

  AssyPartObj* obj =
    static_cast<AssyPartObj*>(comboBox->itemData(0).value<void *>());
  return obj;
}