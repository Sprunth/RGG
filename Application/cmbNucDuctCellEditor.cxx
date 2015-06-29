#include "cmbNucDuctCellEditor.h"

#include "cmbNucDuctCell.h"
#include "cmbNucAssembly.h"
#include "cmbNucMaterialColors.h"

#include <QComboBox>

class DuctTableItem : public QTableWidgetItem
{
public:
  DuctTableItem(Duct * din, cmbNucDuctCellEditor * l, double init)
  :duct(din), link(l)
  {
    this->setText(QString::number(init));
  }
  bool valid(const QVariant& value, double & v)
  {
    bool ok;
    v = value.toDouble(&ok);

    if (!ok)
    {
      return false;
    }

    if(this->column() == 1 && this->row()+1 == this->tableWidget()->rowCount())
    {
      return false;
    }

    if(this->column() == 0 && this->row() == 0)
    {
      return false;
    }

    if(this->column() == 0)
    {
      if(v >= this->duct->getZ2()) return false;
      if(this->row() > 0)
      {
        int other_col = 1;
        int other_row = this->row() - 1;
        QTableWidgetItem * other = this->tableWidget()->item( other_row, other_col );
        DuctTableItem* selItem = dynamic_cast<DuctTableItem*>(other);
        if(v <= selItem->duct->getZ1()) return false;
      }
    }

    if(this->column() == 1)
    {
      if(v <= this->duct->getZ1()) return false;
      if(this->row()-1 < this->tableWidget()->rowCount())
      {
        int other_col = 0;
        int other_row = this->row() + 1;
        QTableWidgetItem * other = this->tableWidget()->item( other_row, other_col );
        DuctTableItem* selItem = dynamic_cast<DuctTableItem*>(other);
        if(v >= selItem->duct->getZ2()) return false;
      }
    }

    return true;
  }

  virtual void setData(int role, const QVariant& value)
  {
    double tmpv;
    if (this->tableWidget() != NULL && role == Qt::EditRole)
    {
      if(!valid(value,tmpv)) return;
      if(this->column() == 0)
      {
        this->duct->setZ1(tmpv);
        if(this->row() > 0)
        {
          int other_col = 1;
          int other_row = this->row() - 1;
          QTableWidgetItem * other = this->tableWidget()->item( other_row, other_col );
          DuctTableItem* selItem = dynamic_cast<DuctTableItem*>(other);
          selItem->duct->setZ2(tmpv);
          other->setText(QString::number(tmpv));
        }
      }
      else
      {
        this->duct->setZ2(tmpv);
        if(this->row()-1 < this->tableWidget()->rowCount())
        {
          int other_col = 0;
          int other_row = this->row() + 1;
          QTableWidgetItem * other = this->tableWidget()->item( other_row, other_col );
          DuctTableItem* selItem = dynamic_cast<DuctTableItem*>(other);
          selItem->duct->setZ1(tmpv);
          other->setText(QString::number(tmpv));
        }
      }
      link->update();
    }
    QTableWidgetItem::setData(role, value);
  }

  virtual ~DuctTableItem() {};
  Duct * duct;
  cmbNucDuctCellEditor * link;
};

class DuctLayerThicknessEditor : public QTableWidgetItem
{
public:
  DuctLayerThicknessEditor(Duct * din, bool hex, cmbNucDuctCellEditor * l, double init)
  :duct(din), link(l), isHex(hex)
  {
    this->setText(QString::number(init));
  }

  void setValue(Duct * in, double v)
  {
    duct = in;
    this->setText(QString::number(v));
  }

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
      int tr = this->row();
      // Make sure value is greater than previous row
      if (this->row() > 0)
      {
        double prev = this->tableWidget()->item(this->row() - 1, this->column())
        ->data(Qt::DisplayRole).toDouble();
        if (dval <= prev)
        {
          return;
        }
      }
      // Make sure value is less than next row
      if (this->row() < this->tableWidget()->rowCount() - 1)
      {
        double next = this->tableWidget()->item(this->row() + 1, this->column())
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
      double* thick = duct->getNormThick(this->row());
      if(isHex)
      {
        thick[0] = thick[1] = dval;
      }
      else
      {
        thick[this->column() - 1] = dval;
      }
      link->update();
    }
    QTableWidgetItem::setData(role, value);
  }
  Duct * duct;
  cmbNucDuctCellEditor * link;
  bool isHex;
};


cmbNucDuctCellEditor
::cmbNucDuctCellEditor(QWidget *p)
: QWidget(p),
  Ui(new Ui::cmbDuctCellEditor),
  AssemblyObject(NULL)
{
  this->isHex = false;
  this->InternalDuctCell = new DuctCell();
  this->ExternalDuctCell = NULL;
  this->Ui->setupUi(this);

  this->Ui->MaterialLayerTable->setRowCount(0);
  this->Ui->MaterialLayerTable->setColumnCount(2);
  this->Ui->MaterialLayerTable->setHorizontalHeaderLabels( QStringList() << "Material"
                                                           << "Normalized\nThickness 1"
                                                           << "Normalized\nThickness 2");
  this->Ui->MaterialLayerTable->horizontalHeader()->setStretchLastSection(true);

  this->Ui->DuctSegmentTable->setRowCount(0);
  this->Ui->DuctSegmentTable->setColumnCount(2);
  this->Ui->DuctSegmentTable->setHorizontalHeaderLabels( QStringList() << "Z1" << "Z2" );
  this->Ui->DuctSegmentTable->horizontalHeader()->setStretchLastSection(true);

  this->Ui->Split->setEnabled(false);
  this->Ui->DeleteUp->setEnabled(false);
  this->Ui->DeleteDown->setEnabled(false);
  this->Ui->AddMaterialBefore->setEnabled(false);
  this->Ui->AddMaterialAfter->setEnabled(false);
  this->Ui->DeleteMaterial->setEnabled(false);

  connect(this->Ui->DuctSegmentTable, SIGNAL(itemSelectionChanged()),
          this, SLOT(ductTableCellSelection()));

  connect(this->Ui->CrossSection, SIGNAL(clicked()),
          this,                   SLOT(update()));

  connect( this->Ui->Split,      SIGNAL(clicked()),
           this,                 SLOT(splitDuct()));
  connect( this->Ui->DeleteUp,   SIGNAL(clicked()),
           this,                 SLOT(deleteUp()));
  connect( this->Ui->DeleteDown, SIGNAL(clicked()),
           this,                 SLOT(deleteDown()));

  connect( this->Ui->AddMaterialBefore, SIGNAL(clicked()),
           this,                        SLOT(addLayerBefore()));
  connect( this->Ui->AddMaterialAfter,  SIGNAL(clicked()),
           this,                        SLOT(addLayerAfter()));
  connect( this->Ui->DeleteMaterial,    SIGNAL(clicked()),
           this,                        SLOT(deleteLayer()));

}

cmbNucDuctCellEditor
::~cmbNucDuctCellEditor()
{
  delete Ui;
  delete InternalDuctCell;
}

void
cmbNucDuctCellEditor
::SetDuctCell(DuctCell *ductcell, bool hex)
{
  this->isHex = hex;

  this->ExternalDuctCell = ductcell;

  this->Ui->Split->setEnabled(false);
  this->Ui->DeleteUp->setEnabled(false);
  this->Ui->DeleteDown->setEnabled(false);
  this->Ui->AddMaterialBefore->setEnabled(false);
  this->Ui->AddMaterialAfter->setEnabled(false);
  this->Ui->DeleteMaterial->setEnabled(false);

  this->Reset();
}

void
cmbNucDuctCellEditor
::clear()
{
  this->Ui->MaterialLayerTable->clear();
  this->Ui->DuctPitchX->setText("0");
  this->Ui->DuctPitchY->setText("0");
  this->Ui->Z1->setText("0");
  this->Ui->Z2->setText("0");
}

void
cmbNucDuctCellEditor
::Apply()
{
  if(this->ExternalDuctCell != NULL)
  {
    QString prevName = this->InternalDuctCell->getName().c_str();
    QString newName = this->Ui->Name->text();
    newName = newName.trimmed().replace(' ', "_");

    if(newName != prevName)
    {
      this->InternalDuctCell->setName(newName.toStdString());
      emit nameChanged(this->InternalDuctCell, prevName, newName);
    }

    this->Ui->Name->setText(this->InternalDuctCell->getName().c_str());

    if(!(*this->ExternalDuctCell == *this->InternalDuctCell) )
    {
      this->ExternalDuctCell->fill(this->InternalDuctCell);
      emit valueChange();
    }
  }
}

bool cmbNucDuctCellEditor
::isCrossSectioned()
{
  return this->Ui->CrossSection->isChecked();
}

void
cmbNucDuctCellEditor
::update()
{
  emit ductcellModified(InternalDuctCell);
}

void
cmbNucDuctCellEditor
::Reset()
{
  QTableWidget * tmpTable = this->Ui->MaterialLayerTable;
  tmpTable->clear();
  tmpTable->setRowCount(0);
  tmpTable->setColumnCount(3);
  tmpTable->setColumnHidden(2, this->isHex);
  if(this->isHex)
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
  this->Ui->DuctPitchXLabel->setVisible(!this->isHex);
  this->Ui->DuctPitchY->setVisible(!this->isHex);
  this->Ui->DuctPitchYLabel->setVisible(!this->isHex);

  tmpTable = this->Ui->DuctSegmentTable;

  tmpTable->setRowCount(0);
  double global_z1 = 0, global_z2 = 0;
  double thickness[] = {0,0};

  if(this->ExternalDuctCell != NULL)
  {
    this->InternalDuctCell->fill(this->ExternalDuctCell);
    this->Ui->Name->setText(this->InternalDuctCell->getName().c_str());
    tmpTable->setRowCount(static_cast<int>(this->InternalDuctCell->numberOfDucts()));
    for(size_t i = 0; i < this->InternalDuctCell->numberOfDucts(); ++i)
    {
      const int ti = static_cast<int>(i);
      Duct* d =this->InternalDuctCell->getDuct(ti);
      this->setDuctRow(ti, this->InternalDuctCell->getDuct(ti));
      if(i == 0)
      {
        global_z1 = d->getZ1();
        global_z2 = d->getZ2();
        thickness[0] = d->getThickness(0);
        thickness[1] = d->getThickness(1);
      }
      else
      {
        global_z2 = d->getZ2();
      }
    }
  }
  this->Ui->Z1->setText(QString::number(global_z1));
  this->Ui->Z2->setText(QString::number(global_z2));

  this->Ui->DuctPitchX->setText(QString::number(thickness[0]));
  this->Ui->DuctPitchY->setText(QString::number(thickness[1]));
  this->update();
}

void
cmbNucDuctCellEditor
::setDuctRow(int r, Duct * d)
{
  DuctTableItem * item = new DuctTableItem(d, this, d->getZ1());
  this->Ui->DuctSegmentTable->setItem(r, 0, item);
  item = new DuctTableItem(d, this, d->getZ2());
  item->link = this;
  this->Ui->DuctSegmentTable->setItem(r, 1, item);
}

void
cmbNucDuctCellEditor
::ductTableCellSelection()
{
  if(this->Ui->DuctSegmentTable->selectedItems().count()==0)
  {
    return;
  }
  this->Ui->Split->setEnabled(true);
  QTableWidgetItem * rad = this->Ui->DuctSegmentTable->selectedItems().value(0);
  DuctTableItem* selItem = dynamic_cast<DuctTableItem*>(rad);
  if(!selItem)
  {
    return;
  }
  this->fillMaterialTable(selItem->duct);
  this->Ui->DeleteUp->setEnabled(rad->row() > 0 &&
                                 this->Ui->DuctSegmentTable->rowCount() > 1);
  this->Ui->DeleteDown->setEnabled(this->Ui->DuctSegmentTable->rowCount() > 1 &&
                                   rad->row() < this->Ui->DuctSegmentTable->rowCount()-1);
  this->Ui->AddMaterialBefore->setEnabled(true);
  this->Ui->AddMaterialAfter->setEnabled(true);

  this->Ui->DeleteMaterial->setEnabled(this->Ui->MaterialLayerTable->rowCount() > 1);
}

void
cmbNucDuctCellEditor
::splitDuct()
{
  if(this->Ui->DuctSegmentTable->selectedItems().count()==0)
  {
    return;
  }
  QTableWidgetItem* rad = this->Ui->DuctSegmentTable->selectedItems().value(0);
  DuctTableItem* selItem = dynamic_cast<DuctTableItem*>(rad);
  if(!selItem)
  {
    return;
  }
  Duct * newd = new Duct(selItem->duct, true);
  this->InternalDuctCell->AddDuct(newd);
  int row = rad->row();
  this->Ui->DuctSegmentTable->insertRow(row+1);
  this->setDuctRow(row, selItem->duct);
  this->setDuctRow(row+1, newd);
  ductTableCellSelection();
  this->update();
}

void
cmbNucDuctCellEditor
::deleteUp()
{
  QTableWidgetItem* rad = this->Ui->DuctSegmentTable->selectedItems().value(0);
  DuctTableItem* selItem = dynamic_cast<DuctTableItem*>(rad);
  int row = rad->row();
  this->InternalDuctCell->RemoveDuct(selItem->duct, true);
  selItem = dynamic_cast<DuctTableItem*>(this->Ui->DuctSegmentTable->item(row-1, 0));
  this->setDuctRow(row-1, selItem->duct);
  this->Ui->DuctSegmentTable->removeRow(row);
  ductTableCellSelection();
  this->update();
}

void cmbNucDuctCellEditor
::deleteDown()
{
  QTableWidgetItem* rad = this->Ui->DuctSegmentTable->selectedItems().value(0);
  DuctTableItem* selItem = dynamic_cast<DuctTableItem*>(rad);
  int row = rad->row();
  this->InternalDuctCell->RemoveDuct(selItem->duct, false);
  selItem = dynamic_cast<DuctTableItem*>(this->Ui->DuctSegmentTable->item(row+1, 0));
  this->Ui->DuctSegmentTable->removeRow(row);
  this->setDuctRow(row, selItem->duct);
  ductTableCellSelection();
  this->update();
}

void
cmbNucDuctCellEditor
::addLayerBefore()
{
  QTableWidget * table = this->Ui->MaterialLayerTable;
  table->blockSignals(true);
  int row = 0;
  if(table->selectedItems().count() > 0)
  {
    row = table->selectedItems().value(0)->row();
  }

  QTableWidgetItem* rad = this->Ui->DuctSegmentTable->selectedItems().value(0);
  DuctTableItem* selItem = dynamic_cast<DuctTableItem*>(rad);

  selItem->duct->insertLayer(row);
  this->fillMaterialTable(selItem->duct);
  this->Ui->DeleteMaterial->setEnabled(this->Ui->MaterialLayerTable->rowCount() > 1);
  table->blockSignals(false);
  this->update();
}

void
cmbNucDuctCellEditor
::addLayerAfter()
{
  QTableWidget * table = this->Ui->MaterialLayerTable;
  table->blockSignals(true);
  int row = table->rowCount();
  if(table->selectedItems().count() > 0)
  {
    row = table->selectedItems().value(0)->row() + 1;
  }

  QTableWidgetItem* rad = this->Ui->DuctSegmentTable->selectedItems().value(0);
  DuctTableItem* selItem = dynamic_cast<DuctTableItem*>(rad);

  selItem->duct->insertLayer(row);
  this->fillMaterialTable(selItem->duct);
  this->Ui->DeleteMaterial->setEnabled(table->rowCount() > 1);
  table->blockSignals(false);
  this->update();
}

void
cmbNucDuctCellEditor
::deleteLayer()
{
  QTableWidget * table = this->Ui->MaterialLayerTable;
  table->blockSignals(true);
  if(table->selectedItems().count() == 0)
  {
    return;
  }
  QTableWidgetItem* rad = this->Ui->DuctSegmentTable->selectedItems().value(0);
  DuctTableItem* selItem = dynamic_cast<DuctTableItem*>(rad);

  int row = table->selectedItems().value(0)->row();

  selItem->duct->removeLayer( row );

  this->fillMaterialTable(selItem->duct);
  table->blockSignals(false);

  this->Ui->DeleteMaterial->setEnabled(table->rowCount() > 1);
  this->update();
}

void
cmbNucDuctCellEditor
::rebuildLayersFromTable()
{
}

void
cmbNucDuctCellEditor
::fillMaterialTable(Duct * duct)
{
  QTableWidget * tmpTable = this->Ui->MaterialLayerTable;
  tmpTable->horizontalHeader()->setStretchLastSection(true);
  if(duct == NULL) return;
  tmpTable->blockSignals(true);
  tmpTable->setRowCount(static_cast<int>(duct->NumberOfLayers()));

  for(size_t i = 0; i < duct->NumberOfLayers(); i++)
  {
    this->setDuctMaterialRow(static_cast<int>(i), duct);
  }
  tmpTable->resizeColumnsToContents();
  tmpTable->blockSignals(false);
}

void
cmbNucDuctCellEditor
::setDuctMaterialRow(int row, Duct * duct)
{
  QTableWidget * tmpTable = this->Ui->MaterialLayerTable;
  {//drop box
    QWidget * tmpWidget = tmpTable->cellWidget(row, 0);
    QComboBox* comboBox = dynamic_cast<QComboBox*>(tmpWidget);
    if(comboBox == NULL)
    {
      {
        //NOTE: This garbage is needed for testing.  It appears that resize does not delete old comboboxes for rows
        //thus testing gets confused by the name.  We are testing to see if the name exists.  If it does
        //we will rename it a more appropriate name.
        QComboBox* garbage = tmpTable->findChild<QComboBox*>( "DuctMaterialBox_" + QString::number(row) );
        if(garbage) garbage->setObjectName("Garbage");
      }
      comboBox = new QComboBox;
      comboBox->setObjectName("DuctMaterialBox_" + QString::number(row));
      tmpTable->setCellWidget(row, 0, comboBox);
      QObject::connect(comboBox, SIGNAL(currentIndexChanged(int)),
                       this, SLOT(onUpdateLayerMaterial()));
    }
    comboBox->blockSignals(true);
    cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
    matColorMap->setUp(comboBox);
    matColorMap->selectIndex(comboBox, duct->getMaterial(row));
    comboBox->blockSignals(false);
  }

  double* thick = duct->getNormThick(row);

  DuctLayerThicknessEditor* thick1Item = dynamic_cast< DuctLayerThicknessEditor* >(tmpTable->item(row,1));
  DuctLayerThicknessEditor* thick2Item = dynamic_cast< DuctLayerThicknessEditor* >(tmpTable->item(row,2));

  if(thick1Item == NULL)
  {
    thick1Item = new DuctLayerThicknessEditor(duct, isHex, this, thick[0]);
    tmpTable->setItem(row, 1, thick1Item);
  }
  else
  {
    thick1Item->setValue(duct, thick[0]);
  }

  if(thick2Item == NULL)
  {
    thick2Item = new DuctLayerThicknessEditor(duct, isHex, this, thick[1]);
    tmpTable->setItem(row, 2, thick2Item);
  }
  else
  {
    thick2Item->setValue(duct, thick[1]);
  }
}

void cmbNucDuctCellEditor::onUpdateLayerMaterial()
{
  QTableWidgetItem* rad = this->Ui->DuctSegmentTable->selectedItems().value(0);
  DuctTableItem* selItem = dynamic_cast<DuctTableItem*>(rad);
  // setup materials
  QComboBox *comboBox;
  for(int i = 0; i < this->Ui->MaterialLayerTable->rowCount(); ++i)
  {
    comboBox = qobject_cast<QComboBox *>(this->Ui->MaterialLayerTable->cellWidget(i, 0));
    if(comboBox)
    {
      QPointer<cmbNucMaterial> mat =
      cmbNucMaterialColors::instance()->getMaterial(comboBox);
      selItem->duct->setMaterial(i, mat);
    }
  }
  this->update();
}
