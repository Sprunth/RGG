
#include "cmbNucPinCellEditor.h"

#include <QComboBox>
#include <QTableWidgetItem>
#include <QObject>
#include <QDebug>

#include <vtkRenderWindow.h>
#include "vtkCompositeDataDisplayAttributes.h"

#include "cmbNucPartDefinition.h"
#include "cmbNucPinCell.h"
#include "cmbNucAssembly.h"
#include "cmbNucMaterialColors.h"
#include "cmbNucDefaults.h"

#define set_and_test(X,Y) \
change |= (Y) != X;\
X = (Y);

class PinSegmentItem : public QTableWidgetItem
{
public:
  PinSegmentItem(PinSubPart * ps)
  :SubPart(ps)
  {}
  bool valid(const QVariant& value, double & v)
  {
    bool ok;
    v = value.toDouble(&ok);

    // Make sure value is positive
    if (!ok || v < 0.)
    {
      return false;
    }
    return true;
  }
  virtual ~PinSegmentItem() {};
  PinSubPart * SubPart;
  virtual void initialize() = 0;
};

// We use this class to validate the input to the radius fields for segments
class SegmentRadiusItem : public PinSegmentItem
{
public:
  SegmentRadiusItem(PinSubPart * ps, PinSubPart::End end)
  :PinSegmentItem(ps),End(end)
  {
    this->initialize();
  }
  virtual void setData(int role, const QVariant& value)
  {
    double tmpv;
    if (this->tableWidget() != NULL && role == Qt::EditRole)
    {
      if(!valid(value,tmpv)) return;
      if(tmpv != SubPart->getRadius(End))
      {
        SubPart->setRadius(End, tmpv);
        this->checkAndSetNeighbors();
      }
    }
    QTableWidgetItem::setData(role, value);
  }

  void checkAndSetNeighbors()
  {
    double d = SubPart->getRadius(End);
    SegmentRadiusItem * current = this;
    //send before
    while(current)
    {
      int c = (current->End+1)%2;
      int row = current->row() - c;
      if( (row != current->row()) || (current->SubPart->GetType() == CMBNUC_ASSY_CYLINDER_PIN))
        current = current->send(row, 2+c, d);
      else
        current = NULL;
    }
    //send after
    current = this;
    while(current)
    {
      int c = (current->End+1)%2;
      int row = current->row() + current->End;
      if( (row != current->row()) || (current->SubPart->GetType() == CMBNUC_ASSY_CYLINDER_PIN))
        current = current->send(row, 2+c, d);
      else
        current = NULL;
    }
  }

  virtual void initialize()
  {
    this->setText(QString::number(SubPart->getRadius(End)));
  }
private:
  PinSubPart::End End;
  inline SegmentRadiusItem * send(int row, int col, double d)
  {
    if(row < 0) return NULL;
    if(row >= this->tableWidget()->rowCount()) return NULL;
    SegmentRadiusItem * seg = dynamic_cast<SegmentRadiusItem*>(this->tableWidget()->item(row, col));
    seg->setText(QString::number(d));
    seg->SubPart->setRadius(seg->End, d);
    seg->initialize();
    return seg;
  }
};

class SegmentOtherValuesItem : public PinSegmentItem
{
public:
  SegmentOtherValuesItem(PinSubPart * ps)
  : PinSegmentItem(ps)
  {  }
  virtual void setData(int role, const QVariant& value)
  {
    double tmpv;
    if (this->tableWidget() != NULL && role == Qt::EditRole )
    {
      if(!valid(value,tmpv)) return;
    }
    QTableWidgetItem::setData(role, value);
  }
  virtual void initialize()
  {
    //For now taken care of by Editor
  }
private:
  double *X;

};


// We use this class to validate the input to the radius fields for layers
class LayerRadiusEditor : public QTableWidgetItem
{
public:
  LayerRadiusEditor(PinCell *pc)
  :Pin(pc)
  {}
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
      else if(this->row() == this->tableWidget()->rowCount() - 1 && dval != 1)
        {
        return;
        }
      Pin->SetRadius(this->row(), dval);
      }
    QTableWidgetItem::setData(role, value);
    }
  void refresh()
  {
    this->setText(QString::number(Pin->Radius(this->row())));
  }

  PinCell* Pin;
};

bool sort_by_z1(const PinSubPart * a, const PinSubPart * b)
{
  return a->z1 < b->z1;
}

cmbNucPinCellEditor::cmbNucPinCellEditor(QWidget *parent)
  : QWidget(parent),
    Ui(new Ui::cmbNucPinCellEditor),
    AssemblyObject(0)
{
  isHex = false;
  InternalPinCell = new PinCell(0,0);
  ExternalPinCell = NULL;
  this->Ui->setupUi(this);

  this->Ui->layersTable->setRowCount(0);
  this->Ui->layersTable->setColumnCount(2);
  this->Ui->layersTable->setHorizontalHeaderLabels( QStringList() << "Material"
                                                    << "Radius\n(normalized)");
  this->Ui->layersTable->horizontalHeader()->setStretchLastSection(true);

  connect(this->Ui->addButton, SIGNAL(clicked()), this, SLOT(addComponent()));
  connect(this->Ui->deleteButton, SIGNAL(clicked()), this, SLOT(deleteComponent()));

  connect(this->Ui->addLayerBeforeButton, SIGNAL(clicked()),
    this, SLOT(addLayerBefore()));
  connect(this->Ui->addLayerAfterButton, SIGNAL(clicked()),
    this, SLOT(addLayerAfter()));
  connect(this->Ui->deleteLayerButton, SIGNAL(clicked()),
    this, SLOT(deleteLayer()));

  connect(this->Ui->piecesTable, SIGNAL(cellChanged(int, int)),
          this, SLOT(tableCellChanged()));
  connect(this->Ui->piecesTable, SIGNAL(itemSelectionChanged()),
    this, SLOT(onPieceSelected()));
  connect(this->Ui->layersTable, SIGNAL(cellChanged(int, int)),
          this, SLOT(UpdateData()));
  connect(this->Ui->cutAwayViewCheckBox, SIGNAL(toggled(bool)),
          this, SLOT(UpdateData()));

  connect(this->Ui->CalculatePitch, SIGNAL(clicked()),
          this, SLOT(calculatePitch()));

  connect(this->Ui->CellMaterial, SIGNAL(currentIndexChanged(const QString &)),
          this,                   SLOT(onUpdateCellMaterial(const QString &)));
}

cmbNucPinCellEditor::~cmbNucPinCellEditor()
{
  delete this->Ui;
}

void cmbNucPinCellEditor::SetPinCell(PinCell *pc, bool h)
{
  if(this->ExternalPinCell == pc)
    {
    return;
    }
  this->ExternalPinCell = pc;
  this->isHex = h;
  this->Reset();

}

void cmbNucPinCellEditor::Reset()
{
  this->InternalPinCell->fill(this->ExternalPinCell);


  this->Ui->CellMaterial->blockSignals(true);
  this->setupMaterialComboBox(this->Ui->CellMaterial);
  this->Ui->CellMaterial->blockSignals(false);

  cmbNucMaterialColors::instance()->selectIndex(this->Ui->CellMaterial,
                                                this->InternalPinCell->getCellMaterial());

  this->Ui->nameLineEdit->setText(this->InternalPinCell->name.c_str());
  this->Ui->labelLineEdit->setText(this->InternalPinCell->label.c_str());
  this->Ui->cutAwayViewCheckBox->setChecked(this->InternalPinCell->cutaway);
  this->Ui->label_7->setVisible(!isHex);
  this->Ui->label_8->setVisible(!isHex);
  this->Ui->pitchY->setVisible(!isHex);

  this->Ui->pitchX->setText(QString::number(this->InternalPinCell->pitchX));
  this->Ui->pitchY->setText(QString::number(this->InternalPinCell->pitchY));

  this->Ui->piecesTable->blockSignals(true);

  this->Ui->piecesTable->setColumnCount(6);
  this->Ui->piecesTable->setHorizontalHeaderLabels( QStringList() << "Segment\nType"
                                                   << "Length" << "Base\nRadius"
                                                   << "Top\nRadius" << "Origin\nX"
                                                   << "Origin\nY");

  std::vector<PinSubPart *> components;
  for(size_t i = 0; i < this->InternalPinCell->NumberOfCylinders(); i++){
    components.push_back(this->InternalPinCell->GetCylinder(i));
  }
  for(size_t i = 0; i < this->InternalPinCell->NumberOfFrustums(); i++){
    components.push_back(this->InternalPinCell->GetFrustum(i));
  }
  std::sort(components.begin(), components.end(), sort_by_z1);

  this->Ui->piecesTable->setRowCount(components.size());
  for(size_t i = 0; i < components.size(); i++)
    {
    PinSubPart *component = components[i];
    this->createComponentItem(i, component);
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
  return this->ExternalPinCell;
}

void cmbNucPinCellEditor::Apply()
{
  this->UpdatePinCell();
  bool change;
  if(this->ExternalPinCell->fill(this->InternalPinCell))
  {
    change = true;
  }

  QString newName = this->Ui->nameLineEdit->text();
  newName = newName.trimmed().replace(' ', "_");
  this->Ui->nameLineEdit->setText(newName);

  QString prevName = QString(this->ExternalPinCell->name.c_str());
  this->ExternalPinCell->name = newName.toStdString();
  if(newName != prevName)
  {
    emit nameChanged(this->ExternalPinCell, prevName, newName);
    change |= this->ExternalPinCell->name != prevName.toStdString();
  }

  QString newlabel = this->Ui->labelLineEdit->text();
  newlabel = newlabel.trimmed().replace(' ', "_");
  this->Ui->labelLineEdit->setText(newlabel);
  QString prevlabel = QString(this->ExternalPinCell->label.c_str());
  this->ExternalPinCell->label = newlabel.toStdString();
  if(newlabel != prevlabel)
  {
    emit labelChanged(this->ExternalPinCell, prevlabel, newlabel);
    change |= this->ExternalPinCell->label != prevlabel.toStdString();
  }

  set_and_test(this->ExternalPinCell->pitchX, this->Ui->pitchX->text().toDouble());
  set_and_test(this->ExternalPinCell->pitchY, this->Ui->pitchY->text().toDouble());
  this->ExternalPinCell->pitchZ = 0;

  this->Reset();
  this->ExternalPinCell->CachedData.TakeReference(
        cmbNucAssembly::CreatePinCellMultiBlock(this->ExternalPinCell, this->isHex, false));
  if(change) emit valueChange();
}

void cmbNucPinCellEditor::UpdatePinCell()
{
  // update components
  double z = 0;
  this->Ui->piecesTable->blockSignals(true);
  for(int i = 0; i < this->Ui->piecesTable->rowCount(); i++)
  {
    QTableWidgetItem *item = this->Ui->piecesTable->item(i, 0);
    QComboBox *comboBox = qobject_cast<QComboBox *>(this->Ui->piecesTable->cellWidget(i, 0));
    PinSubPart* obj = static_cast<PinSubPart*>(comboBox->itemData(0).value<void *>());
    if(!obj)
    {
      continue;
    }
    double x = this->Ui->piecesTable->item(i, 4)->text().toDouble();
    double y = this->Ui->piecesTable->item(i, 5)->text().toDouble();
    double l = this->Ui->piecesTable->item(i, 1)->text().toDouble();
    //Rows are set automatically
    bool change = false;
    set_and_test(obj->x, x);
    set_and_test(obj->y, y);
    set_and_test(obj->z1, z);
    set_and_test(obj->z2, z + l);

    z += l;
    this->Ui->piecesTable->blockSignals(false);
    if(change)
    {
      emit resetView();
    }
  }
  this->Ui->piecesTable->blockSignals(false);
}

void cmbNucPinCellEditor::badLabel(QString label)
{
  this->ExternalPinCell->label = label.toStdString();
  this->Ui->labelLineEdit->setText(label);
}

void cmbNucPinCellEditor::badName(QString name)
{
  this->ExternalPinCell->name = name.toStdString();
  this->Ui->nameLineEdit->setText(name);
}

PinSubPart* cmbNucPinCellEditor::createComponentObject(int r, PinSubPart * before)
{
  if(before == NULL)
  {
    return NULL;
  }
  this->Ui->piecesTable->blockSignals(true);

  QTableWidgetItem *item = this->Ui->piecesTable->item(r, 0);

  QComboBox *comboBox = qobject_cast<QComboBox *>(this->Ui->piecesTable->cellWidget(r, 0));

  PinSubPart* retObj = NULL;
  if(comboBox->currentText() == "Cylinder")
  {
    retObj = new Cylinder(before);
  }
  else if(comboBox->currentText() == "Frustum")
  {
    retObj = new Frustum(before);
  }
  if(retObj == NULL) return NULL;
  QVariant vdata;
  vdata.setValue((void*)(retObj));
  comboBox->setItemData(0, vdata);
  vdata.setValue(r); // row
  comboBox->setItemData(1, vdata);

  for(unsigned int c = 1; c < 6; ++c)//there are 6 columns
  {
    PinSegmentItem * item = dynamic_cast< PinSegmentItem * >(this->Ui->piecesTable->item(r,c));
    item->SubPart = retObj;
    item->initialize();
  }

  int start = (r>0)?r - 1:0;
  (dynamic_cast< SegmentRadiusItem * >(this->Ui->piecesTable->item(start,3)))->checkAndSetNeighbors();

  this->Ui->piecesTable->blockSignals(false);

  return retObj;
}

void cmbNucPinCellEditor::UpdateData()
{
  bool cutaway = this->Ui->cutAwayViewCheckBox->isChecked();
  this->InternalPinCell->cutaway = cutaway;
  this->InternalPinCell->CachedData.TakeReference(
    cmbNucAssembly::CreatePinCellMultiBlock(this->InternalPinCell, this->isHex, cutaway));
  emit this->pincellModified(this->InternalPinCell);
  emit resetView();
}

void cmbNucPinCellEditor::addComponent()
{
  int row = this->Ui->piecesTable->rowCount();
  this->Ui->piecesTable->setRowCount(row + 1);

  PinSubPart * newObj = NULL;

  if(row >= 1)
  {
    PinSubPart * previous =
         (dynamic_cast<SegmentRadiusItem*>(this->Ui->piecesTable->item(row - 1, 3)))->SubPart;
    newObj = previous->clone();
    newObj->z1 = previous->z2;
    newObj->z2 = newObj->z1 + previous->length();
    newObj->reverseRadii();
  }
  else
  {
    double r; double h;
    this->AssemblyObject->calculateRadius(r);
    if(!this->AssemblyObject->getDefaults()->getHeight(h))
    {
      h = this->AssemblyObject->AssyDuct.getLength();
      if(h < 0) h = 10;
    }
    newObj = new Cylinder(r, 0, h);
  }

  this->createComponentItem(row, newObj);
  this->InternalPinCell->AddPart(newObj);

  // Select this row
  QTableWidgetItem* selItem = this->Ui->piecesTable->item(row, 0);
  this->Ui->piecesTable->setCurrentItem(selItem);
  selItem->setSelected(true);

  // update view
  this->UpdateData();
}

void cmbNucPinCellEditor::createComponentItem( int row, PinSubPart* obj)
{
  this->Ui->piecesTable->blockSignals(true);
  // type

  QComboBox *comboBox = new QComboBox;
  comboBox->addItem("Cylinder");
  comboBox->addItem("Frustum");
  QVariant vdata;
  vdata.setValue((void*)(obj));
  comboBox->setItemData(0, vdata);
  vdata.setValue(row);
  comboBox->setItemData(1, vdata);
  if(obj->GetType() == CMBNUC_ASSY_FRUSTUM_PIN)
  {
    comboBox->blockSignals(true);
    comboBox->setCurrentIndex(1);
    comboBox->blockSignals(false);
  }

  connect(comboBox, SIGNAL(currentIndexChanged(QString)),
          this, SLOT(sectionTypeComboBoxChanged(QString)));
  this->Ui->piecesTable->setCellWidget(row, 0, comboBox);
  QTableWidgetItem *item = new QTableWidgetItem;
  this->Ui->piecesTable->setItem(row, 0, item);

  // length
  item = new SegmentOtherValuesItem(obj);
  item->setText(QString::number(obj->length()));
  this->Ui->piecesTable->setItem(row, 1, item);

  // radius (base)
  item = new SegmentRadiusItem(obj, PinSubPart::BOTTOM);
  this->Ui->piecesTable->setItem(row, 2, item);

  // radius (top)
  item = new SegmentRadiusItem(obj, PinSubPart::TOP);
  this->Ui->piecesTable->setItem(row, 3, item);

  // Origin X
  item = new SegmentOtherValuesItem(obj);
  item->setText(QString::number(obj->x));
  this->Ui->piecesTable->setItem(row, 4, item);

  // Origin Y
  item = new SegmentOtherValuesItem(obj);
  item->setText(QString::number(obj->y));
  this->Ui->piecesTable->setItem(row, 5, item);

  this->Ui->piecesTable->blockSignals(false);
}

void cmbNucPinCellEditor::deleteComponent()
{
  this->Ui->piecesTable->blockSignals(true);
  PinSubPart* obj = this->getSelectedPiece();
  this->InternalPinCell->RemoveSection(obj);
  int row = this->Ui->piecesTable->currentRow();
  this->Ui->piecesTable->removeRow(row);

  int start = (row>0)?row - 1:0;

  if(this->Ui->layersTable->rowCount()!=0)
  {
    (dynamic_cast< SegmentRadiusItem * >(this->Ui->piecesTable->item(start,3)))->checkAndSetNeighbors();
  }

  this->Ui->piecesTable->blockSignals(false);

  // update view
  this->tableCellChanged();
}

void cmbNucPinCellEditor::tableCellChanged()
{
  // update pin cell and render view
  this->UpdatePinCell();
  this->UpdateData();
}

void cmbNucPinCellEditor::sectionTypeComboBoxChanged(const QString &type)
{
  QComboBox *comboBox = qobject_cast<QComboBox*>(sender());
  if(!comboBox){
    return;
  }

  PinSubPart* obj =
    static_cast<PinSubPart*>(comboBox->itemData(0).value<void*>());
  if(!obj)
    {
    return;
    }

  bool ok;
  int row = comboBox->itemData(1).toInt(&ok);
  if(ok)
    {
    // Add the new component
    PinSubPart* objPart = this->createComponentObject(row, obj);
    if(objPart!=NULL)
    {
      this->InternalPinCell->RemoveSection(obj);
      this->InternalPinCell->AddPart(objPart);
    }

    this->UpdateData();
    }
}

void cmbNucPinCellEditor::setupMaterialComboBox(QComboBox *comboBox)
{
  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  matColorMap->setUp(comboBox);
}

void cmbNucPinCellEditor::onUpdateLayerMaterial()
{
  // setup materials
  QComboBox *comboBox;
  for(unsigned int i = 0; i < this->Ui->layersTable->rowCount(); ++i)
  {
    comboBox = qobject_cast<QComboBox *>(this->Ui->layersTable->cellWidget(i, 0));
    if(comboBox)
    {
      QPointer<cmbNucMaterial> mat =
         cmbNucMaterialColors::instance()->getMaterial(comboBox);
      this->InternalPinCell->SetMaterial(i, mat);
    }
  }
  this->UpdateData();
}

void cmbNucPinCellEditor::onUpdateCellMaterial( const QString & material )
{
  QPointer<cmbNucMaterial> mat =
      cmbNucMaterialColors::instance()->getMaterialByName(material);
  this->InternalPinCell->setCellMaterial(mat);
  this->UpdateData();
}

void cmbNucPinCellEditor::onPieceSelected()
{
  PinSubPart* obj = this->getSelectedPiece();
  bool pieceSelected = (obj != NULL);
  this->Ui->layersTable->setEnabled(pieceSelected);
  this->Ui->addLayerBeforeButton->setEnabled(pieceSelected);
  this->Ui->addLayerAfterButton->setEnabled(pieceSelected);
  this->Ui->deleteLayerButton->setEnabled(pieceSelected);
  if(!pieceSelected)
    {
    this->Ui->layersTable->clearContents();
    this->Ui->layersTable->setRowCount(0);
    return;
    }
  PinCell *pincell = this->InternalPinCell;
  int layers = pincell->GetNumberOfLayers();
  if(layers < 1)
    {
    pincell->SetNumberOfLayers(1);
    layers = 1;
    }
  this->Ui->layersTable->blockSignals(true);
  this->Ui->layersTable->setRowCount(layers);
  for(int i = 0; i < layers; i++)
    {
    this->createMaterialRow(i, obj);
    }
    this->Ui->layersTable->blockSignals(false);
}
//-----------------------------------------------------------------------------
PinSubPart *cmbNucPinCellEditor::getSelectedPiece()
{
  if(this->Ui->piecesTable->selectedItems().count()==0)
    {
    return NULL;
    }
  QTableWidgetItem * rad = this->Ui->piecesTable->selectedItems().value(1);
  PinSegmentItem* selItem = dynamic_cast<PinSegmentItem*>(rad);
  if(!selItem)
    {
    return NULL;
    }
  return selItem->SubPart;
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
    }
  else
    {
    QTableWidgetItem* selItem = this->Ui->layersTable->selectedItems().value(0);
    row = selItem->row();
    }
  this->Ui->layersTable->insertRow(row);
  this->InternalPinCell->InsertLayer(row);

  this->Ui->layersTable->blockSignals(true);

  this->createMaterialRow(row, InternalPinCell->GetPart(0));

  this->Ui->layersTable->blockSignals(false);
  this->UpdateData();
}

//-----------------------------------------------------------------------------

void cmbNucPinCellEditor::createMaterialRow(int row, PinSubPart * obj)
{
  QComboBox *comboBox = new QComboBox;
  this->setupMaterialComboBox(comboBox);
  this->Ui->layersTable->setCellWidget(row, 0, comboBox);
  this->Ui->layersTable->setItem(row, 0, new QTableWidgetItem());

  QPointer<cmbNucMaterial> selMat;
  selMat = obj->GetMaterial(row);
  cmbNucMaterialColors::instance()->selectIndex(comboBox, selMat);

  QTableWidgetItem *item = new LayerRadiusEditor(this->InternalPinCell);
  item->setText(QString::number(this->InternalPinCell->Radius(row)));
  this->Ui->layersTable->setItem(row, 1, item);

  QObject::connect(comboBox, SIGNAL(currentIndexChanged(int)),
                   this, SLOT(onUpdateLayerMaterial()));
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
    }
  else
    {
    QTableWidgetItem* selItem = this->Ui->layersTable->selectedItems().value(0);
    row = selItem->row() + 1;
    }
  this->Ui->layersTable->insertRow(row);
  this->InternalPinCell->InsertLayer(row);
  dynamic_cast<LayerRadiusEditor*>(this->Ui->layersTable->item(row-1,1))->refresh();

  this->createMaterialRow(row, InternalPinCell->GetPart(0));

  this->Ui->layersTable->blockSignals(false);
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
  this->InternalPinCell->DeleteLayer(row);
  this->Ui->layersTable->removeRow(row);
  if(row == this->Ui->layersTable->rowCount())
  {
    dynamic_cast<LayerRadiusEditor*>(this->Ui->layersTable->item(row-1,1))->refresh();
  }
  this->Ui->layersTable->blockSignals(false);
  this->UpdateData();
}

//-----------------------------------------------------------------------------
void cmbNucPinCellEditor::rebuildLayersFromTable()
{
  int layers = this->Ui->layersTable->rowCount();
  this->InternalPinCell->SetNumberOfLayers(layers);

  for(int layer = 0; layer < layers; layer++)
    {
    QComboBox* mat = qobject_cast<QComboBox*>(
      this->Ui->layersTable->cellWidget(layer, 0));
    QPointer<cmbNucMaterial> matPtr =
        cmbNucMaterialColors::instance()->getMaterial(mat);
    QTableWidgetItem* item = this->Ui->layersTable->item(layer, 1);
    this->InternalPinCell->SetMaterial(layer, matPtr);
    this->InternalPinCell->SetRadius(layer, item->text().toDouble());
    }
}

//-----------------------------------------------------------------------------
void cmbNucPinCellEditor::calculatePitch()
{
  if(this->AssemblyObject != NULL)
  {
    double x, y;
    this->AssemblyObject->calculatePitch(x,y);
    this->Ui->pitchX->setText(QString::number(x));
    this->Ui->pitchY->setText(QString::number(y));
  }
}

//-----------------------------------------------------------------------------
void cmbNucPinCellEditor::SetAssembly(cmbNucAssembly *assembly)
{
  if(assembly)
  {
    this->Ui->pitchX->setEnabled(!assembly->isPinsAutoCentered());
    this->Ui->pitchY->setEnabled(!assembly->isPinsAutoCentered());
    this->Ui->CalculatePitch->setEnabled(!assembly->isPinsAutoCentered());
  }
  this->AssemblyObject = assembly;
}
