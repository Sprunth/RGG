#ifndef __cmbNucPinCellEditor_h
#define __cmbNucPinCellEditor_h

#include <QWidget>

#include <vtkActor.h>
#include <vtkCompositePolyDataMapper2.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

#include "ui_cmbNucPinCellEditor.h"

class QComboBox;
class PinCell;
class cmbNucAssembly;

class cmbNucPinCellEditor : public QWidget
{
  Q_OBJECT

public:
  cmbNucPinCellEditor(QWidget *parent = 0);
  ~cmbNucPinCellEditor();

  void SetPinCell(PinCell *pincell);
  PinCell* GetPinCell();

  void SetAssembly(cmbNucAssembly *assembly) { this->AssemblyObject = assembly; }

  void UpdatePinCell();
  void UpdatePolyData();
  void UpdateRenderView();

public slots:
  void Apply();

signals:
  void accepted();
  void rejected();

private slots:
  void tableCellChanged(int row, int col);
  void tableItemChanged(QTableWidgetItem *item);
  void addComponent();
  void deleteComponent();
  void numberOfLayersChanged(int layers);
  void sectionTypeComboBoxChanged(const QString &type);
  void setupMaterialComboBox(QComboBox *comboBox);
  void layerTableCellChanged(int row, int col);

private:
  Ui::cmbNucPinCellEditor *Ui;
  PinCell *PinCellObject;
  cmbNucAssembly *AssemblyObject;
  vtkSmartPointer<vtkActor> Actor;
  vtkSmartPointer<vtkCompositePolyDataMapper2> Mapper;
  vtkSmartPointer<vtkRenderer> Renderer;
};

#endif // __cmbNucPinCellEditor_h
