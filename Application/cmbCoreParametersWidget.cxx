
#include "cmbCoreParametersWidget.h"
#include "ui_qCoreParameters.h"

#include "cmbNucCore.h"

#include <QLabel>
#include <QPointer>
#include <QtDebug>
#include <QDebug>
#include <QIntValidator>

#include <sstream>

class cmbCoreParametersWidget::cmbCoreParametersWidgetInternal :
  public Ui::qCoreParametersWidget
{
public:

};

//-----------------------------------------------------------------------------
cmbCoreParametersWidget::cmbCoreParametersWidget(QWidget *p)
  : QFrame(p)
{
  this->Internal = new cmbCoreParametersWidgetInternal;
  this->Internal->setupUi(this);
  this->initUI();
  this->Core = NULL;
}

//-----------------------------------------------------------------------------
cmbCoreParametersWidget::~cmbCoreParametersWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void cmbCoreParametersWidget::initUI()
{
  Internal->NeumannSetId->setValidator( new QIntValidator(0, 100000, this) );
  connect(Internal->NewmannSetAdd, SIGNAL(clicked()), this, SLOT(onAddToTable()));
  connect(Internal->NewmannSetDel, SIGNAL(clicked()), this, SLOT(onDeleteRow()));
}

//-----------------------------------------------------------------------------
void cmbCoreParametersWidget::setCore(cmbNucCore *CoreObj)
{
  if(this->Core == CoreObj)
    {
    return;
    }
  this->Core = CoreObj;
  this->onReset();
}
// Invoked when Apply button clicked
//-----------------------------------------------------------------------------
void cmbCoreParametersWidget::onApply()
{
  if(this->Core == NULL)
    {
    return;
    }
  this->applyToCore(this->Core);
}
// Invoked when Reset button clicked
//-----------------------------------------------------------------------------
void cmbCoreParametersWidget::onReset()
{
  if(this->Core == NULL)
    {
    return;
    }

  this->resetCore(this->Core);
}

//-------
void cmbCoreParametersWidget::onAddToTable()
{
  addToTable(Internal->NeumannSetSide->currentText(),
             Internal->NeumannSetId->text(),
             Internal->NeumannSetEquation->text());
}

void cmbCoreParametersWidget::onDeleteRow()
{
  int r = Internal->NeumannSetTable->currentRow();
  if(r >= 0)
    {
    Internal->NeumannSetTable->removeRow(r);
    }
}

//Helpers

void convert(QString qw, std::string & result)
{
  result = qw.toStdString();
}

void convert(QString qw, double & result)
{
  bool ok;
  double previous = result;
  result = qw.toDouble(&ok);
  if(!ok)
  {
    result = previous;
  }
}

void convert(QString qw, int & result)
{
  bool ok;
  int previous = result;
  result = qw.toInt(&ok);
  if(!ok)
  {
    result = previous;
  }
}

void convert(bool qw, bool & result)
{
  result = qw;
}

void setValue(std::string &to, QComboBox * from)
{
  qDebug() << "sending value";
  convert(from->currentText (), to);
}

void setValue(QComboBox * to, std::string &from)
{
  QString tmp(from.c_str());
  qDebug() << "Settign combo box: " << tmp;
  to->setCurrentIndex(to->findText(tmp, Qt::MatchFixedString));
}

void setValue(double &to, QLineEdit * from)
{
  if(from->text().isEmpty()) return;
  convert(from->text(), to);
}

void setValue(int &to, QLineEdit * from)
{
  if(from->text().isEmpty()) return;
  convert(from->text(), to);
}


void setValue(QLineEdit * to, double &from)
{
  QString tmp = QString::number(from);
  if(tmp.isEmpty()) return;
  to->setText(tmp);
}

void setValue(QLineEdit * to, int &from)
{
  QString tmp = QString::number(from);
  if(tmp.isEmpty()) return;
  to->setText(tmp);
}

void setValue(bool &to, QCheckBox * from)
{
  to = from->isChecked();
}

void setValue(QCheckBox * to, bool &from)
{
  to->setChecked(from);
}

#define USED_SIMPLE_VARABLE_MACRO() \
FUN_SIMPLE(std::string, QString, ProblemType, problemtype, "", "") \
FUN_SIMPLE(std::string, QString, Geometry, geometry, "", "") \
FUN_SIMPLE(double, QString, MergeTolerance, mergetolerance, -1e23, "") \
FUN_SIMPLE(std::string, QString, SaveParallel, saveparallel, "", "") \
FUN_SIMPLE(bool, bool, Info, info, false, "on") \
FUN_SIMPLE(bool, bool, MeshInfo, meshinfo, false, "on")

//-----------------------------------------------------------------------------
void cmbCoreParametersWidget::applyToCore(cmbNucCore* Core)
{
#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, DK) \
setValue(Core->Params.Var, Internal->Var);

  USED_SIMPLE_VARABLE_MACRO()

#undef FUN_SIMPLE

  cmbNucCoreParams::NeumannSetStruct tmp;
  Core->Params.NeumannSet.clear();
  for (unsigned int i = 0; i < Internal->NeumannSetTable->rowCount(); ++i )
  {
    convert(Internal->NeumannSetTable->item( i, 0 )->text(), tmp.Side);
    convert(Internal->NeumannSetTable->item( i, 1 )->text(), tmp.Id);
    convert(Internal->NeumannSetTable->item( i, 2 )->text(), tmp.Equation);
    Core->Params.NeumannSet.push_back(tmp);
  }
  convert(Internal->ExtrudeDivisions->text(), Core->Params.Extrude.Divisions);
  convert(Internal->ExtrudeHeight->text(), Core->Params.Extrude.Size);

  std::stringstream ss(Internal->UnknownsVars->toPlainText().toStdString().c_str());
  std::string line;
  while( std::getline(ss, line))
  {
    Core->Params.UnknownKeyWords.push_back(line);
    line.clear();
  }
}

//-----------------------------------------------------------------------------
void cmbCoreParametersWidget::resetCore(cmbNucCore* Core)
{
  qDebug() << "Resetting core";
#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, DK) \
if(Core->Params.Var##IsSet()){ setValue(Internal->Var, Core->Params.Var); }

  USED_SIMPLE_VARABLE_MACRO()

#undef FUN_SIMPLE

  std::vector<cmbNucCoreParams::NeumannSetStruct> & ns = Core->Params.NeumannSet;
  while ( Internal->NeumannSetTable->rowCount() > 0)
  {
     Internal->NeumannSetTable->removeRow(0);
  }
  //cmbNucCoreParams::NeumannSetStruct, NeumannSet
  for(unsigned int i = 0; i < ns.size(); ++i)
  {
    addToTable(QString::fromStdString(ns[i].Side),
               QString::number(ns[i].Id),
               QString::fromStdString(ns[i].Equation));
  }

  if(Core->Params.ExtrudeIsSet())
    {
    setValue(Internal->ExtrudeDivisions, Core->Params.Extrude.Divisions);
    setValue(Internal->ExtrudeHeight, Core->Params.Extrude.Size);
    }
  std::string unknowns;
  for(unsigned int i = 0; i < Core->Params.UnknownKeyWords.size(); ++i)
    {
    unknowns += Core->Params.UnknownKeyWords[i] + "\n";
    }
  Internal->UnknownsVars->setPlainText(QString::fromStdString(unknowns));
}

void cmbCoreParametersWidget::addToTable(QString side, QString sid, QString equ)
{
  if(side.isEmpty() || sid.isEmpty()) return;
  unsigned int row = Internal->NeumannSetTable->rowCount();
  Internal->NeumannSetTable->insertRow( row );
  QTableWidgetItem * item = new QTableWidgetItem(side);
  Internal->NeumannSetTable->setItem ( row, 0, item );
  item = new QTableWidgetItem(sid);
  Internal->NeumannSetTable->setItem ( row, 1, item );
  item = new QTableWidgetItem(equ);
  Internal->NeumannSetTable->setItem ( row, 2, item );
}
