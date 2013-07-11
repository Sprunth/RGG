#ifndef __cmbNucInputListWidget_h
#define __cmbNucInputListWidget_h

#include <QWidget>

class cmbNucInputListWidgetInternal;

class cmbNucInputListWidget : public QWidget
{
  Q_OBJECT

public:
  cmbNucInputListWidget(QWidget* parent=0);
  virtual ~cmbNucInputListWidget();

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
  cmbNucInputListWidgetInternal* Internal;

};
#endif
