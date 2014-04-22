
#include "cmbNucMaterialTreeItem.h"
#include <QFont>
#include <QBrush>
#include <QDebug>

void cmbNucMaterialTreeItemConnection::revert()
{
  qDebug() << "Reverting: " << v->Material->getName() << v->Material->getLabel();
  v->setHidden(true);
  v->setText(1,v->Material->getName());
  v->setText(2,v->Material->getLabel());
  v->setHidden(false);
}

//-----------------------------------------------------------------------------
cmbNucMaterialTreeItem::cmbNucMaterialTreeItem( QTreeWidgetItem* pNode,
                                                QPointer<cmbNucMaterial> mat)
: QTreeWidgetItem(pNode), Material(mat)
{
  Connection = new cmbNucMaterialTreeItemConnection;
  Connection->v = this;
  Qt::ItemFlags matFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable |
                         Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
  this->setText(1, mat->getName());
  this->setText(2, mat->getLabel());
  QBrush bgBrush(mat->getColor());
  this->setBackground(3, bgBrush);
  this->setFlags(matFlags);
  this->setCheckState(0, Qt::Checked);
  //QObject::connect(mat, SIGNAL(invalidName()), Connection, SLOT(revert()));
  //QObject::connect(mat, SIGNAL(invalidLabel()), Connection, SLOT(revert()));
}

//-----------------------------------------------------------------------------
cmbNucMaterialTreeItem::~cmbNucMaterialTreeItem()
{
  delete this->Connection;
  //TODO tell color manager to delete material
}

//-----------------------------------------------------------------------------
void cmbNucMaterialTreeItem::setText ( int column, const QString & text )
{
  //qDebug() << "1 set Text: " << column << text << this->text( column );
  this->QTreeWidgetItem::setText(column, text);
  //qDebug() << "2 set Text: " << column << text << this->text( column );
  if(column == 0)
    {
    this->PreviousText = text;
    }
}

//-----------------------------------------------------------------------------
void cmbNucMaterialTreeItem::setData ( int column, int role,
                                       const QVariant & value )
{
  QVariant sv = value;
  if(column == 1 && role == Qt::EditRole)
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
  else if(column == 3 && role == Qt::EditRole)
  {
    Material->setColor(value.value<QColor>());
  }
  this->QTreeWidgetItem::setData(column, role, sv);
}
