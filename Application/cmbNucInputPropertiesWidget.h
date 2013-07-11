#ifndef __cmbNucInputPropertiesWidget_h
#define __cmbNucInputPropertiesWidget_h

#include <QWidget>

class cmbNucInputPropertiesWidgetInternal;

class cmbNucInputPropertiesWidget : public QWidget
{
  Q_OBJECT

public:
  cmbNucInputPropertiesWidget(QWidget* parent=0);
  virtual ~cmbNucInputPropertiesWidget();

  // Description:
  // Set the label text of the widget
  void setLabelText(const char*);
  
signals:
  // Description:
  // Fired when the text in the dropdown box is changed
  void currentTextChanged(const QString&);
  
public slots:

private slots:

  // Description:
  // Called when the qt widget changes, we mark undo set
  // and push the widget changes to the property.
  void onQtWidgetChanged();
  
private:
  cmbNucInputPropertiesWidgetInternal* Internal;

};
#endif

