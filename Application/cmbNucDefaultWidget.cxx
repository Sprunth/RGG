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

void cmbNucDefaultWidget::set(QPointer<cmbNucDefaults> c, bool isCore, bool isHex)
{
  this->disConnect();
  this->Current = c;
  this->Internal->ui->AxialMeshLayout->setVisible(isCore);
  this->Internal->ui->DuctThicknessLayout->setVisible(isCore);
  this->Internal->ui->ductX->setVisible(!isHex);
  this->Internal->ui->ductY->setVisible(!isHex);
  this->Internal->ui->DuctThickY->setVisible(!isHex);
  this->Internal->ui->ductForce->setVisible(false);
  this->Internal->ui->EdgeIntervalLayout->setVisible(isCore);
  this->Internal->ui->HeightLayout->setVisible(!isCore);
  this->Internal->ui->HeightForce->setVisible(false);
  this->Internal->ui->MeshTypeLayout->setVisible(isCore);
  this->Internal->ui->PinRadiusLayout->setVisible(!isCore);
  this->Internal->ui->pinRadiusForce->setVisible(false);
  this->Internal->ui->PitchLayout->setVisible(!isCore);
  this->Internal->ui->pitchXLabel->setVisible(!isHex);
  this->Internal->ui->pitchYLabel->setVisible(!isHex);
  this->Internal->ui->PitchY->setVisible(!isHex);
  this->Internal->ui->pitchForce->setVisible(false);
  this->Internal->ui->RadialMeshLayout->setVisible(isCore);
  this->Internal->ui->RotateLayout->setVisible(isCore);
  this->setConnections();
  this->reset();
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

  bool getValue(int &to, QLineEdit * from)
  {
    QString v = from->text();
    if(v.isEmpty())
    {
      return false;
    }
    bool ok;
    to = v.toInt(&ok);
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

  void getValue(QLineEdit * to, int &from)
  {
    QString tmp = QString::number(from);
    if(tmp.isEmpty()) return;
    to->setText(tmp);
  }

  void getValue(QLineEdit * to, QString from)
  {
    to->setText(from);
  }

  bool getValue(QString &to, QComboBox * from)
  {
    to = from->currentText();
    return !to.isEmpty();
  }

  void getValue(QComboBox * to, QString from)
  {
    to->setCurrentIndex(to->findText(from, Qt::MatchFixedString));
  }
}

void cmbNucDefaultWidget::apply()
{
  if(Current == NULL) return;
#define FUN1(T,X)                                   \
{                                                   \
  T tmp##X;                                         \
  bool v = getValue(tmp##X, this->Internal->ui->X); \
  if(v){ Current->set##X(tmp##X); }                 \
}
#define FUN2(T1, X, T2, Y, L)                       \
{                                                   \
  T1 tmp##X; T2 tmp##Y;                             \
  bool v = getValue(tmp##X, this->Internal->ui->X); \
  v &= getValue(tmp##Y, this->Internal->ui->Y);     \
  if(v){ Current->set##L(tmp##X, tmp##Y); }         \
}
  EASY_DEFAULT_PARAMS_MACRO()
#undef FUN1
#undef FUN2
  this->reset();
}

void cmbNucDefaultWidget::reset()
{
  if(Current == NULL) return;
#define FUN1(T,X)                                 \
{                                                 \
  T tmp##X;                                       \
  if(Current->get##X(tmp##X))                     \
  {                                               \
    getValue(this->Internal->ui->X,tmp##X);       \
  }                                               \
  else                                            \
  {                                               \
    getValue(this->Internal->ui->X, QString("")); \
  }                                               \
}

#define FUN2(T1, X, T2, Y, L)                     \
{                                                 \
  T1 tmp##X; T2 tmp##Y;                           \
  if(Current->get##L(tmp##X, tmp##Y))             \
  {                                               \
    getValue(this->Internal->ui->X,tmp##X);       \
    getValue(this->Internal->ui->Y,tmp##Y);       \
  }                                               \
  else                                            \
  {                                               \
    getValue(this->Internal->ui->X, QString("")); \
    getValue(this->Internal->ui->Y, QString("")); \
  }                                               \
}
  EASY_DEFAULT_PARAMS_MACRO()
#undef FUN1
#undef FUN2

}

void cmbNucDefaultWidget::setConnections()
{
  if(this->Current == NULL) return;
  QObject::connect(this->Internal->ui->computePitch, SIGNAL(clicked()),
                   this->Current, SIGNAL(calculatePitch()));
  QObject::connect(this->Internal->ui->calculatePinRadius, SIGNAL(clicked()),
                   this->Current, SIGNAL(calculatePinRadius()));

  QObject::connect(this->Current, SIGNAL(recieveCalculatedPitch(double, double)),
                   this,          SLOT(recievePitch(double, double)));
  QObject::connect(this->Current, SIGNAL(recieveRadius(double)),
                   this,          SLOT(recieveRadius(double)));
}

void cmbNucDefaultWidget::disConnect()
{
  if(this->Current == NULL) return;
  QObject::disconnect(this->Internal->ui->computePitch, SIGNAL(clicked()),
                      this->Current, SIGNAL(calculatePitch()));
  QObject::disconnect(this->Internal->ui->calculatePinRadius, SIGNAL(clicked()),
                      this->Current, SIGNAL(calculatePinRadius()));
  QObject::disconnect(this->Current, SIGNAL(recieveCalculatedPitch(double, double)),
                      this,          SLOT(recievePitch(double, double)));
  QObject::disconnect(this->Current, SIGNAL(recieveRadius(double)),
                      this,          SLOT(recieveRadius(double)));
}

void cmbNucDefaultWidget::recievePitch(double x, double y)
{
  if(x<0||y<0)
  {
    double tmpX; double tmpY;
    if(Current->getPitch(tmpX, tmpY))
    {
      this->Internal->ui->PitchX->setText(QString::number(tmpX));
      this->Internal->ui->PitchY->setText(QString::number(tmpY));
    }
    else
    {
      this->Internal->ui->PitchX->setText("");
      this->Internal->ui->PitchY->setText("");
    }
  }
  else
  {
    this->Internal->ui->PitchX->setText(QString::number(x));
    this->Internal->ui->PitchY->setText(QString::number(y));
  }
}

void cmbNucDefaultWidget::recieveRadius(double r)
{
  if(r<0)
  {
    double tmpX;
    if(Current->getPinRadius(tmpX))
    {
      this->Internal->ui->PinRadius->setText(QString::number(tmpX));
    }
    else
    {
      getValue(this->Internal->ui->PinRadius, QString(""));
    }
  }
  this->Internal->ui->PinRadius->setText(QString::number(r));
}
