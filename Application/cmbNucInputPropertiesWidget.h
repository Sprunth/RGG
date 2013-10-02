#ifndef __cmbNucInputPropertiesWidget_h
#define __cmbNucInputPropertiesWidget_h

#include <QWidget>
#include "cmbNucPartDefinition.h"
#include <QStringList>

class cmbNucInputPropertiesWidgetInternal;
class cmbNucAssemblyEditor;
class cmbNucMainWindow;
class cmbNucAssembly;
class cmbNucCore;

class cmbNucInputPropertiesWidget : public QWidget
{
  Q_OBJECT

public:
  cmbNucInputPropertiesWidget(cmbNucMainWindow *mainWindow);
  virtual ~cmbNucInputPropertiesWidget();

  // Description:
  // Set the assembly object the widget will be interacting
  void setObject(AssyPartObj* selObj, const char* name);
  AssyPartObj* getObject() {return this->CurrentObject;}

  // Description:
  // set/get the assembly that this widget with be interact with
  void setAssembly(cmbNucAssembly*);
  cmbNucAssembly* getAssembly(){return this->Assembly;}

  // Description:
  // Update material lists
  void updateMaterials();

signals:
  // Description:
  // Fired when the current object is modified
  void currentObjectModified(AssyPartObj* selObj);
  
protected slots:
  // Invoked when Apply button clicked
  void onApply();
  // Invoked when Reset button clicked
  void onReset();
  // reset property panel with given object
  void resetPinCell(PinCell* pincell);
  void resetFrustum(Frustum* frust);
  void resetCylinder(Cylinder* cylin);
  void resetDuct(Duct* duct);
  void resetLattice(Lattice* lattice);
  void resetAssembly(cmbNucAssembly* assy);
  void resetCore(cmbNucCore* nucCore);

  // apply property panel to given object
  void applyToPinCell(PinCell* pincell);
  void applyToFrustum(Frustum* frust);
  void applyToCylinder(Cylinder* cylin);
  void applyToDuct(Duct* duct);
  void applyToLattice(Lattice* lattice);
  void applyToAssembly(cmbNucAssembly* assy);
  void applyToCore(cmbNucCore* nucCore);

  // Slot for duct layers
  void onNumberOfDuctLayersChanged(int numLayers);
  void onCurrentDuctLayerChanged(int idx);
  // the following three works on current duct layer
  void onCurrentDuctMaterialChanged();
  void onDuctThicknessChanged();

  // Slot for Lattice dimensions
  void onLatticeDimensionChanged();
  void onCoreDimensionChanged();

  // reset assembly lattice
  void resetAssemblyLattice();

  void showPinCellEditor();
  void pinCellEditorAccepted();

private:
  cmbNucInputPropertiesWidgetInternal* Internal;

  void initUI();
  AssyPartObj* CurrentObject;
  cmbNucAssemblyEditor *AssemblyEditor;
  cmbNucAssemblyEditor *CoreEditor;
  cmbNucAssembly *Assembly;
  cmbNucMainWindow *MainWindow;
};
#endif
