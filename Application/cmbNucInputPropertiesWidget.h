#ifndef __cmbNucInputPropertiesWidget_h
#define __cmbNucInputPropertiesWidget_h

#include <QWidget>
#include "cmbNucPartDefinition.h"
#include <QStringList>
#include <QPointer>

class cmbNucInputPropertiesWidgetInternal;
class cmbNucAssemblyEditor;
class cmbNucMainWindow;
class cmbNucAssembly;
class cmbNucCore;
class PinCell;
class Frustum;
class Cylinder;
class Duct;
class DuctCell;
class cmbNucHexLattice;
class cmbCoreParametersWidget;
class cmbAssyParametersWidget;
class cmbNucDefaultWidget;

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

signals:
  // Description:
  // Fired when the current object is modified
  void objGeometryChanged(AssyPartObj* selObj);
  void currentObjectNameChanged(const QString& name);
  void sendLabelChange(const QString);
  void badPinLabel(QString prevL);
  void badPinName(QString prev);
  void valuesChanged();
  void resetView();

public slots:
  void resetCore(cmbNucCore* nucCore);
  void resetAssemblyEditor(cmbNucAssembly* assembly);
  void pinLabelChanged(PinCell*, QString previous, QString current);
  void pinNameChanged(PinCell*, QString previous, QString current);

protected slots:
  // Invoked when Apply button clicked
  void onApply();
  // Invoked when Reset button clicked
  void onReset();
  // reset property panel with given object
  void resetPinCell(PinCell* pincell);
  void resetDuct(Duct* duct);
  void resetLattice(Lattice* lattice);
  void resetAssemblyLattice();
  void resetAssembly(cmbNucAssembly* assy);

  // apply property panel to given object
  void applyToPinCell(PinCell* pincell);
  void applyToDuct(Duct* duct);
  void applyToLattice(Lattice* lattice);
  void applyToAssembly(cmbNucAssembly* assy);
  void applyToCore(cmbNucCore* nucCore);

  // Slot for Lattice dimensions
  void onLatticeDimensionChanged();
  void onCoreDimensionChanged();

  void showPinCellEditor();

  void onCoreLayersChanged();
  void onAssyLayersChanged();
  void choosePinLegendColor();
  void chooseAssyLegendColor();

  void addDuctLayerBefore();
  void addDuctLayerAfter();
  void deleteDuctLayer();

private:
  void setUpDuctTable(bool isHex, Duct* duct);
  bool setDuctValuesFromTable(Duct* duct);
  void addDuctLayerSetRowValue(int row);
  bool RebuildCoreGrid;
  cmbNucInputPropertiesWidgetInternal* Internal;

  void initUI();
  AssyPartObj* CurrentObject;
  cmbNucAssemblyEditor *AssemblyEditor;
  cmbNucAssemblyEditor *CoreEditor;
  cmbNucAssembly *Assembly;
  cmbNucCore *Core;
  cmbNucMainWindow *MainWindow;
  cmbNucHexLattice* HexCore;
  cmbNucHexLattice* HexAssy;
  cmbCoreParametersWidget* HexCoreProperties;
  cmbCoreParametersWidget* RectCoreProperties;
  cmbAssyParametersWidget* assyConf;
  QPointer<cmbNucDefaultWidget> hexCoreDefaults;
  QPointer<cmbNucDefaultWidget> rectCoreDefaults;
  QPointer<cmbNucDefaultWidget> assyDefaults;
};
#endif
