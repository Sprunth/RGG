#ifndef __cmbNucInputListWidget_h
#define __cmbNucInputListWidget_h

#include <QWidget>
#include "cmbNucPartDefinition.h"

class cmbNucAssembly;
class cmbNucCore;
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

  /// set core this widget with be interact with
  void setCore(cmbNucCore*);
  /// get current assembly that this widget is interacting with
  cmbNucAssembly* getCurrentAssembly();
  /// update UI and if selCore is not set,
  /// the last assembly lattice will be selected by default,
  void updateUI(bool selCore);

signals:
  // Description:
  // Fired when a part/material is selected in the tree
  void objectSelected(AssyPartObj*, const char* name);
  void objectRemoved();

public slots:
  void onNewAssembly();

protected:
  cmbNucPartsTreeItem* getSelectedItem(QTreeWidget* treeWidget);
  void fireObjectSelectedSignal(cmbNucPartsTreeItem* selItem);
  void updateContextMenu(AssyPartObj* selObj);
  void setActionsEnabled(bool val);
  void updateWithAssembly(cmbNucAssembly* assy, bool select=true);
  void updateMaterial(cmbNucAssembly* assy);
  cmbNucPartsTreeItem* getCurrentAssemblyNode();
  cmbNucPartsTreeItem* getDuctCellNode(cmbNucPartsTreeItem* assyNode);

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

private:
  cmbNucInputListWidgetInternal* Internal;

  /// clear UI
  void initUI();
  void initTree(QTreeWidget* treeWidget);

  cmbNucCore *NuclearCore;
};
#endif
