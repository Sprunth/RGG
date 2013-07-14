
#ifndef __cmbNucPartsTreeItem_h
#define __cmbNucPartsTreeItem_h

#include <QTreeWidgetItem>
#include "cmbNucPartDefinition.h"

class  cmbNucPartsTreeItem : public QTreeWidgetItem
{
public:         
  cmbNucPartsTreeItem(QTreeWidgetItem* pNode, AssyPartObj* );
  virtual ~cmbNucPartsTreeItem();
  
  // Description:
  // Get the assembly part object  
  virtual AssyPartObj* getPartObject()
    {return this->PartObject;}

protected:

  AssyPartObj* PartObject;
};

#endif /* __cmbNucPartsTreeItem_h */
