
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
