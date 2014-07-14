#ifndef __cmbNucDuctCellEditor_h
#define __cmbNucDuctCellEditor_h

#include <QWidget>
#include <vtkSmartPointer.h>
#include "vtkMultiBlockDataSet.h"

#include <vector>

#include "ui_cmbDuctCellEditor.h"

class QComboBox;
class DuctCell;
class Duct;
class cmbNucAssembly;
class AssyPartObj;

class DuctTabelCellConnection : public QObject
{
  Q_OBJECT
public:
public slots:
  void SetValue(double d)
  { this->SetValueImp(d); }
  void NeighborRowDeleted(int r)
  {
    this->NeighborDeleteIMP(r);
  }
signals:
  void ValueChanged(double);
  void Deleted(int);
protected:
  virtual void SetValueImp(double){}
  virtual void NeighborDeleteIMP(int){}
};

// the pin cell editor widget. this dialog allows the user to modify a
// single pin cell in an assembly. the sections (cylinders/frustums) can
// be added/removed/modifed as well as the layers. materials can be assigned
// to each layer via the materials table.
class cmbNucDuctCellEditor : public QWidget
{
  Q_OBJECT

public:
  cmbNucDuctCellEditor(QWidget *parent = 0);
  ~cmbNucDuctCellEditor();

  void SetDuctCell(DuctCell *pincell, bool hex);

  void SetAssembly(cmbNucAssembly *assembly);

  void clear();

signals:
  void display(std::vector<Duct*>);
  void valueChange();

public slots:
  void Apply();
  void Reset();

private slots:
  void ductTableCellSelection();
  void addDuctBefore();
  void addDuctAfter();
  void deleteDuct();
  void addLayerBefore();
  void addLayerAfter();
  void deleteLayer();

private:
  void rebuildLayersFromTable();

  void setDuctRow(int i, Duct * d);

  void fillMaterialTable(Duct * d);

  Ui::cmbDuctCellEditor *Ui;
  DuctCell *InternalDuctCell;
  DuctCell *ExternalDuctCell;
  cmbNucAssembly *AssemblyObject;
  bool isHex;
};

#endif // __cmbNucDuctCellEditor_h
