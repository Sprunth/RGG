
#include "cmbAssyParametersWidget.h"
#include "ui_qAssyParameters.h"

#include "cmbNucAssembly.h"

#include <QLabel>
#include <QPointer>
#include <QtDebug>
#include <QDoubleSpinBox>

class cmbAssyParametersWidget::cmbAssyParametersWidgetInternal :
  public Ui::qAssyParametersWidget
{
public:

};

//-----------------------------------------------------------------------------
cmbAssyParametersWidget::cmbAssyParametersWidget(QWidget *p)
  : QWidget(p)
{
  this->Internal = new cmbAssyParametersWidgetInternal;
  this->Internal->setupUi(this);
  this->initUI();
  this->Assembly = NULL;
}

//-----------------------------------------------------------------------------
cmbAssyParametersWidget::~cmbAssyParametersWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void cmbAssyParametersWidget::initUI()
{
}

//-----------------------------------------------------------------------------
void cmbAssyParametersWidget::setAssembly(cmbNucAssembly *assyObj)
{
  if(this->Assembly == assyObj)
    {
    return;
    }
  this->Assembly = assyObj;
  this->onReset();
}
// Invoked when Apply button clicked
//-----------------------------------------------------------------------------
void cmbAssyParametersWidget::onApply()
{
  if(this->Assembly == NULL)
    {
    return;
    }
  this->applyToAssembly(this->Assembly);
}
// Invoked when Reset button clicked
//-----------------------------------------------------------------------------
void cmbAssyParametersWidget::onReset()
{
  if(this->Assembly == NULL)
    {
    return;
    }

  this->resetAssembly(this->Assembly);
}

//Helpers
namespace
{

bool convert(QString qw, std::string & result)
{
  std::string prev = result;
  result = qw.toStdString();
  return result != prev;
}

bool convert(QString qw, double & result)
{
  if(qw.isEmpty())
  {
    bool r = result != ASSY_NOT_SET_VALUE;
    result = ASSY_NOT_SET_VALUE;
    return r;
  }
  bool ok;
  double previous = result;
  result = qw.toDouble(&ok);
  if(!ok)
  {
    result = previous;
  }
  return result != previous;
}

bool convert(QString qw, int & result)
{
  bool ok;
  int previous = result;
  result = qw.toInt(&ok);
  if(!ok)
  {
    result = previous;
  }
  return result != previous;
}

bool setValue(std::string &to, QComboBox * from)
{
  QString tmp = from->currentText();
  return convert(tmp, to);
}

void setValue(QComboBox * to, std::string &from)
{
  QString tmp(from.c_str());
  tmp = tmp.simplified();
  to->setCurrentIndex(to->findText(tmp, Qt::MatchFixedString));
}

bool setValue(double &to, QLineEdit * from)
{
  if(from->text().isEmpty())
  {
    bool r = to != ASSY_NOT_SET_VALUE;
    to = ASSY_NOT_SET_VALUE;
    return r;
  }
  return convert(from->text(), to);
}

bool setValue(int &to, QLineEdit * from)
{
  if(from->text().isEmpty())
  {
    bool r = to != ASSY_NOT_SET_VALUE;
    to = ASSY_NOT_SET_VALUE;
    return r;
  }
  return convert(from->text(), to);
}

bool setValue(std::string &to, QLineEdit * from)
{
  if(from->text().isEmpty())
  {
    bool r = to != ASSY_NOT_SET_KEY;
    to = ASSY_NOT_SET_KEY;
    return r;
  }
  return convert(from->text(), to);
}

void setValue(QDoubleSpinBox * to, double from)
{
  to->setValue(from);
}

bool setValue(double &to, QDoubleSpinBox * from)
{
  double prev = to;
  to = from->value();
  return to != prev;
}

void setValue(QLineEdit * to, std::string &from)
{
  if(from == ASSY_NOT_SET_KEY)
  {
    to->setText("");
    return;
  }
  QString tmp(from.c_str());
  to->setText(tmp);
}

void setValue(QLineEdit * to, double &from)
{
  if(from == ASSY_NOT_SET_VALUE)
  {
    to->setText("");
    return;
  }
  QString tmp = QString::number(from);
  to->setText(tmp);
}

void setValue(QLineEdit * to, int &from)
{
  if(from == ASSY_NOT_SET_VALUE)
  {
    to->setText("");
    return;
  }
  QString tmp = QString::number(from);
  to->setText(tmp);
}

bool setValue(bool &to, QCheckBox * from)
{
  bool prev = to;
  to = from->isChecked();
  return prev != to;
}

void setValue(QCheckBox * to, bool &from)
{
  to->setChecked(from);
}
}

#define EASY_ASSY_PARAMS_MACRO()\
  FUN(TetMeshSize) \
  FUN(RadialMeshSize) \
  FUN(AxialMeshSize) \
  FUN(NeumannSet_StartId) \
  FUN(MaterialSet_StartId) \
  FUN(EdgeInterval)\
  FUN(MergeTolerance) \
  FUN(CreateFiles) \
  FUN(RotateXYZ) \
  FUN(RotateAngle) \
  FUN(CenterXYZ) \
  FUN(Info) \
  FUN(SectionXYZ) \
  FUN(SectionOffset) \
  FUN(StartPinId) \
  FUN(CellMaterial) \
  FUN(CreateMatFiles) \
  FUN(Save_Exodus) \
  FUN(List_NeumannSet_StartId) \
  FUN(List_MaterialSet_StartId) \
  FUN(NumSuperBlocks) \
  FUN(SuperBlocks) \
  FUN(MeshType)\
  FUN2(MoveXYZ[0],MoveX)\
  FUN2(MoveXYZ[1],MoveY)\
  FUN2(MoveXYZ[2],MoveZ)




//-----------------------------------------------------------------------------
void cmbAssyParametersWidget::applyToAssembly(cmbNucAssembly* assy)
{
  cmbAssyParameters* parameters = assy->GetParameters();
  bool changed = false;
#define FUN(X)    changed |= setValue(parameters->X, this->Internal->X);
#define FUN2(X,Y) changed |= setValue(parameters->X, this->Internal->Y);
  EASY_ASSY_PARAMS_MACRO()
#undef FUN
#undef FUN2
  std::stringstream ss(Internal->Unknown->toPlainText().toStdString().c_str());
  std::string line;
  parameters->UnknownParams.clear();
  while( std::getline(ss, line))
  {
    parameters->UnknownParams.push_back(line);
    line.clear();
  }
  if(changed) emit valuesChanged();
}

//-----------------------------------------------------------------------------
void cmbAssyParametersWidget::resetAssembly(cmbNucAssembly* assy)
{
  cmbAssyParameters* parameters = assy->GetParameters();
#define FUN(X) setValue(this->Internal->X, parameters->X);
#define FUN2(X,Y) setValue(this->Internal->Y, parameters->X);
  EASY_ASSY_PARAMS_MACRO()
#undef FUN
#undef FUN2
  std::string unknowns;
  for(unsigned int i = 0; i < parameters->UnknownParams.size(); ++i)
  {
    unknowns += parameters->UnknownParams[i] + "\n";
  }
  Internal->Unknown->setPlainText(QString::fromStdString(unknowns));
}
