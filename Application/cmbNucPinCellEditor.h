#ifndef __cmbNucPinCellEditor_h
#define __cmbNucPinCellEditor_h

#include <QWidget>
#include <vtkSmartPointer.h>
#include "vtkMultiBlockDataSet.h"

#include "ui_cmbNucPinCellEditor.h"

class QComboBox;
class PinCell;
class cmbNucAssembly;
class AssyPartObj;

// the pin cell editor widget. this dialog allows the user to modify a
// single pin cell in an assembly. the sections (cylinders/frustums) can
// be added/removed/modifed as well as the layers. materials can be assigned
// to each layer via the materials table.
class cmbNucPinCellEditor : public QWidget
{
  Q_OBJECT

public:
  cmbNucPinCellEditor(QWidget *parent = 0);
  ~cmbNucPinCellEditor();

  void SetPinCell(PinCell *pincell, bool hex);
  PinCell* GetPinCell();

  void SetAssembly(cmbNucAssembly *assembly) { this->AssemblyObject = assembly; }

signals:
  void pincellModified(AssyPartObj*);
  void labelChanged(PinCell*, QString prev, QString current);
  void nameChanged(PinCell*, QString prev, QString current);
  void valueChange();
  void resetView();

public slots:
  void Apply();
  void UpdatePinCell();
  void UpdateData();
  void onUpdateLayerMaterial();
  void badLabel(QString);
  void badName(QString);

private slots:
  void tableCellChanged(int row, int col);
  void addComponent();
  void deleteComponent();
  void addLayerBefore();
  void addLayerAfter();
  void deleteLayer();
  void numberOfLayersChanged(int layers);
  void sectionTypeComboBoxChanged(const QString &type);
  void setupMaterialComboBox(QComboBox *comboBox);
  void layerTableCellChanged(int row, int col);
  void UpdateLayerMaterials();
  void onPieceSelected();
  AssyPartObj* createComponentObject(int i, double& z);
  void updateComponentObject(int i, double& z);
  void createComponentItem(int row, double default_length,
                           double default_radius1,
                           double default_radius2,
                           double x, double y);
  void onCutAwayCheckBoxToggled(bool state);

private:
  AssyPartObj* getSelectedPiece();
  void rebuildLayersFromTable();

  Ui::cmbNucPinCellEditor *Ui;
  PinCell *PinCellObject;
  cmbNucAssembly *AssemblyObject;
  bool isHex;

};

#endif // __cmbNucPinCellEditor_h
