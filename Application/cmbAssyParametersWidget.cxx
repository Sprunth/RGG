
#include "cmbAssyParametersWidget.h"
#include "ui_qAssyParameters.h"

#include "cmbNucAssembly.h"

#include <QLabel>
#include <QPointer>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QDoubleValidator>

class cmbAssyParametersWidget::cmbAssyParametersWidgetInternal :
  public Ui::qAssyParametersWidget
{
public:
  void setTransformRow(int r, cmbNucAssembly::Transform * xform);
  void resizeTable(int i)
  {
    this->TransformTable->setRowCount(i);
  }
};

void cmbAssyParametersWidget::cmbAssyParametersWidgetInternal
::setTransformRow(int r, cmbNucAssembly::Transform * xform)
{
  QTableWidget * table = this->TransformTable;
  table->setSelectionBehavior(QAbstractItemView::SelectRows);
  table->horizontalHeader()->setStretchLastSection(true);
  table->verticalHeader()->setVisible(false);
  //Type
  {
    QTableWidgetItem * item = new QTableWidgetItem(xform->getLabel().c_str());
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    table->setItem(r, 0, item);
  }
  //AXIS
  {
    QComboBox* comboBox = new QComboBox(table);
    comboBox->addItem("");
    comboBox->addItem("X");
    comboBox->addItem("Y");
    comboBox->addItem("Z");
    comboBox->setCurrentIndex(xform->getAxis()+1);
    comboBox->setEditable(xform->getControls() & cmbNucAssembly::Transform::HAS_AXIS);
    comboBox->setVisible(xform->getControls() & cmbNucAssembly::Transform::HAS_AXIS);
    table->setCellWidget(r, 1, comboBox);
  }
  //Value
  {
    QDoubleSpinBox* value = new QDoubleSpinBox(table);
    value->setDecimals(2);
    value->setFrame(false);
    value->setMaximum(10000.0);
    value->setMinimum(-10000.0);
    value->setValue(xform->getValue());
    value->setAlignment(Qt::AlignRight);
    value->setReadOnly(!(xform->getControls() & cmbNucAssembly::Transform::HAS_VALUE));
    table->setCellWidget(r, 2, value);
  }
  //Reverse
  {
    QTableWidgetItem *item = new QTableWidgetItem();
    item->setCheckState(xform->reverse()?(Qt::Checked):(Qt::Unchecked));
    if(!(xform->getControls() & cmbNucAssembly::Transform::HAS_REVERSE))
      item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    table->setItem(r,3,item);
  }
  table->resizeColumnsToContents();
}

//-----------------------------------------------------------------------------
cmbAssyParametersWidget::cmbAssyParametersWidget(QWidget *p)
  : QWidget(p)
{
  this->Internal = new cmbAssyParametersWidgetInternal;
  this->Internal->setupUi(this);
  this->initUI();
  this->Assembly = NULL;
  this->Internal->AxialMeshLayout->setVisible(false);
  this->Internal->EdgeIntervalLayout->setVisible(false);
  this->Internal->MeshTypeLayout_2->setVisible(false);
  connect(this->Internal->addRotation, SIGNAL(clicked ()),
          this, SLOT(addRotation()));
  connect(this->Internal->addSection, SIGNAL(clicked ()),
          this, SLOT(addSection()));
  connect(this->Internal->deleteSelected, SIGNAL(clicked ()),
          this, SLOT(deleteSelected()));
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
  FUN(CenterXYZ) \
  FUN(Info) \
  FUN(StartPinId) \
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

  //update transforms
  QTableWidget * table = this->Internal->TransformTable;
  changed |= assy->removeOldTransforms(table->rowCount());
  for(int i = 0; i < table->rowCount(); ++i)
  {
    std::string xform = table->item( i, 0 )->text().toStdString();
    std::string axis = (qobject_cast<QComboBox *>(table->cellWidget(i, 1)))->currentText().toStdString();
    double value = (qobject_cast<QDoubleSpinBox *>(table->cellWidget(i, 2)))->value();
    bool reverse = table->item( i, 3 )->checkState() == (Qt::Checked);
    cmbNucAssembly::Transform * tmp = NULL;
    if(xform == "Rotate") tmp = new cmbNucAssembly::Rotate(axis, value );
    else if(xform == "Section") tmp = new cmbNucAssembly::Section(axis, value, (reverse)?"reverse":"");
    changed |= assy->updateTransform(i, tmp);
  }

  std::stringstream ss(Internal->Unknown->toPlainText().toStdString().c_str());
  std::string line;
  unsigned int i = 0;
  while( std::getline(ss, line))
  {
    if(i <parameters->UnknownParams.size())
    {
      changed |= parameters->UnknownParams[i] != line;
      parameters->UnknownParams[i] = line;
    }
    else
    {
      changed = true;
      parameters->UnknownParams.push_back(line);
    }
    ++i;
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
  Internal->resizeTable(assy->getNumberOfTransforms());
  for(int i = 0; i < assy->getNumberOfTransforms(); ++i)
  {
    Internal->setTransformRow(i, assy->getTransform(i));
  }
}

void cmbAssyParametersWidget::addRotation()
{
  cmbNucAssembly::Rotate r("Z",0);
  int at = this->Internal->TransformTable->rowCount();
  this->Internal->resizeTable(at+1);
  this->Internal->setTransformRow(at, &r);
}

void cmbAssyParametersWidget::addSection()
{
  cmbNucAssembly::Section s("X",0,"");
  int at = this->Internal->TransformTable->rowCount();
  this->Internal->resizeTable(at+1);
  this->Internal->setTransformRow(at, &s);
}

void cmbAssyParametersWidget::deleteSelected()
{
  this->Internal->TransformTable->removeRow(this->Internal->TransformTable->currentRow());
}
