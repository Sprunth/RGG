
#include "cmbNucMaterialTreeItem.h"
#include <QFont>
#include <QBrush>
#include <QDebug>

void cmbNucMaterialTreeItemConnection::show(int mode)
{
  if(this->v->Material == NULL)
  {
    this->v->setHidden(true);
    return;
  }
  switch(mode)
  {
    case 0:
      this->v->setHidden(false);
      break;
    case 1:
      this->v->setHidden(!this->v->Material->isUsed());
      break;
    case 2:
      this->v->setHidden(!this->v->Material->isDisplayed());
      break;
  }
}

//-----------------------------------------------------------------------------
cmbNucMaterialTreeItem::cmbNucMaterialTreeItem( QTreeWidgetItem* pNode,
                                                QPointer<cmbNucMaterial> mat)
: QTreeWidgetItem(pNode), Material(mat)
{
  Connection = new cmbNucMaterialTreeItemConnection();
  Connection->v = this;
  Qt::ItemFlags matFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable |
                         Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
  this->setText(1, mat->getName());
  this->setText(2, mat->getLabel());
  QBrush bgBrush(mat->getColor());
  this->setBackground(3, bgBrush);
  this->setFlags(matFlags);
  this->setCheckState(0, Qt::Checked);
}

//-----------------------------------------------------------------------------
cmbNucMaterialTreeItem::~cmbNucMaterialTreeItem()
{
  delete this->Connection;
}

//-----------------------------------------------------------------------------
void cmbNucMaterialTreeItem::setText ( int column, const QString & tin )
{
  this->QTreeWidgetItem::setText(column, tin);
  if(column == 0)
    {
    this->PreviousText = tin;
    }
}

//-----------------------------------------------------------------------------
void cmbNucMaterialTreeItem::setData ( int column, int role,
                                       const QVariant & value )
{
  QVariant sv = value;
  if(column == 0 && role == Qt::CheckStateRole)
  {
    bool cs = this->checkState(0)==0;
    if( cs != Material->isVisible() )
    {
      Material->setVisible(cs);
      Material->emitColorChange();
    }
  }
  else if(column == 1 && role == Qt::EditRole)
  {
    if(value.toString()!=Material->getName())
      Material->setName(value.toString());
    sv = QVariant(Material->getName());
  }
  else if(column == 2 && role == Qt::EditRole)
  {
    if(value.toString()!=Material->getLabel())
      Material->setLabel(value.toString());
    sv = QVariant(Material->getLabel());
  }
  else if(column == 3 && role == Qt::BackgroundRole)
  {
    QColor tmp = value.value<QColor>();
    if(Material->getColor()!=tmp)
    {
      Material->setColor(tmp);
      Material->emitColorChange();
    }
  }
  this->QTreeWidgetItem::setData(column, role, sv);
}
