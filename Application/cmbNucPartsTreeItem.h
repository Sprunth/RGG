
#ifndef __cmbNucPartsTreeItem_h
#define __cmbNucPartsTreeItem_h

#include <QTreeWidgetItem>
#include "cmbNucPartDefinition.h"

class  cmbNucPartsTreeItem;

class cmbNucPartsTreeItemConnection: public QObject
{
  Q_OBJECT
public:
  cmbNucPartsTreeItemConnection()
  {}
  virtual ~cmbNucPartsTreeItemConnection()
  {}
  cmbNucPartsTreeItem * v;
signals:

public slots:
  void checkSaveAndGenerate();
};

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

  QVariant data( int index, int role ) const;

  // column text
  virtual void setData ( int column, int role, const QVariant & value );
  void setText ( int column, const QString & text );
  const QString& previousText() const{return this->PreviousText;}
  void setHightlights(bool fileChange, bool needGeneration);

  void checkSaveAndGenerate();

  cmbNucPartsTreeItemConnection * connection;
  bool fileChanged() const;
  bool needGeneration() const;

protected:
  bool FileChanged, NeedGeneration;
  AssyPartObj* PartObject;
  QString PreviousText;
};

#endif /* __cmbNucPartsTreeItem_h */
