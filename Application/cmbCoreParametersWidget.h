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

public slots:
  // Invoked when Apply button clicked
  void onApply();
  // Invoked when Reset button clicked
  void onReset();
  void onAddToTable();
  void onDeleteRow();

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
};
#endif
