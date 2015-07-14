#ifndef __cmbNucInputPropertiesWidget_h
#define __cmbNucInputPropertiesWidget_h

#include <QWidget>
#include "cmbNucPartDefinition.h"
#include "cmbNucLattice.h"
#include "cmbNucDraw2DLattice.h"
#include <QStringList>
#include <QPointer>

class cmbNucInputPropertiesWidgetInternal;
class cmbNucMainWindow;
class cmbNucAssembly;
class cmbNucAssemblyLink;
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

  bool ductCellIsCrossSectioned();
  bool pinCellIsCrossSectioned();

  void clear();

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
  void sendLattice(LatticeContainer *);
  void apply();
  void reset();
  void sendXSize(int i);
  void sendYSize(int i);
  void select3DModelView();
  void checkSaveAndGenerate();
  void drawCylinder(double r, int i);
  void clearCylinder();

public slots:
  void colorChanged();
  void resetCore(cmbNucCore* nucCore);
  void resetAssemblyEditor(cmbNucAssembly* assembly);
  void clearPincellEditor();
  void pinLabelChanged(PinCell*, QString previous, QString current);
  void pinNameChanged(PinCell*, QString previous, QString current);
  void ductNameChanged(DuctCell*, QString previous, QString current);

protected slots:
  // Invoked when Apply button clicked
  void onApply();
  // Invoked when Reset button clicked
  void onReset();
  // reset property panel with given object
  void resetPinCell(PinCell* pincell);
  void resetLattice(Lattice* lattice);
  void resetAssemblyLattice();
  void resetAssembly(cmbNucAssembly* assy);
  void resetAssemblyLink(cmbNucAssemblyLink* link);

  void xSizeChanged(int i);
  void ySizeChanged(int i);

  void coreXSizeChanged(int i);
  void coreYSizeChanged(int i);

  // apply property panel to given object
  void applyToPinCell(PinCell* pincell);
  void applyToAssembly(cmbNucAssembly* assy);
  void applyToAssemblyLink(cmbNucAssemblyLink* link);
  void applyToCore(cmbNucCore* nucCore);

  void setAutoPitch(bool);

  void computePitch();

  void showPinCellEditor();
  void showDuctCellEditor();

  void chooseLegendColor();

  void onCalculateCylinderDefaults(bool checkOld = false);
  void displayBackgroundControls(int);
  void onRadiusChanged(double v);
  void onIntervalChanged(int v);
  void onDrawCylinder();
  void onSetBackgroundMesh();
  void onClearBackgroundMesh();

private:
  bool RebuildCoreGrid;
  cmbNucInputPropertiesWidgetInternal* Internal;

  void initUI();
  AssyPartObj* CurrentObject;
  cmbNucAssembly *Assembly;
  cmbNucCore *Core;

  cmbNucMainWindow *MainWindow;
  cmbCoreParametersWidget* CoreProperties;
  cmbAssyParametersWidget* assyConf;
  QPointer<cmbNucDefaultWidget> CoreDefaults;

  double currentRadius, previousRadius;
  int currentInterval, previousInterval;
};
#endif
