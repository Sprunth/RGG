
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
  virtual void setPartObject(AssyPartObj* obj)
    {this->PartObject=obj;}

  // column text
  virtual void setData ( int column, int role, const QVariant & value );
  void setText ( int column, const QString & text );
  const QString& previousText() const{return this->PreviousText;}
protected:

  AssyPartObj* PartObject;
  QString PreviousText;
};

#endif /* __cmbNucPartsTreeItem_h */
