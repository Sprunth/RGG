#ifndef __cmbNucInputListWidget_h
#define __cmbNucInputListWidget_h

#include <QWidget>
#include "cmbNucPartDefinition.h"

class cmbNucInputListWidgetInternal;

class cmbNucInputListWidget : public QWidget
{
  Q_OBJECT

public:
  cmbNucInputListWidget(QWidget* parent=0);
  virtual ~cmbNucInputListWidget();
  
signals:
  // Description:
  // Fired when the tab is switched
  void partTypeSwitched(enumNucParts enType);

  // Description:
  // Fired when the tab is switched
  void partSelected(enumNucParts enType);
  
public slots:

private slots:

  // Description:
  // Called when the qt widget changes, we mark undo set
  // and push the widget changes to the property.
  void onQtWidgetChanged();
  
private:
  cmbNucInputListWidgetInternal* Internal;

};
#endif

