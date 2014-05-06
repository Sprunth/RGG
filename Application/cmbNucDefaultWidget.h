#ifndef __cmbNucDefaultWidget_h
#define __cmbNucDefaultWidget_h

#include <QWidget>
#include "cmbNucPartDefinition.h"
#include <QPointer>

class cmbNucDefaultWidgetInternal;
class cmbNucDefaults;

class cmbNucDefaultWidget : public QWidget
{
  Q_OBJECT

public:
  cmbNucDefaultWidget(QWidget *parent = 0);
  ~cmbNucDefaultWidget();
public slots:
  void set(QPointer<cmbNucDefaults> c, bool isCore, bool isHex);
  void apply();
  void reset();
  void recievePitch(double, double);
protected:
  void setConnections();
  void disConnect();

  QPointer<cmbNucDefaults> Current;
  cmbNucDefaultWidgetInternal * Internal;

};
#endif