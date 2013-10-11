#ifndef __cmbNucPinCellEditor_h
#define __cmbNucPinCellEditor_h

#include <QWidget>
#include <vtkActor.h>
#include <vtkCompositePolyDataMapper2.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkClipClosedSurface.h>

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

  void SetPinCell(PinCell *pincell);
  PinCell* GetPinCell();

  void SetAssembly(cmbNucAssembly *assembly) { this->AssemblyObject = assembly; }

public slots:
  void Apply();
  void UpdatePinCell();
  void UpdatePolyData();
  void UpdateRenderView();
  void onUpdateLayerMaterial();
  void setZScale(double scale);

signals:
  void accepted();
  void rejected();

private slots:
  void tableCellChanged(int row, int col);
  void addComponent();
  void deleteComponent();
  void numberOfLayersChanged(int layers);
  void sectionTypeComboBoxChanged(const QString &type);
  void setupMaterialComboBox(QComboBox *comboBox);
  void layerTableCellChanged(int row, int col);
  void UpdateLayerMaterials(AssyPartObj* objPart);
  void onPieceSelected();
  AssyPartObj* createComponentObject(int i, double& z);
  void updateComponentObject(int i, double& z);
  void createComponentItem(int row, double default_length,
    double default_radius1, double default_radius2);
  void onCutAwayCheckBoxToggled(bool state);

private:
  AssyPartObj* getSelectedPiece();

  Ui::cmbNucPinCellEditor *Ui;
  PinCell *PinCellObject;
  cmbNucAssembly *AssemblyObject;
  vtkSmartPointer<vtkActor> Actor;
  vtkSmartPointer<vtkCompositePolyDataMapper2> Mapper;
  vtkSmartPointer<vtkRenderer> Renderer;
};

#endif // __cmbNucPinCellEditor_h
