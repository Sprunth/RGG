
#include "cmbNucPartsTreeItem.h"

//-----------------------------------------------------------------------------
cmbNucPartsTreeItem::cmbNucPartsTreeItem(
  QTreeWidgetItem* pNode, AssyPartObj* obj)
: QTreeWidgetItem(pNode), PartObject(obj)
{
}

//-----------------------------------------------------------------------------
cmbNucPartsTreeItem::~cmbNucPartsTreeItem()
{
}
//-----------------------------------------------------------------------------
void cmbNucPartsTreeItem::setText ( int column, const QString & text )
{
  this->QTreeWidgetItem::setText(column, text);
  if(column == 0)
    {
    this->PreviousText = text;
    }
}
//-----------------------------------------------------------------------------
void cmbNucPartsTreeItem::setData (
  int column, int role, const QVariant & value )
{
  if(column == 0 && role == Qt::EditRole) // keep the original text
    {
    this->PreviousText = this->text(column);
    }
  this->QTreeWidgetItem::setData(column, role, value);
}
