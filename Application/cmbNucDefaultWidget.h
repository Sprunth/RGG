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
  bool assyPitchChanged();
public slots:
  void set(QPointer<cmbNucDefaults> c, bool isCore, bool isHex);
  void apply();
  void reset();
  void recievePitch(double, double);
  void setPitchAvail(bool);
signals:
  void commonChanged();
protected:
  void setConnections();
  void disConnect();

  QPointer<cmbNucDefaults> Current;
  cmbNucDefaultWidgetInternal * Internal;

};
#endif
