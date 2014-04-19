
#include "cmbNucPinCellEditor.h"

#include <QComboBox>
#include <QTableWidgetItem>

#include <vtkRenderWindow.h>
#include "vtkCompositeDataDisplayAttributes.h"

#include "cmbNucPartDefinition.h"
#include "cmbNucAssembly.h"
#include "cmbNucMaterialColors.h"

#define set_and_test(X,Y) \
change |= (Y) != X;\
X = (Y);


class PinCellComponent
{
public:
  enum Type {
    CylinderType,
    FrustumType
  };

  explicit PinCellComponent(const Cylinder *cylinder)
    : type(CylinderType),
      x(cylinder->x),
      y(cylinder->y),
      z1(cylinder->z1),
      z2(cylinder->z2),
      r1(cylinder->r),
      r2(cylinder->r),
      PartObj(cylinder)
  {
  }

  explicit PinCellComponent(const Frustum *frustum)
    : type(FrustumType),
      x(frustum->x),
      y(frustum->y),
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
  double x;
  double y;
  double z1;
  double z2;
  double r1;
  double r2;
  const AssyPartObj* PartObj;
};

// We use this class to validate the input to the radius fields for layers
class LayerRadiusEditor : public QTableWidgetItem
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
        if (dval < prev)
          {
          return;
          }
        }
      // Make sure value is less than next row
      if (this->row() < this->tableWidget()->rowCount() - 1)
        {
        double next = this->tableWidget()->item(this->row() + 1, 1)
                                         ->data(Qt::DisplayRole).toDouble();
        if (dval > next)
          {
          return;
          }
        }
      }
    QTableWidgetItem::setData(role, value);
    }
};

// We use this class to validate the input to the radius fields for segments
class SegmentRadiusEditor : public QTableWidgetItem
{
public:
  virtual void setData(int role, const QVariant& value)
    {
    if (this->tableWidget() != NULL && role == Qt::EditRole)
      {
      bool ok;
      double dval = value.toDouble(&ok);

      // Make sure value is positive
      if (!ok || dval < 0.)
        {
        return;
        }
      }
    QTableWidgetItem::setData(role, value);
    }
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
  isHex = false;
  this->Ui->setupUi(this);

  this->Ui->layersTable->setRowCount(0);
  this->Ui->layersTable->setColumnCount(2);
  this->Ui->layersTable->horizontalHeader()->setStretchLastSection(true);
  this->Ui->layersTable->setHorizontalHeaderLabels(
    QStringList() << "Material" << "Radius (normalized)"
  );

  connect(this->Ui->addButton, SIGNAL(clicked()), this, SLOT(addComponent()));
  connect(this->Ui->deleteButton, SIGNAL(clicked()), this, SLOT(deleteComponent()));

  connect(this->Ui->addLayerBeforeButton, SIGNAL(clicked()),
    this, SLOT(addLayerBefore()));
  connect(this->Ui->addLayerAfterButton, SIGNAL(clicked()),
    this, SLOT(addLayerAfter()));
  connect(this->Ui->deleteLayerButton, SIGNAL(clicked()),
    this, SLOT(deleteLayer()));

  connect(this->Ui->piecesTable, SIGNAL(cellChanged(int, int)),
          this, SLOT(tableCellChanged(int, int)));
  connect(this->Ui->piecesTable, SIGNAL(itemSelectionChanged()),
    this, SLOT(onPieceSelected()));
  connect(this->Ui->layersTable, SIGNAL(cellChanged(int, int)),
          this, SLOT(layerTableCellChanged(int, int)));
  connect(this->Ui->cutAwayViewCheckBox, SIGNAL(toggled(bool)),
          this, SLOT(onCutAwayCheckBoxToggled(bool)));
}

cmbNucPinCellEditor::~cmbNucPinCellEditor()
{
  delete this->Ui;
}

void cmbNucPinCellEditor::SetPinCell(PinCell *pincell, bool h)
{
  if(this->PinCellObject == pincell)
    {
    return;
    }
  this->PinCellObject = pincell;
  this->isHex = h;

  this->Ui->nameLineEdit->setText(pincell->name.c_str());
  this->Ui->labelLineEdit->setText(pincell->label.c_str());
  this->Ui->cutAwayViewCheckBox->setChecked(this->PinCellObject->cutaway);
  if(isHex)
  {
    this->Ui->label_7->setVisible(false);
    this->Ui->label_8->setVisible(false);
    this->Ui->pitchY->setVisible(false);
  }
  else
  {
    this->Ui->label_7->setVisible(true);
    this->Ui->label_8->setVisible(true);
    this->Ui->pitchY->setVisible(true);
  }
  this->Ui->pitchX->setText(QString::number(pincell->pitchX));
  this->Ui->pitchY->setText(QString::number(pincell->pitchY));
  //this->Ui->pitchZ->setText(QString::number(pincell->pitchZ));

  this->Ui->piecesTable->blockSignals(true);

  this->Ui->piecesTable->setColumnCount(6);
  this->Ui->piecesTable->setHorizontalHeaderLabels( QStringList() << "Segment Type"
                                                   << "Length" << "Radius (base)"
                                                   << "Radius (top)" << "Origin X"
                                                   << "Origin Y");

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
                              component.radius1(),
                              component.radius2(),
                              component.x,
                              component.y);
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

  this->onPieceSelected();
  this->UpdateData();
}

PinCell* cmbNucPinCellEditor::GetPinCell()
{
  return this->PinCellObject;
}

void cmbNucPinCellEditor::Apply()
{
  this->UpdatePinCell();
}

void cmbNucPinCellEditor::UpdatePinCell()
{
  // apply pincell attributes
  QString newName = this->Ui->nameLineEdit->text();
  newName = newName.trimmed().replace(' ', "_");
  this->Ui->nameLineEdit->setText(newName);
  QString prevName = QString(this->PinCellObject->name.c_str());
  this->PinCellObject->name = newName.toStdString();
  if(newName != prevName)
  {
    emit nameChanged(this->PinCellObject, prevName, newName);
    emit valueChange();
  }

  QString newlabel = this->Ui->labelLineEdit->text();
  newlabel = newlabel.trimmed().replace(' ', "_");
  this->Ui->labelLineEdit->setText(newlabel);
  QString prevlabel = QString(this->PinCellObject->label.c_str());
  this->PinCellObject->label = newlabel.toStdString();
  if(newlabel != prevlabel)
    {
    emit labelChanged(this->PinCellObject, prevlabel, newlabel);
    emit valueChange();
    }

  bool change = false;
  set_and_test(this->PinCellObject->pitchX, this->Ui->pitchX->text().toDouble());
  set_and_test(this->PinCellObject->pitchY, this->Ui->pitchY->text().toDouble());
  set_and_test(this->PinCellObject->pitchZ, this->Ui->pitchX->text().toDouble());
  if(change) emit valueChange();

  // update components
  double z = 0;
  for(int i = 0; i < this->Ui->piecesTable->rowCount(); i++){
    this->updateComponentObject(i, z);
  }
}

void cmbNucPinCellEditor::badLabel(QString label)
{
  this->PinCellObject->label = label.toStdString();
  this->Ui->labelLineEdit->setText(label);
}

void cmbNucPinCellEditor::badName(QString name)
{
  this->PinCellObject->name = name.toStdString();
  this->Ui->nameLineEdit->setText(name);
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
  double x = this->Ui->piecesTable->item(i, 4)->text().toDouble();
  double y = this->Ui->piecesTable->item(i, 5)->text().toDouble();
  double l = this->Ui->piecesTable->item(i, 1)->text().toDouble();
  double r1 = this->Ui->piecesTable->item(i, 2)->text().toDouble();
  double r2 = this->Ui->piecesTable->item(i, 3)->text().toDouble();
  bool change = false;

  if(obj->GetType() == CMBNUC_ASSY_CYLINDER_PIN){
      Cylinder *cylinder = dynamic_cast<Cylinder*>(obj);
      set_and_test(cylinder->x, x);
      set_and_test(cylinder->y, y);
      set_and_test(cylinder->z1, z);
      set_and_test(cylinder->z2, z + l);
      set_and_test(cylinder->r, r1);
  }
  else if(obj->GetType() == CMBNUC_ASSY_FRUSTUM_PIN){
      Frustum *frustum =  dynamic_cast<Frustum*>(obj);
      set_and_test(frustum->x, x);
      set_and_test(frustum->y, y);
      set_and_test(frustum->z1, z);
      set_and_test(frustum->z2, z + l);
      set_and_test(frustum->r1, r1);
      set_and_test(frustum->r2, r2);
  }
  z += l;
  this->Ui->piecesTable->blockSignals(false);
  if(change)
  {
    emit valueChange();
    emit resetView();
  }
}

AssyPartObj* cmbNucPinCellEditor::createComponentObject(int i, double& z)
{
  this->Ui->piecesTable->blockSignals(true);

  QTableWidgetItem *item = this->Ui->piecesTable->item(i, 0);

  QComboBox *comboBox = qobject_cast<QComboBox *>(this->Ui->piecesTable->cellWidget(i, 0));

  double x = this->Ui->piecesTable->item(i, 4)->text().toDouble();
  double y = this->Ui->piecesTable->item(i, 5)->text().toDouble();
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
  this->UpdateLayerMaterials();
  this->Ui->piecesTable->blockSignals(false);

  emit valueChange();

  return retObj;
}

void cmbNucPinCellEditor::UpdateLayerMaterials()
{
  // setup materials
  int materials = this->Ui->layersTable->rowCount();

  AssyPartObj *obj;
  QComboBox *comboBox;
  int row, nrows = this->Ui->piecesTable->rowCount();
  for(row = 0; row < nrows; ++row)
    {
    comboBox = qobject_cast<QComboBox *>
      (this->Ui->piecesTable->cellWidget(row, 0));

    if(!comboBox)
      {
      continue;
      }

    obj = static_cast<AssyPartObj*>
      (comboBox->itemData(0).value<void *>());

    if (!obj)
      {
      continue;
      }

    for(int i = 0; i < materials; i++)
      {
      comboBox = qobject_cast<QComboBox *>(this->Ui->layersTable->cellWidget(i, 0));
      if(comboBox)
        {
        if(obj->GetType() == CMBNUC_ASSY_FRUSTUM_PIN)
          {
          dynamic_cast<Frustum*>(obj)->SetMaterial(i, comboBox->currentText().toStdString());
          }
        else if(obj->GetType() == CMBNUC_ASSY_CYLINDER_PIN)
          {
          dynamic_cast<Cylinder*>(obj)->SetMaterial(i, comboBox->currentText().toStdString());
          }
        }
      }
    }
  emit valueChange();
}

void cmbNucPinCellEditor::UpdateData()
{
  bool cutaway = this->Ui->cutAwayViewCheckBox->isChecked();
  this->PinCellObject->cutaway = cutaway;
  this->PinCellObject->CachedData.TakeReference(
    cmbNucAssembly::CreatePinCellMultiBlock(this->PinCellObject, cutaway));
  emit this->pincellModified(this->PinCellObject);
  emit resetView();
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

  this->createComponentItem(row, default_length, default_radius, default_radius, 0, 0);

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
  this->UpdateData();
}

void cmbNucPinCellEditor::createComponentItem( int row, double default_length,
                                               double default_radius1,
                                               double default_radius2,
                                               double x, double y)
{
  this->Ui->piecesTable->blockSignals(true);
  // type

  QComboBox *comboBox = new QComboBox;
  comboBox->addItem("Cylinder");
  comboBox->addItem("Frustum");

  connect(comboBox, SIGNAL(currentIndexChanged(QString)),
          this, SLOT(sectionTypeComboBoxChanged(QString)));
  this->Ui->piecesTable->setCellWidget(row, 0, comboBox);
  QTableWidgetItem *item = new SegmentRadiusEditor;
  this->Ui->piecesTable->setItem(row, 0, item);

  // length
  item = new SegmentRadiusEditor;
  item->setText(QString::number(default_length));
  this->Ui->piecesTable->setItem(row, 1, item);

  // radius (base)
  item = new SegmentRadiusEditor;
  item->setText(QString::number(default_radius1));
  this->Ui->piecesTable->setItem(row, 2, item);

  // radius (top)
  item = new SegmentRadiusEditor;
  item->setText(QString::number(default_radius2));
  this->Ui->piecesTable->setItem(row, 3, item);

  // Origin X
  item = new SegmentRadiusEditor;
  item->setText(QString::number(x));
  this->Ui->piecesTable->setItem(row, 4, item);

  // Origin Y
  item = new SegmentRadiusEditor;
  item->setText(QString::number(y));
  this->Ui->piecesTable->setItem(row, 5, item);

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
  this->UpdateData();
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
  this->UpdateData();
}

void cmbNucPinCellEditor::numberOfLayersChanged(int layers)
{
  this->Ui->layersTable->blockSignals(true);
  int current = this->Ui->layersTable->rowCount();
  this->Ui->layersTable->setRowCount(layers);
  this->PinCellObject->SetNumberOfLayers(layers);

  // new rows
  for(int row = current; row < layers; row++)
    {
    QComboBox *comboBox = new QComboBox;
    this->setupMaterialComboBox(comboBox);
    this->Ui->layersTable->setCellWidget(row, 0, comboBox);
    QObject::connect(comboBox, SIGNAL(currentIndexChanged(int)),
      this, SLOT(onUpdateLayerMaterial()));

    QTableWidgetItem *item = new LayerRadiusEditor;
    item->setText(QString::number(1.0));
    this->Ui->layersTable->setItem(row, 1, item);
    this->PinCellObject->SetMaterial(row,
      comboBox->currentText().toStdString());
    }
  this->Ui->layersTable->blockSignals(false);
  this->UpdateData();
}

void cmbNucPinCellEditor::sectionTypeComboBoxChanged(const QString &type)
{
  QComboBox *comboBox = qobject_cast<QComboBox*>(sender());
  if(!comboBox){
    return;
  }

  AssyPartObj* obj =
    static_cast<AssyPartObj*>(comboBox->itemData(0).value<void*>());
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
    double z = (obj->GetType() == CMBNUC_ASSY_FRUSTUM_PIN) ?
      dynamic_cast<Frustum*>(obj)->z1 :
    dynamic_cast<Cylinder*>(obj)->z1;
    this->PinCellObject->RemoveSection(obj);
    AssyPartObj* objPart = this->createComponentObject(row, z);
    this->UpdateLayerMaterials();
    this->UpdateData();
    }
}

void cmbNucPinCellEditor::setupMaterialComboBox(QComboBox *comboBox)
{
  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  //TODO COLOR
  //comboBox->addItems(matColorMap->MaterialColorMap().keys());
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
  this->UpdateData();
}

void cmbNucPinCellEditor::onUpdateLayerMaterial()
{
  this->UpdateLayerMaterials();
  this->UpdateData();
}

void cmbNucPinCellEditor::onPieceSelected()
{
  AssyPartObj* obj = this->getSelectedPiece();
  bool pieceSelected = (obj != NULL);
  this->Ui->layersTable->setEnabled(pieceSelected);
  if(!pieceSelected)
    {
    this->Ui->layersTable->clear();
    this->Ui->layersTable->setRowCount(0);
    return;
    }
  PinCell *pincell = this->PinCellObject;
  int layers = pincell->GetNumberOfLayers();
  if(layers < 1)
    {
    this->Ui->layersTable->clear();
    this->Ui->layersTable->setRowCount(0);
    this->numberOfLayersChanged(1);
    }
  else
    {
    this->Ui->layersTable->blockSignals(true);
    this->Ui->layersTable->setRowCount(layers);
    for(int i = 0; i < layers; i++)
      {
      QComboBox *comboBox = new QComboBox;
      this->setupMaterialComboBox(comboBox);
      this->Ui->layersTable->setCellWidget(i, 0, comboBox);
      this->Ui->layersTable->setItem(i, 0, new LayerRadiusEditor);

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
      QTableWidgetItem *item = new LayerRadiusEditor;
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
  if(!comboBox)
    {
    return NULL;
    }

  AssyPartObj* obj =
    static_cast<AssyPartObj*>(comboBox->itemData(0).value<void *>());
  return obj;
}

//-----------------------------------------------------------------------------
void cmbNucPinCellEditor::onCutAwayCheckBoxToggled(bool state)
{
  this->UpdateData();
}

//-----------------------------------------------------------------------------
void cmbNucPinCellEditor::addLayerBefore()
{
  int row;
  double radius;
  if((this->Ui->layersTable->selectedItems().count() == 0) ||
     (this->Ui->layersTable->selectedItems().value(0)->row() == 0))
    {
    row = 0;
    radius = 0.5 * this->PinCellObject->Radius(0);
    }
  else
    {
    QTableWidgetItem* selItem = this->Ui->layersTable->selectedItems().value(0);
    row = selItem->row();
    radius = 0.5 * (this->PinCellObject->Radius(row) + this->PinCellObject->Radius(row-1));
    }

  this->Ui->layersTable->blockSignals(true);
  this->Ui->layersTable->insertRow(row);
  QComboBox* comboBox = new QComboBox;
  this->setupMaterialComboBox(comboBox);
  this->Ui->layersTable->setCellWidget(row, 0, comboBox);
  QObject::connect(comboBox, SIGNAL(currentIndexChanged(int)),
      this, SLOT(onUpdateLayerMaterial()));

  QTableWidgetItem* item = new LayerRadiusEditor;
  item->setText(QString::number(radius));
  this->Ui->layersTable->setItem(row, 1, item);

  item = new LayerRadiusEditor;
  item->setText(QString::number(0.0));
  this->Ui->layersTable->setItem(row, 4, item);

  item = new LayerRadiusEditor;
  item->setText(QString::number(0.0));
  this->Ui->layersTable->setItem(row, 5, item);

  this->Ui->layersTable->blockSignals(false);
  this->rebuildLayersFromTable();
  this->UpdateData();
}

//-----------------------------------------------------------------------------
void cmbNucPinCellEditor::addLayerAfter()
{
  int row;
  double radius;
  this->Ui->layersTable->blockSignals(true);

  // If we are appending to the outer-most layer then the new layer is radius
  // 1 and the original outer most layer is between it and the previous
  if((this->Ui->layersTable->selectedItems().count() == 0) ||
     (this->Ui->layersTable->selectedItems().value(0)->row() ==
      (this->Ui->layersTable->rowCount()-1)))
    {
    row = this->Ui->layersTable->rowCount();
    if (row == 1)
      {
      // there was only 1 layer so the original layer is now at 0.5
      this->Ui->layersTable->item(0, 1)->setText("0.5");
      }
    else
      {
      radius = 0.5 * (this->PinCellObject->Radius(row-1) +
                      this->PinCellObject->Radius(row-2));
      this->Ui->layersTable->item(row-1, 1)->setText(QString::number(radius));
      }
    radius = 1.0;
    }
  else
    {
    QTableWidgetItem* selItem = this->Ui->layersTable->selectedItems().value(0);
    row = selItem->row() + 1;
    radius = 0.5 * (this->PinCellObject->Radius(row-1) +
                    this->PinCellObject->Radius(row));
    }

  this->Ui->layersTable->insertRow(row);
  QComboBox* comboBox = new QComboBox;
  this->setupMaterialComboBox(comboBox);
  this->Ui->layersTable->setCellWidget(row, 0, comboBox);
  QObject::connect(comboBox, SIGNAL(currentIndexChanged(int)),
      this, SLOT(onUpdateLayerMaterial()));

  QTableWidgetItem* item = new LayerRadiusEditor;
  item->setText(QString::number(radius));
  this->Ui->layersTable->setItem(row, 1, item);

  item = new LayerRadiusEditor;
  item->setText(QString::number(0.0));
  this->Ui->layersTable->setItem(row, 4, item);

  item = new LayerRadiusEditor;
  item->setText(QString::number(0.0));
  this->Ui->layersTable->setItem(row, 5, item);

  this->Ui->layersTable->blockSignals(false);
  this->rebuildLayersFromTable();
  this->UpdateData();
}

//-----------------------------------------------------------------------------
void cmbNucPinCellEditor::deleteLayer()
{
  // If no layer is selected or if there is only 1 layer do nothing
  if((this->Ui->layersTable->selectedItems().count() == 0) ||
     (this->Ui->layersTable->rowCount() == 1))
    {
    return; // if no layer is selected, don't delete any
    }
  QTableWidgetItem* selItem = this->Ui->layersTable->selectedItems().value(0);
  int row = selItem->row();
  this->Ui->layersTable->blockSignals(true);

  // If the outer layer is being removed we need to extend the previous layer
  // to go to 1.0
  if (row == (this->Ui->layersTable->rowCount() - 1))
    {
      this->Ui->layersTable->item(row-1, 1)->setText("1.0");
    }
  this->Ui->layersTable->removeRow(selItem->row());
  this->Ui->layersTable->blockSignals(false);
  this->rebuildLayersFromTable();
  this->UpdateData();
}

//-----------------------------------------------------------------------------
void cmbNucPinCellEditor::rebuildLayersFromTable()
{
  int layers = this->Ui->layersTable->rowCount();
  this->PinCellObject->SetNumberOfLayers(layers);

  for(int layer = 0; layer < layers; layer++)
    {
    QComboBox* mat = qobject_cast<QComboBox*>(
      this->Ui->layersTable->cellWidget(layer, 0));

    QTableWidgetItem* item = this->Ui->layersTable->item(layer, 1);
    this->PinCellObject->SetMaterial(layer, mat->currentText().toStdString());
    this->PinCellObject->SetRadius(layer, item->text().toDouble());
    }
}
