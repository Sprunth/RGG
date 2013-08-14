#ifndef __cmbNucPinCellEditor_h
#define __cmbNucPinCellEditor_h

#include <QWidget>

#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

#include "ui_cmbNucPinCellEditor.h"

class PinCell;

class cmbNucPinCellEditor : public QWidget
{
  Q_OBJECT

public:
  cmbNucPinCellEditor(QWidget *parent = 0);
  ~cmbNucPinCellEditor();

  void SetPinCell(PinCell *pincell);
  PinCell* GetPinCell();

  void UpdatePinCell();
  void UpdatePolyData();

public slots:
  void Apply();

signals:
  void accepted();
  void rejected();

private slots:
  void addComponent();
  void deleteComponent();

private:
  Ui::cmbNucPinCellEditor *Ui;
  PinCell *PinCellObject;
  vtkSmartPointer<vtkActor> Actor;
  vtkSmartPointer<vtkPolyDataMapper> Mapper;
  vtkSmartPointer<vtkRenderer> Renderer;
};

#endif // __cmbNucPinCellEditor_h
