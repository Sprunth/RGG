#include "cmbNucDefaultWidget.h"
#include "cmbNucDefaults.h"
#include "ui_qDefaults.h"
#include <QDebug>

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
  bool isHex;
};

cmbNucDefaultWidget::cmbNucDefaultWidget(QWidget *p)
:QWidget(p)
{
  this->Internal = new cmbNucDefaultWidgetInternal();
  this->Internal->ui->setupUi(this);
}

cmbNucDefaultWidget::~cmbNucDefaultWidget()
{
  delete Internal;
}

void cmbNucDefaultWidget::set(QPointer<cmbNucDefaults> c, bool isHex)
{
  this->disConnect();
  this->Current = c;
  this->Internal->ui->ductX->setVisible(!isHex);
  this->Internal->ui->ductY->setVisible(!isHex);
  this->Internal->ui->DuctThickY->setVisible(!isHex);
  this->Internal->isHex = isHex;
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

  bool getValue(QString &v, QPlainTextEdit * from)
  {
    v = from->toPlainText();
    return !v.isEmpty();
  }

  void getValue(QPlainTextEdit * to, QString v)
  {
    to->setPlainText(v);
  }
}

bool cmbNucDefaultWidget::assyPitchChanged()
{
  double tmpDuctThickX; double tmpDuctThickY;
  double cDuctThickX; double cDuctThickY;
  bool v = getValue(tmpDuctThickX, this->Internal->ui->DuctThickX);
  v &= getValue(tmpDuctThickY, this->Internal->ui->DuctThickY);
  if(v && Current->getDuctThickness(cDuctThickX, cDuctThickY))
  {
    return cDuctThickX != tmpDuctThickX || cDuctThickY != tmpDuctThickY;
  }
  return false;
}

#define COMMONMACRO() \
COMMON(double, AxialMeshSize) \
COMMON(int, EdgeInterval)\
COMMON(QString, MeshType) \
COMMON(QString, UserDefined) \

bool cmbNucDefaultWidget::apply()
{
  bool changed = false;
  if(Current == NULL) return changed;
#define COMMON(T,X) \
{ \
  T tmp1, tmp2; \
  bool v1 = getValue(tmp1, this->Internal->ui->X);   \
  bool v2 = Current->get##X(tmp2); \
  if((v1 != v2) || (v1 && tmp1 != tmp2) )\
  { emit commonChanged(); changed = true; } \
}
  COMMONMACRO()
#define FUN1(T,X)                                   \
{                                                   \
  T tmp##X;                                         \
  bool v = getValue(tmp##X, this->Internal->ui->X); \
  if(v){ Current->set##X(tmp##X); }                 \
  else { Current->clear##X(); }                     \
}
#define FUN2(T1, X, T2, Y, L)                       \
{                                                   \
  T1 tmp##X; T2 tmp##Y;                             \
  bool v = getValue(tmp##X, this->Internal->ui->X); \
  v &= getValue(tmp##Y, this->Internal->ui->Y);     \
  changed &= v != Current->has##L();                \
  T1 tmpA##X; T2 tmpA##Y;                           \
  if(Current->get##L(tmpA##X, tmpA##Y) &&           \
     (tmpA##X != tmp##X || tmpA##Y != tmp##Y))      \
      changed = true;                               \
  if( this->Internal->isHex )  tmp##Y = tmp##X;     \
  if(v){ Current->set##L(tmp##X, tmp##Y); }         \
  else { Current->clear##L(); }                     \
}
  EASY_DEFAULT_PARAMS_MACRO()
#undef COMMON
#undef FUN1
#undef FUN2
  this->reset();
  return changed;
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
}

void cmbNucDefaultWidget::disConnect()
{
  if(this->Current == NULL) return;
}
