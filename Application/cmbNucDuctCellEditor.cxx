#include "cmbNucDuctCellEditor.h"

#include "cmbNucDuctCell.h"
#include "cmbNucAssembly.h"

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

  connect( this->Ui->AddSegmentBefore, SIGNAL(clicked()),
           this,                       SLOT(addDuctBefore()));
  connect( this->Ui->AddSegmentAfter,  SIGNAL(clicked()),
           this,                       SLOT(addDuctAfter()));
  connect( this->Ui->DeleteSegment,    SIGNAL(clicked()),
           this,                       SLOT(deleteDuct()));

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

  if(this->ExternalDuctCell != NULL)
  {
    this->InternalDuctCell->fill(this->ExternalDuctCell);
    tmpTable->setRowCount(this->ExternalDuctCell->numberOfDucts());
    for(unsigned int i = 0; i < this->ExternalDuctCell->numberOfDucts(); ++i)
    {
      this->setDuctRow(i, this->ExternalDuctCell->getDuct(i));
    }
  }
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
}

void
cmbNucDuctCellEditor
::addDuctBefore()
{
}

void
cmbNucDuctCellEditor
::addDuctAfter()
{
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
::fillMaterialTable(Duct * d)
{
}
