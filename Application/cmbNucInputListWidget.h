#ifndef __cmbNucInputListWidget_h
#define __cmbNucInputListWidget_h

#include <QWidget>
#include "cmbNucPartDefinition.h"

class cmbNucAssembly;
class cmbNucCore;
class AssyPartObj;
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
  /// get selected part object
  AssyPartObj* getSelectedPart();
  cmbNucPartsTreeItem* getSelectedPartNode();

  QTreeWidget * getModelTree();

signals:
  // Description:
  // Fired when a part/material is selected in the tree
  void objectSelected(AssyPartObj*, const char* name);
  void objectRemoved();
  void materialColorChanged(const QString& name);
  void materialVisibilityChanged(const QString& name);

  void pinsModified(cmbNucAssembly*);
  void assembliesModified(cmbNucCore*);
  void switchToModelTab();
  void switchToNonModelTab();

  void deleteCore();

public slots:
  void onNewAssembly();

protected:
  cmbNucPartsTreeItem* getSelectedItem(QTreeWidget* treeWidget);
  void fireObjectSelectedSignal(cmbNucPartsTreeItem* selItem);
  void updateContextMenu(AssyPartObj* selObj);
  void setActionsEnabled(bool val);
  void updateWithAssembly(cmbNucAssembly* assy, bool select=true);
  cmbNucPartsTreeItem* getCurrentAssemblyNode();
  cmbNucPartsTreeItem* getDuctCellNode(cmbNucPartsTreeItem* assyNode);
  void initCoreRootNode();
  void createMaterialItem(
    const QString& name, const QString& label, const QColor& color);

private slots:
  // Description:
  // Tree widget interactions related slots
  virtual void onPartsSelectionChanged();
  virtual void onMaterialSelectionChanged();
  virtual void onMaterialChanged(QTreeWidgetItem*, int col);
  virtual void onTabChanged(int);
  void onMaterialClicked(QTreeWidgetItem*, int col);
  void onDeleteAssembly(QTreeWidgetItem*);

  // Description:
  // Tree widget context menu related slots
  void onNewCylinder();
  void onNewDuct();
  void onNewFrustum();
  void onNewPin();
  void onRemoveSelectedPart();
  void onNewMaterial();
  void onRemoveMaterial();
  void onImportMaterial();
  void onSaveMaterial();

signals:
  void deleteAssembly(QTreeWidgetItem*);

private:
  cmbNucInputListWidgetInternal* Internal;

  /// clear UI
  void initUI();
  void initPartsTree();
  void initMaterialsTree();

  cmbNucCore *NuclearCore;
};
#endif
