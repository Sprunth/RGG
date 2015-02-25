
#include "cmbCoreParametersWidget.h"
#include "ui_qCoreParameters.h"

#include "cmbNucCore.h"
#include "cmbNucDefaults.h"

#include <QLabel>
#include <QPointer>
#include <QtDebug>
#include <QDebug>
#include <QIntValidator>
#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>

#include <sstream>
#include <cmath>

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

bool convert(QString qw, std::string & result)
{
  bool diff = result != qw.toStdString();
  result = qw.toStdString();
  return diff;
}

bool convert(QString qw, double & result)
{
  if(qw.isEmpty())
  {
    bool r = result != -1e23;
    result = -1e23;
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
  return convert(from->currentText (), to);
}

void setValue(QComboBox * to, std::string from)
{
  QString tmp(from.c_str());
  to->setCurrentIndex(to->findText(tmp, Qt::MatchFixedString));
}

bool setValue(double &to, QLineEdit * from)
{
  return convert(from->text(), to);
}

bool setValue(int &to, QLineEdit * from)
{
  if(from->text().isEmpty())
  {
    bool r = to != -100;
    to = -100;
    return r;
  }
  return convert(from->text(), to);
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

void setValue(QLineEdit * to, std::string tmp)
{
  to->setText(tmp.c_str());
}

bool setValue(bool &to, QCheckBox * from)
{
  bool r = to != from->isChecked();
  to = from->isChecked();
  return r;
}

void setValue(QCheckBox * to, bool from)
{
  to->setChecked(from);
}

bool setValue(std::string &to, QLabel * from)
{
  return convert(from->text(), to);
}


void setValue(QLabel * to, std::string from)
{
  QString tmp(from.c_str());
  to->setText(tmp);
}


#define USED_SIMPLE_VARABLE_MACRO() \
FUN_SIMPLE(std::string, QString, ProblemType, problemtype, "", "") \
FUN_SIMPLE(std::string, QString, Geometry, geometry, "", "") \
FUN_SIMPLE(double, QString, MergeTolerance, mergetolerance, "", "") \
FUN_SIMPLE(std::string, QString, SaveParallel, saveparallel, "", "") \
FUN_SIMPLE(bool, bool, Info, info, false, "on") \
FUN_SIMPLE(bool, bool, MeshInfo, meshinfo, false, "on")

//-----------------------------------------------------------------------------
void cmbCoreParametersWidget::applyToCore(cmbNucCore* corein)
{
  bool changed = false;
#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, DK) \
changed |= setValue(corein->Params.Var, Internal->Var);

  USED_SIMPLE_VARABLE_MACRO()

#undef FUN_SIMPLE

  std::string meshFile = Internal->OutputFile->text().toStdString();
  if(meshFile != corein->h5mFile)
  {
    corein->h5mFile = meshFile;
    changed = true;
  }

  cmbNucCoreParams::NeumannSetStruct tmp;
  changed |= corein->Params.NeumannSet != Internal->NeumannSetTable->rowCount();
  corein->Params.NeumannSet.resize(Internal->NeumannSetTable->rowCount());
  for (int i = 0; i < Internal->NeumannSetTable->rowCount(); ++i )
  {
    changed |= convert(Internal->NeumannSetTable->item( i, 0 )->text(),
                       corein->Params.NeumannSet[i].Side);
    changed |= convert(Internal->NeumannSetTable->item( i, 1 )->text(),
                       corein->Params.NeumannSet[i].Id);
    changed |= convert(Internal->NeumannSetTable->item( i, 2 )->text(),
                       corein->Params.NeumannSet[i].Equation);
  }
  changed |= convert(Internal->ExtrudeDivisions->text(), corein->Params.Extrude.Divisions);
  changed |= convert(Internal->ExtrudeHeight->text(), corein->Params.Extrude.Size);

  std::stringstream ss(Internal->UnknownsVars->toPlainText().toStdString().c_str());
  std::string line;
  unsigned int j = 0;
  while( std::getline(ss, line))
  {
    if(j<corein->Params.UnknownKeyWords.size())
    {
      changed |= corein->Params.UnknownKeyWords[j] != line;
      corein->Params.UnknownKeyWords[j] = line;
    }
    else
    {
      changed = true;
      corein->Params.UnknownKeyWords.push_back(line);
    }
    j++;
    line.clear();
  }

  if(changed) emit valuesChanged();
}

//-----------------------------------------------------------------------------
void cmbCoreParametersWidget::resetCore(cmbNucCore* corein)
{
#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, DK) \
if(Core->Params.Var##IsSet()){ setValue(Internal->Var, corein->Params.Var); }\
else{ setValue(Internal->Var, DEFAULT); }

  USED_SIMPLE_VARABLE_MACRO()

#undef FUN_SIMPLE

  Internal->OutputFile->setText(corein->h5mFile.c_str());



  std::vector<cmbNucCoreParams::NeumannSetStruct> & ns = corein->Params.NeumannSet;
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
    setValue(Internal->ExtrudeDivisions, corein->Params.Extrude.Divisions);
    setValue(Internal->ExtrudeHeight, corein->Params.Extrude.Size);
    }
  std::string unknowns;
  for(unsigned int i = 0; i < corein->Params.UnknownKeyWords.size(); ++i)
    {
    unknowns += corein->Params.UnknownKeyWords[i] + "\n";
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
