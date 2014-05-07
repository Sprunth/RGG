#ifndef __cmbNucPinCellEditor_h
#define __cmbNucPinCellEditor_h

#include <QWidget>
#include <vtkSmartPointer.h>
#include "vtkMultiBlockDataSet.h"

#include "ui_cmbNucPinCellEditor.h"

class QComboBox;
class PinCell;
class PinSubPart;
class cmbNucAssembly;
class AssyPartObj;

class CellConnection : public QObject
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
class cmbNucPinCellEditor : public QWidget
{
  Q_OBJECT

public:
  cmbNucPinCellEditor(QWidget *parent = 0);
  ~cmbNucPinCellEditor();

  void SetPinCell(PinCell *pincell, bool hex);
  PinCell* GetPinCell();

  void SetAssembly(cmbNucAssembly *assembly);

signals:
  void pincellModified(AssyPartObj*);
  void labelChanged(PinCell*, QString prev, QString current);
  void nameChanged(PinCell*, QString prev, QString current);
  void valueChange();
  void resetView();

public slots:
  void Apply();
  void Reset();
  void UpdatePinCell();
  void UpdateData();
  void onUpdateLayerMaterial();
  void badLabel(QString);
  void badName(QString);
  void calculatePitch();

private slots:
  void tableCellChanged();
  void addComponent();
  void deleteComponent();
  void addLayerBefore();
  void addLayerAfter();
  void deleteLayer();
  void sectionTypeComboBoxChanged(const QString &type);
  void setupMaterialComboBox(QComboBox *comboBox);
  void onPieceSelected();
  PinSubPart* createComponentObject(int i, PinSubPart * );
  void createComponentItem(int row, PinSubPart *);

private:
  PinSubPart* getSelectedPiece();
  void rebuildLayersFromTable();

  void createMaterialRow(int row, PinSubPart *);

  Ui::cmbNucPinCellEditor *Ui;
  PinCell *InternalPinCell;
  PinCell *ExternalPinCell;
  cmbNucAssembly *AssemblyObject;
  bool isHex;
};

#endif // __cmbNucPinCellEditor_h
