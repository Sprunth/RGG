#ifndef __cmbAssyParametersWidget_h
#define __cmbAssyParametersWidget_h

#include <QWidget>
#include "cmbNucPartDefinition.h"
#include <QStringList>
#include <cmbNucCore.h>

class cmbNucAssembly;

class cmbAssyParametersWidget : public QWidget
{
  Q_OBJECT

public:
  cmbAssyParametersWidget(QWidget* p);
  virtual ~cmbAssyParametersWidget();

  // Description:
  // set/get the assembly that this widget with be interact with
  void setAssembly(cmbNucAssembly*);
  cmbNucAssembly* getAssembly(){return this->Assembly;}

signals:
  void valuesChanged();

public slots:
  // Invoked when Reset button clicked
  void onReset();

public slots:  // reset property panel with given object
  void resetAssembly(cmbNucAssembly* assy);

  // apply property panel to given object
  void applyToAssembly(cmbNucAssembly* assy, cmbNucCore* core);

private:

  class cmbAssyParametersWidgetInternal;
  cmbAssyParametersWidgetInternal* Internal;

  void initUI();
  cmbNucAssembly *Assembly;
};
#endif
