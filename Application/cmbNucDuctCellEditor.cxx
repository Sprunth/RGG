#include "cmbNucDuctCellEditor.h"

#include "cmbNucDuctCell.h"
#include "cmbNucAssembly.h"
#include "cmbNucMaterialColors.h"

#include <QComboBox>

class DuctTableItem : public QTableWidgetItem
{
public:
  DuctTableItem(Duct * d, double init)
  :duct(d)
  {
    this->setText(QString::number(init));
  }
  bool valid(const QVariant& value, double & v)
  {
    bool ok;
    v = value.toDouble(&ok);

    // Make sure value is positive
    if (!ok)
    {
      return false;
    }

    if(this->column() == 1 && this->row()+1 == this->tableWidget()->rowCount())
    {
      return false;
    }

    return true;
  }

  /*virtual void setData(int role, const QVariant& value)
  {
  }*/

  virtual ~DuctTableItem() {};
  Duct * duct;
};

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
                                                           << "Normalized Thickness 1"
                                                           << "Normalized Thickness 2");
  this->Ui->MaterialLayerTable->horizontalHeader()->setStretchLastSection(true);

  this->Ui->DuctSegmentTable->setRowCount(0);
  this->Ui->DuctSegmentTable->setColumnCount(2);
  this->Ui->DuctSegmentTable->setHorizontalHeaderLabels( QStringList() << "Z1" << "Z2" );
  this->Ui->DuctSegmentTable->horizontalHeader()->setStretchLastSection(true);

  this->Ui->Split->setEnabled(false);
  this->Ui->DeleteUp->setEnabled(false);

  connect(this->Ui->DuctSegmentTable, SIGNAL(itemSelectionChanged()),
          this, SLOT(ductTableCellSelection()));

  connect( this->Ui->Split, SIGNAL(clicked()),
           this,            SLOT(splitDuct()));
  connect( this->Ui->DeleteUp,  SIGNAL(clicked()),
           this,                SLOT(deleteUp()));
  //connect( this->Ui->DeleteSegment,    SIGNAL(clicked()),
  //         this,                       SLOT(deleteDuct()));

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
  this->isHex = false;

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

  if(this->ExternalDuctCell != NULL)
  {
    this->InternalDuctCell->fill(this->ExternalDuctCell);
    tmpTable->setRowCount(this->ExternalDuctCell->numberOfDucts());
    for(unsigned int i = 0; i < this->ExternalDuctCell->numberOfDucts(); ++i)
    {
      Duct* d =this->ExternalDuctCell->getDuct(i);
      this->setDuctRow(i, this->ExternalDuctCell->getDuct(i));
      if(i == 0)
      {
        global_z1 = d->z1;
        global_z2 = d->z2;
      }
      else
      {
        global_z2 = d->z2;
      }
    }
  }
  this->Ui->Z1->setText(QString::number(global_z1));
  this->Ui->Z2->setText(QString::number(global_z2));
}

void
cmbNucDuctCellEditor
::setDuctRow(int r, Duct * d)
{
  DuctTableItem * item = new DuctTableItem(d, d->z1);
  this->Ui->DuctSegmentTable->setItem(r, 0, item);
  item = new DuctTableItem(d, d->z2);
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
  QTableWidgetItem * rad = this->Ui->DuctSegmentTable->selectedItems().value(1);
  DuctTableItem* selItem = dynamic_cast<DuctTableItem*>(rad);
  if(!selItem)
  {
    return;
  }
  this->fillMaterialTable(selItem->duct);
  this->Ui->DeleteUp->setEnabled(rad->row() > 0 &&
                                 this->Ui->DuctSegmentTable->rowCount() > 1);
}

void
cmbNucDuctCellEditor
::splitDuct()
{
  if(this->Ui->DuctSegmentTable->selectedItems().count()==0)
  {
    return;
  }
  QTableWidgetItem* rad = this->Ui->DuctSegmentTable->selectedItems().value(1);
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
}

void
cmbNucDuctCellEditor
::deleteUp()
{
  QTableWidgetItem* rad = this->Ui->DuctSegmentTable->selectedItems().value(1);
  DuctTableItem* selItem = dynamic_cast<DuctTableItem*>(rad);
  int row = rad->row();
  double z2 = selItem->duct->z2;
  this->InternalDuctCell->RemoveDuct(selItem->duct);
  selItem = dynamic_cast<DuctTableItem*>(this->Ui->DuctSegmentTable->item(row-1, 0));
  selItem->duct->z2 = z2;
  this->setDuctRow(row-1, selItem->duct);
  this->Ui->DuctSegmentTable->removeRow(row);
}

void
cmbNucDuctCellEditor
::deleteDuct()
{
}

void
cmbNucDuctCellEditor
::addLayerBefore()
{
}

void
cmbNucDuctCellEditor
::addLayerAfter()
{
}

void
cmbNucDuctCellEditor
::deleteLayer()
{
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
