#ifndef __cmbNucInputListWidget_h
#define __cmbNucInputListWidget_h

#include <QWidget>
#include "cmbNucPartDefinition.h"

class cmbNucAssembly;
class cmbNucInputListWidgetInternal;
class QTreeWidget;
class QTreeWidgetItem;
class cmbNucPartsTreeItem;

class cmbNucInputListWidget : public QWidget
{
  Q_OBJECT

public:
  cmbNucInputListWidget(QWidget* parent=0);
  virtual ~cmbNucInputListWidget();

  /// set/get the assembly that this widget with be interact with
  void setAssembly(cmbNucAssembly*);
  cmbNucAssembly* getAssembly();

signals:
  // Description:
  // Fired when a part/material is selected in the tree
  void objectSelected(AssyPartObj*, const char* name);
  void objectRemoved();

private slots:
  // Description:
  // Tree widget interactions related slots 
  virtual void onPartsSelectionChanged();
  virtual void onMaterialSelectionChanged();
  virtual void onMaterialNameChanged(QTreeWidgetItem*, int);
  virtual void onTabChanged(int);

  // Description:
  // Tree widget context menu related slots
  void onNewCylinder();
  void onNewDuct();
  void onNewFrustum();
  void onNewPin();
  void onRemoveSelectedPart();
  void onNewMaterial();
  void onRemoveMaterial();

protected:
  cmbNucPartsTreeItem* getSelectedItem(QTreeWidget* treeWidget);
  void fireObjectSelectedSignal(cmbNucPartsTreeItem* selItem);
  void updateContextMenu(AssyPartObj* selObj);
  void setActionsEnabled(bool val);

private:
  cmbNucInputListWidgetInternal* Internal;

  /// clear UI
  void initUI();
  void initTree(QTreeWidget* treeWidget);
  void updateUI();

  cmbNucAssembly *Assembly;
};
#endif
