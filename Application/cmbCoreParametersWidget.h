#ifndef __cmbCoreParametersWidget_h
#define __cmbCoreParametersWidget_h

#include <QFrame>
#include "cmbNucPartDefinition.h"
#include <QStringList>

class cmbNucCore;

class cmbCoreParametersWidget : public QFrame
{
  Q_OBJECT

public:
  cmbCoreParametersWidget(QWidget* p);
  virtual ~cmbCoreParametersWidget();

  // Description:
  // set/get the Core that this widget with be interact with
  void setCore(cmbNucCore*);
  cmbNucCore* getCore(){return this->Core;}

signals:
  void set(QString v);
  void set(bool b);
  void valuesChanged();
  void drawCylinder(double r, int i);
  void clearCylinder();

public slots:
  // Invoked when Apply button clicked
  void onApply();
  // Invoked when Reset button clicked
  void onReset();
  void onAddToTable();
  void onDeleteRow();
  void onSetBackgroundMesh();
  void onRadiusChanged(double v);
  void onIntervalChanged(int v);
  void onCalculateCylinderDefaults();
  void onDrawCylinder();
  void onClearBackgroundMesh();
  void controlDisplayBackgroundControls();

public slots:  // reset property panel with given object
  void resetCore(cmbNucCore* Core);

  // apply property panel to given object
  void applyToCore(cmbNucCore* Core);

  void addToTable(QString side, QString sid, QString equ);

private:
  class cmbCoreParametersWidgetInternal;
  cmbCoreParametersWidgetInternal* Internal;

  void initUI();
  cmbNucCore *Core;
  double currentRadius;
  int currentInterval;
  double previousRadius;
  int previousInterval;
};
#endif
