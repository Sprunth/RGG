
#include "cmbNucInputPropertiesWidget.h"
#include "ui_qInputPropertiesWidget.h"

#include <QLabel>
#include <QPointer>

class cmbNucInputPropertiesWidgetInternal : 
  public Ui::InputPropertiesWidget
{
public:
};

//-----------------------------------------------------------------------------
cmbNucInputPropertiesWidget::cmbNucInputPropertiesWidget(
  QWidget* _p): QWidget(_p)
{
  this->Internal = new cmbNucInputPropertiesWidgetInternal;
  this->Internal->setupUi(this);
}

//-----------------------------------------------------------------------------
cmbNucInputPropertiesWidget::~cmbNucInputPropertiesWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::setLabelText(
  const char* labelText)
{
  this->Internal->labelInput->setText(labelText);
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::onQtWidgetChanged()
{
}
