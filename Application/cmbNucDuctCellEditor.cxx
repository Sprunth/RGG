#include "cmbNucDuctCellEditor.h"

#include "cmbNucDuctCell.h"
#include "cmbNucAssembly.h"
#include "cmbNucMaterialColors.h"

#include <QComboBox>

class DuctTableItem : public QTableWidgetItem
{
public:
  DuctTableItem(Duct * d, cmbNucDuctCellEditor * l, double init)
  :duct(d), link(l)
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
      if(v >= this->duct->z2) return false;
      if(this->row() > 0)
      {
        int other_col = 1;
        int other_row = this->row() - 1;
        QTableWidgetItem * other = this->tableWidget()->item( other_row, other_col );
        DuctTableItem* selItem = dynamic_cast<DuctTableItem*>(other);
        if(v <= selItem->duct->z1) return false;
      }
    }

    if(this->column() == 1)
    {
      if(v <= this->duct->z1) return false;
      if(this->row()-1 < this->tableWidget()->rowCount())
      {
        int other_col = 0;
        int other_row = this->row() + 1;
        QTableWidgetItem * other = this->tableWidget()->item( other_row, other_col );
        DuctTableItem* selItem = dynamic_cast<DuctTableItem*>(other);
        if(v >= selItem->duct->z2) return false;
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
        this->duct->z1 = tmpv;
        if(this->row() > 0)
        {
          int other_col = 1;
          int other_row = this->row() - 1;
          QTableWidgetItem * other = this->tableWidget()->item( other_row, other_col );
          DuctTableItem* selItem = dynamic_cast<DuctTableItem*>(other);
          selItem->duct->z2 = tmpv;
          other->setText(QString::number(tmpv));
        }
      }
      else
      {
        this->duct->z2 = tmpv;
        if(this->row()-1 < this->tableWidget()->rowCount())
        {
          int other_col = 0;
          int other_row = this->row() + 1;
          QTableWidgetItem * other = this->tableWidget()->item( other_row, other_col );
          DuctTableItem* selItem = dynamic_cast<DuctTableItem*>(other);
          selItem->duct->z1 = tmpv;
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
  DuctLayerThicknessEditor(Duct * d, cmbNucDuctCellEditor * l, double init)
  :duct(d), link(l)
  {
    this->setText(QString::number(init));
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
      double* thick = duct->getNormThick(this->row());
      thick[this->column() - 1] = dval;
      link->update();
    }
    QTableWidgetItem::setData(role, value);
  }
  Duct * duct;
  cmbNucDuctCellEditor * link;
};


cmbNucDuctCellEditor
::cmbNucDuctCellEditor(QWidget *parent)
: QWidget(parent),
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
  this->Reset();
}

void
cmbNucDuctCellEditor
::SetAssembly(cmbNucAssembly *assembly)
{
  AssemblyObject = assembly;
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
    if(!(*this->ExternalDuctCell == *this->InternalDuctCell))
    {
      this->ExternalDuctCell->fill(this->InternalDuctCell);
      AssemblyObject->geometryChanged();
      emit valueChange();
    }
  }
}

void
cmbNucDuctCellEditor
::update()
{
  this->InternalDuctCell->CachedData.TakeReference(cmbNucAssembly::CreateDuctCellMultiBlock(this->InternalDuctCell,
                                                                                            this->isHex,
                                                                                            this->Ui->CrossSection->isChecked()));
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
    tmpTable->setRowCount(this->InternalDuctCell->numberOfDucts());
    for(unsigned int i = 0; i < this->InternalDuctCell->numberOfDucts(); ++i)
    {
      Duct* d =this->InternalDuctCell->getDuct(i);
      this->setDuctRow(i, this->InternalDuctCell->getDuct(i));
      if(i == 0)
      {
        global_z1 = d->z1;
        global_z2 = d->z2;
        thickness[0] = d->thickness[0];
        thickness[1] = d->thickness[1];
      }
      else
      {
        global_z2 = d->z2;
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
  DuctTableItem * item = new DuctTableItem(d, this, d->z1);
  this->Ui->DuctSegmentTable->setItem(r, 0, item);
  item = new DuctTableItem(d, this, d->z2);
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
  double z2 = selItem->duct->z2;
  this->InternalDuctCell->RemoveDuct(selItem->duct);
  selItem = dynamic_cast<DuctTableItem*>(this->Ui->DuctSegmentTable->item(row-1, 0));
  selItem->duct->z2 = z2;
  this->setDuctRow(row-1, selItem->duct);
  this->Ui->DuctSegmentTable->removeRow(row);
  this->update();
}

void cmbNucDuctCellEditor
::deleteDown()
{
  QTableWidgetItem* rad = this->Ui->DuctSegmentTable->selectedItems().value(0);
  DuctTableItem* selItem = dynamic_cast<DuctTableItem*>(rad);
  int row = rad->row();
  double z1 = selItem->duct->z1;
  this->InternalDuctCell->RemoveDuct(selItem->duct);
  selItem = dynamic_cast<DuctTableItem*>(this->Ui->DuctSegmentTable->item(row+1, 0));
  selItem->duct->z1 = z1;
  this->setDuctRow(row+1, selItem->duct);
  this->Ui->DuctSegmentTable->removeRow(row);
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
  tmpTable->setRowCount(duct->NumberOfLayers());

  for(size_t i = 0; i < duct->NumberOfLayers(); i++)
  {
    this->setDuctMaterialRow(i, duct);
  }
  tmpTable->resizeColumnsToContents();
  tmpTable->blockSignals(false);
}

void
cmbNucDuctCellEditor
::setDuctMaterialRow(int row, Duct * duct)
{
  QTableWidget * tmpTable = this->Ui->MaterialLayerTable;
  QComboBox* comboBox = new QComboBox;
  double* thick = duct->getNormThick(row);

  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  matColorMap->setUp(comboBox);
  matColorMap->selectIndex(comboBox, duct->getMaterial(row));

  tmpTable->setCellWidget(row, 0, comboBox);

  QObject::connect(comboBox, SIGNAL(currentIndexChanged(int)),
                   this, SLOT(onUpdateLayerMaterial()));

  DuctLayerThicknessEditor* thick1Item = new DuctLayerThicknessEditor(duct, this, thick[0]);
  DuctLayerThicknessEditor* thick2Item = new DuctLayerThicknessEditor(duct, this, thick[1]);

  tmpTable->setItem(row, 1, thick1Item);
  tmpTable->setItem(row, 2, thick2Item);
}

void cmbNucDuctCellEditor::onUpdateLayerMaterial()
{
  QTableWidgetItem* rad = this->Ui->DuctSegmentTable->selectedItems().value(0);
  DuctTableItem* selItem = dynamic_cast<DuctTableItem*>(rad);
  // setup materials
  QComboBox *comboBox;
  for(unsigned int i = 0; i < this->Ui->MaterialLayerTable->rowCount(); ++i)
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
