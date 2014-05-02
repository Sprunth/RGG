#include "cmbNucDefaultWidget.h"
#include "cmbNucDefaults.h"
#include "ui_qDefaults.h"

class cmbNucDefaultWidgetInternal
{
public:
  cmbNucDefaultWidgetInternal()
  {
    ui = new Ui::qDefaults;
  }
  ~cmbNucDefaultWidgetInternal()
  {
    delete ui;
  }
  Ui_qDefaults * ui;
};

cmbNucDefaultWidget::cmbNucDefaultWidget(QWidget *parent)
:QWidget(parent)
{
  this->Internal = new cmbNucDefaultWidgetInternal();
  this->Internal->ui->setupUi(this);
}

cmbNucDefaultWidget::~cmbNucDefaultWidget()
{
  delete Internal;
}

void cmbNucDefaultWidget::set(QPointer<cmbNucDefaults> c, bool isCore)
{
}

namespace
{
  bool getValue(double &to, QLineEdit * from)
  {
    QString v = from->text();
    if(v.isEmpty())
    {
      return false;
    }
    bool ok;
    to = v.toDouble(&ok);
    if(!ok)
    {
      return false;
    }
    return true;
  }

  void getValue(QLineEdit * to, double &from)
  {
    QString tmp = QString::number(from);
    if(tmp.isEmpty()) return;
    to->setText(tmp);
  }

  bool getValue(QString &to, QComboBox * from)
  {
    to = from->currentText();
    return !to.isEmpty();
  }

  void getValue(QComboBox * to, QString &from)
  {
    to->setCurrentIndex(to->findText(from, Qt::MatchFixedString));
  }

}

void cmbNucDefaultWidget::apply()
{
}

void cmbNucDefaultWidget::reset()
{
}

void cmbNucDefaultWidget::setConnections()
{
}

void cmbNucDefaultWidget::disConnect()
{
}
