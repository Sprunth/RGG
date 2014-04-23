
#ifndef __cmbNucMaterialTreeItem_h
#define __cmbNucMaterialTreeItem_h

#include <QTreeWidgetItem>
#include "cmbNucMaterial.h"

class cmbNucMaterialTreeItemConnection;

class  cmbNucMaterialTreeItem : public QTreeWidgetItem
{
public:
  friend class cmbNucMaterialTreeItemConnection;
  cmbNucMaterialTreeItem(QTreeWidgetItem* pNode, QPointer<cmbNucMaterial> );
  virtual ~cmbNucMaterialTreeItem();

  // Description:
  // Get the assembly part object
  virtual QPointer<cmbNucMaterial> getMaterial()
    {return this->Material;}
  virtual void setPartObject(QPointer<cmbNucMaterial> input)
    {this->Material=input;}

  // column text
  virtual void setData ( int column, int role, const QVariant & value );
  void setText ( int column, const QString & text );
  const QString& previousText() const{return this->PreviousText;}
  void setHightlights(bool fileChange, bool needGeneration);

  cmbNucMaterialTreeItemConnection * getConnection()
  { return Connection; }
protected:
  QPointer<cmbNucMaterial> Material;
  QString PreviousText;
  cmbNucMaterialTreeItemConnection * Connection;
};

class cmbNucMaterialTreeItemConnection: public QObject
{
  Q_OBJECT
public:
  cmbNucMaterialTreeItemConnection()
  {}
  virtual ~cmbNucMaterialTreeItemConnection()
  {}
  cmbNucMaterialTreeItem * v;
signals:
public slots:
  void revert();
  void show(bool justUsed);
};


#endif /* __cmbNucPartsTreeItem_h */
