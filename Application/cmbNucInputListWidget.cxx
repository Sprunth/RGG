
#include "cmbNucInputListWidget.h"
#include "ui_qInputListWidget.h"

#include <QLabel>
#include <QPointer>

class cmbNucInputListWidgetInternal : 
  public Ui::InputListWidget
{
public:
};

//-----------------------------------------------------------------------------
cmbNucInputListWidget::cmbNucInputListWidget(
  QWidget* _p): QWidget(_p)
{
  this->Internal = new cmbNucInputListWidgetInternal;
  this->Internal->setupUi(this);
}

//-----------------------------------------------------------------------------
cmbNucInputListWidget::~cmbNucInputListWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::onQtWidgetChanged()
{
}
