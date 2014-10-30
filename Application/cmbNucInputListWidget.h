#ifndef __cmbNucInputListWidget_h
#define __cmbNucInputListWidget_h

#include <QWidget>
#include <QTreeWidgetItem>
#include "cmbNucPartDefinition.h"

class cmbNucAssembly;
class cmbNucCore;
class AssyPartObj;
class cmbNucInputListWidgetInternal;
class QTreeWidget;
class QTreeWidgetItem;
class cmbNucPartsTreeItem;
class QMenu;

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
  AssyPartObj* getSelectedCoreOrAssembly();
  cmbNucPartsTreeItem* getSelectedPartNode();

  bool onlyMeshLoaded();

  void clear();

  void setToModel();

  void clearTable();

  void setPartOptions(QMenu * qm) const;

  void initMaterialsTree();

signals:
  // Description:
  // Fired when a part/material is selected in the tree
  void objectSelected(AssyPartObj*, const char* name);
  void objectRemoved();

  void pinsModified(cmbNucAssembly*);
  void assembliesModified(cmbNucCore*);

  void deleteCore();

  void checkSavedAndGenerate();

  void pincellDeleted();
  void subMeshSelected(QTreeWidgetItem*);
  void meshValueChanged(QTreeWidgetItem*);

  void sendColorControl(bool);
  void sendEdgeControl(bool);

public slots:
  void onNewAssembly();
  void valueChanged();
  void onRemoveSelectedPart();
  void meshIsLoaded(bool);
  void updateMeshTable(QList<QTreeWidgetItem*> MeshParts);

protected:
  cmbNucPartsTreeItem* getSelectedItem(QTreeWidget* treeWidget);
  void fireObjectSelectedSignal(cmbNucPartsTreeItem* selItem);
  void updateContextMenu(AssyPartObj* selObj);
  void setActionsEnabled(bool val);
  void updateWithAssembly(cmbNucAssembly* assy, bool select=true);
  cmbNucPartsTreeItem* getCurrentAssemblyNode();
  void initCoreRootNode();
  void createMaterialItem( const QString& name, const QString& label,
                           const QColor& color );
  void assemblyModified(cmbNucPartsTreeItem* assyNode);
  void coreModified();


private slots:
  // Description:
  // Tree widget interactions related slots
  virtual void onPartsSelectionChanged();
  virtual void onTabChanged(int);
  void onMaterialClicked(QTreeWidgetItem*, int col);
  void onDeleteAssembly(QTreeWidgetItem*);
  void labelChanged(QString);

  // Description:
  // Tree widget context menu related slots
  void onNewPin();
  void onImportMaterial();
  void onSaveMaterial();

  void repaintList();
  void hideLabels(bool);

signals:
  void deleteAssembly(QTreeWidgetItem*);

private:
  cmbNucInputListWidgetInternal* Internal;

  /// clear UI
  void initUI();
  void initPartsTree();

  cmbNucCore *NuclearCore;
};
#endif
