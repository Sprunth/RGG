
#include "cmbNucPinCellEditor.h"

#include <QComboBox>
#include <QTableWidgetItem>
#include <QObject>
#include <QDebug>
#include <QObjectList>

#include <vtkRenderWindow.h>
#include "vtkCompositeDataDisplayAttributes.h"

#include "cmbNucPartDefinition.h"
#include "cmbNucPinCell.h"
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
    double td = SubPart->getRadius(End);
    SegmentRadiusItem * current = this;
    //send before
    while(current)
    {
      int c = (current->End+1)%2;
      int tmprow = current->row() - c;
      if( (tmprow != current->row()) || (current->SubPart->GetType() == CMBNUC_ASSY_CYLINDER_PIN))
        current = current->send(tmprow, 2+c, td);
      else
        current = NULL;
    }
    //send after
    current = this;
    while(current)
    {
      int c = (current->End+1)%2;
      int tmprow = current->row() + current->End;
      if( (tmprow != current->row()) || (current->SubPart->GetType() == CMBNUC_ASSY_CYLINDER_PIN))
        current = current->send(tmprow, 2+c, td);
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
  inline SegmentRadiusItem * send(int rowin, int colin, double din)
  {
    if(rowin < 0) return NULL;
    if(rowin >= this->tableWidget()->rowCount()) return NULL;
    SegmentRadiusItem * seg = dynamic_cast<SegmentRadiusItem*>(this->tableWidget()->item(rowin, colin));
    seg->setText(QString::number(din));
    seg->SubPart->setRadius(seg->End, din);
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
      if(this->row() < Pin->GetNumberOfLayers())
        Pin->SetRadius(this->row(), dval);
      else return;
      }
    QTableWidgetItem::setData(role, value);
    }
  void refresh()
  {
    this->setText(QString::number(Pin->Radius(this->row())));
  }

  PinCell* Pin;
};

cmbNucPinCellEditor::cmbNucPinCellEditor(QWidget *p)
  : QWidget(p),
    Ui(new Ui::cmbNucPinCellEditor)
{
  isHex = false;
  InternalPinCell = new PinCell;
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

  connect(this->Ui->CellMaterial, SIGNAL(currentIndexChanged(const QString &)),
          this,                   SLOT(onUpdateCellMaterial(const QString &)));

  this->setButtons();
}

cmbNucPinCellEditor::~cmbNucPinCellEditor()
{
  delete this->Ui;
  delete this->InternalPinCell;
  this->ExternalPinCell = NULL;
}

bool cmbNucPinCellEditor::isCrossSectioned()
{
  return this->Ui->cutAwayViewCheckBox->isChecked();
}

void cmbNucPinCellEditor::setButtons()
{
  int rc = this->Ui->piecesTable->rowCount();
  this->Ui->deleteButton->setEnabled(rc > 1);
  rc = this->Ui->layersTable->rowCount();
  this->Ui->deleteLayerButton->setEnabled(rc > 1);
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
  if(this->ExternalPinCell == NULL) return;
  this->InternalPinCell->fill(this->ExternalPinCell);


  this->Ui->CellMaterial->blockSignals(true);
  this->setupMaterialComboBox(this->Ui->CellMaterial, true);
  this->Ui->CellMaterial->blockSignals(false);

  cmbNucMaterialColors::instance()->selectIndex(this->Ui->CellMaterial,
                                                this->InternalPinCell->getCellMaterial());

  this->Ui->nameLineEdit->setText(this->InternalPinCell->getName().c_str());
  this->Ui->labelLineEdit->setText(this->InternalPinCell->getLabel().c_str());
  this->Ui->cutAwayViewCheckBox->setChecked(this->InternalPinCell->cutaway);

  this->Ui->Z0->setValue(this->InternalPinCell->getZ0());

  this->Ui->piecesTable->blockSignals(true);

  this->Ui->piecesTable->setColumnCount(6);
  this->Ui->piecesTable->setHorizontalHeaderLabels( QStringList() << "Segment\nType"
                                                   << "Length" << "Base\nRadius"
                                                   << "Top\nRadius" << "Origin\nX"
                                                   << "Origin\nY");

  this->Ui->piecesTable->setRowCount(static_cast<int>(this->InternalPinCell->GetNumberOfParts()));
  for(size_t i = 0; i < this->InternalPinCell->GetNumberOfParts(); i++)
  {
    PinSubPart *component = this->InternalPinCell->GetPart(static_cast<int>(i));
    this->createComponentItem(static_cast<int>(i), component);
  }

  this->Ui->piecesTable->resizeColumnsToContents();

  this->Ui->piecesTable->blockSignals(false);

  // Select the first row
  QTableWidgetItem* selItem = this->Ui->piecesTable->item(0, 0);
  this->Ui->piecesTable->setCurrentItem(selItem);
  selItem->setSelected(true);

  this->onPieceSelected();
  this->UpdateData();
  this->setButtons();
}

PinCell* cmbNucPinCellEditor::GetPinCell()
{
  return this->ExternalPinCell;
}

void cmbNucPinCellEditor::clear()
{
  delete(InternalPinCell);
  InternalPinCell = new PinCell();
  ExternalPinCell = NULL;
}

void cmbNucPinCellEditor::Apply()
{
  this->UpdatePinCell();
  bool change = false;
  if(this->ExternalPinCell->fill(this->InternalPinCell))
  {
    change = true;
  }

  QString newName = this->Ui->nameLineEdit->text();
  newName = newName.trimmed().replace(' ', "_");
  this->Ui->nameLineEdit->setText(newName);

  QString prevName = QString(this->ExternalPinCell->getName().c_str());
  this->ExternalPinCell->setName(newName.toStdString());
  if(newName != prevName)
  {
    emit nameChanged(this->ExternalPinCell, prevName, newName);
    change |= this->ExternalPinCell->getName() != prevName.toStdString();
  }

  QString newlabel = this->Ui->labelLineEdit->text();
  newlabel = newlabel.trimmed().replace(' ', "_");
  this->Ui->labelLineEdit->setText(newlabel);
  QString prevlabel = QString(this->ExternalPinCell->getLabel().c_str());
  this->ExternalPinCell->setLabel(newlabel.toStdString());
  if(newlabel != prevlabel)
  {
    emit labelChanged(this->ExternalPinCell, prevlabel, newlabel);
    change |= this->ExternalPinCell->getLabel() != prevlabel.toStdString();
  }

  this->Reset();
  if(change)
  {
    this->ExternalPinCell->GetConnection()->EmitChangeSignal();
    emit valueChange();
  }
}

void cmbNucPinCellEditor::UpdatePinCell()
{
  // update components
  double z = this->Ui->Z0->value();
  this->Ui->piecesTable->blockSignals(true);
  for(int i = 0; i < this->Ui->piecesTable->rowCount(); i++)
  {
    QComboBox *comboBox = qobject_cast<QComboBox *>(this->Ui->piecesTable->cellWidget(i, 0));
    PinSubPart* obj = static_cast<PinSubPart*>(comboBox->itemData(0).value<void *>());
    if(!obj)
    {
      continue;
    }
    double tmpx = this->Ui->piecesTable->item(i, 4)->text().toDouble();
    double tmpy = this->Ui->piecesTable->item(i, 5)->text().toDouble();
    double l = this->Ui->piecesTable->item(i, 1)->text().toDouble();
    //Rows are set automatically
    bool change = false;
    set_and_test(obj->x, tmpx);
    set_and_test(obj->y, tmpy);
    double z1 = obj->getZ1(), z2 = obj->getZ2();
    set_and_test(z1, z);
    set_and_test(z2, z + l);
    obj->setZ1(z1);
    obj->setZ2(z2);

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
  this->ExternalPinCell->setLabel(label.toStdString());
  this->Ui->labelLineEdit->setText(label);
}

void cmbNucPinCellEditor::badName(QString name)
{
  this->ExternalPinCell->setName(name.toStdString());
  this->Ui->nameLineEdit->setText(name);
}

PinSubPart* cmbNucPinCellEditor::createComponentObject(int r, PinSubPart * before)
{
  if(before == NULL)
  {
    return NULL;
  }
  this->Ui->piecesTable->blockSignals(true);

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
  vdata.setValue(static_cast<void*>(retObj));
  comboBox->setItemData(0, vdata);
  vdata.setValue(r); // row
  comboBox->setItemData(1, vdata);

  for(unsigned int c = 1; c < 6; ++c)//there are 6 columns
  {
    PinSegmentItem * tmpi = dynamic_cast< PinSegmentItem * >(this->Ui->piecesTable->item(r,c));
    tmpi->SubPart = retObj;
    tmpi->initialize();
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
  emit this->pincellModified(this->InternalPinCell);
  emit resetView();
}

void cmbNucPinCellEditor::addComponent()
{
  int row = this->Ui->piecesTable->rowCount();
  this->Ui->piecesTable->setRowCount(row + 1);

  PinSubPart * newObj = NULL;

  PinSubPart * previous =
         (dynamic_cast<SegmentRadiusItem*>(this->Ui->piecesTable->item(row - 1, 3)))->SubPart;
  newObj = previous->clone();
  newObj->setZ1(previous->getZ2());
  newObj->setZ2(newObj->getZ1() + previous->length());
  newObj->reverseRadii();

  this->createComponentItem(row, newObj);
  this->InternalPinCell->AddPart(newObj);

  // Select this row
  QTableWidgetItem* selItem = this->Ui->piecesTable->item(row, 0);
  this->Ui->piecesTable->setCurrentItem(selItem);
  selItem->setSelected(true);

  // update view
  this->UpdateData();
  this->setButtons();
}

void cmbNucPinCellEditor::createComponentItem( int row, PinSubPart* obj)
{
  QTableWidget * tmpTable = this->Ui->piecesTable;
  tmpTable->blockSignals(true);
  QTableWidgetItem *item = NULL;
  // type
  {//drop box
    QWidget * tmpWidget = tmpTable->cellWidget(row, 0);
    QComboBox* comboBox = dynamic_cast<QComboBox*>(tmpWidget);
    if(comboBox == NULL)
    {
      comboBox = new QComboBox;
      comboBox->addItem("Cylinder");
      comboBox->addItem("Frustum");
      comboBox->setObjectName("PincellPartBox_" + QString::number(row));
      tmpTable->setCellWidget(row, 0, comboBox);
      item = new QTableWidgetItem;
      tmpTable->setItem(row, 0, item);
      connect(comboBox, SIGNAL(currentIndexChanged(QString)),
              this, SLOT(sectionTypeComboBoxChanged(QString)));
    }
    QVariant vdata;
    vdata.setValue(static_cast<void*>(obj));
    comboBox->setItemData(0, vdata);
    vdata.setValue(row);
    comboBox->setItemData(1, vdata);
    comboBox->blockSignals(true);
    if(obj->GetType() == CMBNUC_ASSY_FRUSTUM_PIN)
    {
      comboBox->setCurrentIndex(1);
    }
    else
    {
      comboBox->setCurrentIndex(0);
    }
    comboBox->blockSignals(false);
  }

  // length
  item = new SegmentOtherValuesItem(obj);
  item->setText(QString::number(obj->length()));
  tmpTable->setItem(row, 1, item);

  // radius (base)
  item = new SegmentRadiusItem(obj, PinSubPart::BOTTOM);
  tmpTable->setItem(row, 2, item);

  // radius (top)
  item = new SegmentRadiusItem(obj, PinSubPart::TOP);
  tmpTable->setItem(row, 3, item);

  // Origin X
  item = new SegmentOtherValuesItem(obj);
  item->setText(QString::number(obj->x));
  tmpTable->setItem(row, 4, item);

  // Origin Y
  item = new SegmentOtherValuesItem(obj);
  item->setText(QString::number(obj->y));
  tmpTable->setItem(row, 5, item);

  tmpTable->blockSignals(false);
}

void cmbNucPinCellEditor::deleteComponent()
{
  this->Ui->piecesTable->blockSignals(true);
  PinSubPart* obj = this->getSelectedPiece();
  this->InternalPinCell->RemoveSection(obj);
  int row = this->Ui->piecesTable->currentRow();
  this->Ui->piecesTable->removeRow(row);

  int start = (row>0)?row - 1:0;

  (dynamic_cast< SegmentRadiusItem * >(this->Ui->piecesTable->item(start,3)))->checkAndSetNeighbors();

  this->Ui->piecesTable->blockSignals(false);

  // update view
  this->tableCellChanged();
  this->setButtons();
}

void cmbNucPinCellEditor::tableCellChanged()
{
  // update pin cell and render view
  this->UpdatePinCell();
  this->UpdateData();
}

void cmbNucPinCellEditor::sectionTypeComboBoxChanged(const QString &/*type*/)
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

void cmbNucPinCellEditor::setupMaterialComboBox(QComboBox *comboBox, bool iscell)
{
  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  matColorMap->setUp(comboBox);
  if(iscell)
  {
    comboBox->setItemText(0, "No Cell Material");
  }
}

void cmbNucPinCellEditor::onUpdateLayerMaterial()
{
  // setup materials
  QComboBox *comboBox;
  for(int i = 0; i < this->Ui->layersTable->rowCount(); ++i)
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
   this->Ui->layersTable->blockSignals(true);
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
  this->Ui->layersTable->blockSignals(true);
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
  this->InternalPinCell->InsertLayer(row);

  this->Ui->layersTable->setRowCount(this->InternalPinCell->GetNumberOfLayers());

  for(int i = 0; i < this->InternalPinCell->GetNumberOfLayers(); i++)
  {
    this->createMaterialRow(i, InternalPinCell->GetPart(0));
  }

  this->Ui->layersTable->blockSignals(false);
  this->UpdateData();
  this->setButtons();
}

//-----------------------------------------------------------------------------

void cmbNucPinCellEditor::createMaterialRow(int row, PinSubPart * obj)
{
  QTableWidget * tmpTable = this->Ui->layersTable;
  {//drop box
    QWidget * tmpWidget = tmpTable->cellWidget(row, 0);
    QComboBox* comboBox = dynamic_cast<QComboBox*>(tmpWidget);
    if(comboBox == NULL)
    {
      {
        //NOTE: This garbage is needed for testing.  It appears that resize does not delete old comboboxes for rows
        //thus testing gets confused by the name.  We are testing to see if the name exists.  If it does
        //we will rename it a more appropriate name.
        QComboBox* garbage = tmpTable->findChild<QComboBox*>( "PinMaterialBox_" + QString::number(row) );
        if(garbage)
          garbage->setObjectName("Garbage");
      }
      comboBox = new QComboBox;
      comboBox->setObjectName("PinMaterialBox_" + QString::number(row));
      tmpTable->setCellWidget(row, 0, comboBox);
      QObject::connect(comboBox, SIGNAL(currentIndexChanged(int)),
                       this, SLOT(onUpdateLayerMaterial()));
    }
    comboBox->blockSignals(true);
    this->setupMaterialComboBox(comboBox, false);
    QPointer<cmbNucMaterial> selMat;
    selMat = obj->GetMaterial(row);
    cmbNucMaterialColors::instance()->selectIndex(comboBox, selMat);
    comboBox->blockSignals(false);
  }

  tmpTable->setItem(row, 0, new QTableWidgetItem());

  QTableWidgetItem *item = new LayerRadiusEditor(this->InternalPinCell);
  item->setText(QString::number(this->InternalPinCell->Radius(row)));
  tmpTable->setItem(row, 1, item);
}

//-----------------------------------------------------------------------------
void cmbNucPinCellEditor::addLayerAfter()
{
  int row;
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
  this->InternalPinCell->InsertLayer(row);
  this->Ui->layersTable->setRowCount(this->InternalPinCell->GetNumberOfLayers());
  dynamic_cast<LayerRadiusEditor*>(this->Ui->layersTable->item(row-1,1))->refresh();

  for(int i = 0; i < this->InternalPinCell->GetNumberOfLayers(); i++)
  {
    this->createMaterialRow(i, InternalPinCell->GetPart(0));
  }

  this->Ui->layersTable->blockSignals(false);
  this->UpdateData();
  this->setButtons();
}

//-----------------------------------------------------------------------------
void cmbNucPinCellEditor::deleteLayer()
{
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
  this->setButtons();
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
  this->setButtons();
}

