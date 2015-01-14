#ifndef __cmbNucInputListWidget_h
#define __cmbNucInputListWidget_h

#include <QWidget>
#include <QTreeWidgetItem>
#include "cmbNucPartDefinition.h"

class cmbNucAssembly;
class cmbNucCore;
class cmbNucPinLibrary;
class cmbNucDuctLibrary;
class AssyPartObj;
class cmbNucInputListWidgetInternal;
class QTreeWidget;
class QTreeWidgetItem;
class cmbNucPartsTreeItem;
class PinCell;
class DuctCell;
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
  void majorMeshSelection(int);

  void sendColorControl(int);
  void sendEdgeControl(bool);
  void resetMeshCamera();

  void raiseMeshDock();
  void raiseModelDock();

public slots:
  void onNewAssembly();
  void valueChanged();
  void onRemoveSelectedPart();
  void meshIsLoaded(bool);
  void modelIsLoaded(bool);
  void selectMeshTab(bool);
  void selectModelTab(bool);
  void updateMainMeshComponents(QStringList parts, int select);
  void updateMeshTable(QList<QTreeWidgetItem*> MeshParts);

protected:
  cmbNucPartsTreeItem* getSelectedItem(QTreeWidget* treeWidget);
  void fireObjectSelectedSignal(cmbNucPartsTreeItem* selItem);
  void updateContextMenu(AssyPartObj* selObj);
  void setActionsEnabled(bool val);
  void updateWithAssembly(cmbNucAssembly* assy, bool select=true);
  void updateWithPinLibrary(cmbNucPinLibrary * pl);
  void updateWithPin(PinCell * pc, bool select=false);
  void updateWithDuctLibrary(cmbNucDuctLibrary * dl);
  void updateWithDuct(DuctCell * dc, bool select=false);
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
  void onNewDuct();

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
