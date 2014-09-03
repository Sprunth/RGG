
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
  std::string background_full_path;
};

//-----------------------------------------------------------------------------
cmbCoreParametersWidget::cmbCoreParametersWidget(QWidget *p)
  : QFrame(p)
{
  this->Internal = new cmbCoreParametersWidgetInternal;
  this->currentRadius = 0;
  this->currentInterval = 0;
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
  connect( this->Internal->BackgroundSetting, SIGNAL(clicked()),
           this, SLOT(onSetBackgroundMesh()) );
  connect( this->Internal->BackgroundClear, SIGNAL(clicked()),
           this, SLOT(onClearBackgroundMesh()));
  connect(this->Internal->OuterEdgeInterval, SIGNAL(valueChanged(int)),
          this, SLOT(onIntervalChanged(int)));
  connect(this->Internal->RadiusBox, 	SIGNAL(valueChanged(double)),
          this, SLOT(onRadiusChanged(double)));
  connect(this->Internal->CalculateDefaults, SIGNAL(clicked()),
          this, SLOT(onCalculateCylinderDefaults()));
  connect(this->Internal->JacketMode, SIGNAL(currentIndexChanged(int)),
          this, SLOT(onDrawCylinder()));

  connect(this->Internal->JacketMode, SIGNAL(currentIndexChanged(int)),
          this, SLOT(controlDisplayBackgroundControls(int)));
}

//-----------------------------------------------------------------------------
void cmbCoreParametersWidget::setCore(cmbNucCore *CoreObj)
{
  if(this->Core == CoreObj)
    {
    return;
    }
  this->Core = CoreObj;
  this->previousRadius = CoreObj->getCylinderRadius();
  this->previousInterval = CoreObj->getCylinderOuterSpacing();
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

void cmbCoreParametersWidget::onSetBackgroundMesh()
{
  if(this->Core == NULL)
  {
    return;
  }
  QString fileName;
  if(this->Internal->JacketMode->currentIndex() == cmbNucCoreParams::Generate)
  {
    QString defaultLoc;
    QString name(Core->FileName.c_str());
    if(!name.isEmpty())
    {
      QFileInfo fi(name);
      QDir dir = fi.dir();
      if(dir.path() == ".")
      {
        QDir tdir = QSettings("CMBNuclear", "CMBNuclear").value("cache/lastDir",
                                                                QDir::homePath()).toString();
        defaultLoc = tdir.path();
      }
      else
      {
        defaultLoc = dir.path();
      }
    }
    else
    {
      QDir tdir = QSettings("CMBNuclear", "CMBNuclear").value("cache/lastDir",
                                                              QDir::homePath()).toString();
      defaultLoc = tdir.path();
    }
    QFileDialog saveQD( this, "Save Outer Cylinder File...", defaultLoc, "cub Files (*.cub)");
    saveQD.setOptions(QFileDialog::DontUseNativeDialog); //There is a bug on the mac were one does not seem to be able to set the default name.
    saveQD.setAcceptMode(QFileDialog::AcceptSave);
    saveQD.selectFile("outer_cylinder.cub");

    if(saveQD.exec()== QDialog::Accepted)
    {
      fileName = saveQD.selectedFiles().first();
    }
  }
  else
  {
    // Use cached value for last used directory if there is one,
    // or default to the user's home dir if not.
    QSettings settings("CMBNuclear", "CMBNuclear");
    QDir dir = settings.value("cache/lastDir", QDir::homePath()).toString();

    QStringList fileNames =
    QFileDialog::getOpenFileNames(this,
                                  "Open File...",
                                  dir.path(),
                                  "cub Files (*.cub)");
    if(fileNames.count()==0)
    {
      return;
    }
    fileName =fileNames[0];
  }
  // Cache the directory for the next time the dialog is opened
  QFileInfo info(fileName);
  this->Internal->background_full_path = info.absoluteFilePath().toStdString();
  Internal->Background->setText(info.fileName());
}

void cmbCoreParametersWidget::onClearBackgroundMesh()
{
  if(this->Core == NULL)
  {
    return;
  }
  this->Internal->background_full_path = "";
  Internal->Background->setText("");
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
FUN_SIMPLE(std::string, QString, Background, Background, "", "") \
FUN_SIMPLE(bool, bool, Info, info, false, "on") \
FUN_SIMPLE(bool, bool, MeshInfo, meshinfo, false, "on")

//-----------------------------------------------------------------------------
void cmbCoreParametersWidget::applyToCore(cmbNucCore* Core)
{
  bool changed = false;
#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, DK) \
changed |= setValue(Core->Params.Var, Internal->Var);

  USED_SIMPLE_VARABLE_MACRO()

#undef FUN_SIMPLE

  if(Core->Params.BackgroundMode != this->Internal->JacketMode->currentIndex())
  {
    switch(this->Internal->JacketMode->currentIndex())
    {
      case cmbNucCoreParams::None:
        Core->Params.BackgroundMode =cmbNucCoreParams::None;
        break;
      case cmbNucCoreParams::External:
        Core->Params.BackgroundMode =cmbNucCoreParams::External;
        break;
      case cmbNucCoreParams::Generate:
        Core->Params.BackgroundMode =cmbNucCoreParams::Generate;
        break;
      default:
        break;
    }
    changed = true;
  }

  if(Core->Params.BackgroundFullPath != Internal->background_full_path &&
     Core->Params.BackgroundMode != cmbNucCoreParams::None)
  {
    Core->Params.BackgroundFullPath = Internal->background_full_path;
    changed = true;
  }

  std::string meshFile = Internal->OutputFile->text().toStdString();
  if(meshFile != Core->h5mFile)
  {
    Core->h5mFile = meshFile;
  }

  cmbNucCoreParams::NeumannSetStruct tmp;
  changed |= Core->Params.NeumannSet != Internal->NeumannSetTable->rowCount();
  Core->Params.NeumannSet.resize(Internal->NeumannSetTable->rowCount());
  for (unsigned int i = 0; i < Internal->NeumannSetTable->rowCount(); ++i )
  {
    changed |= convert(Internal->NeumannSetTable->item( i, 0 )->text(),
                       Core->Params.NeumannSet[i].Side);
    changed |= convert(Internal->NeumannSetTable->item( i, 1 )->text(),
                       Core->Params.NeumannSet[i].Id);
    changed |= convert(Internal->NeumannSetTable->item( i, 2 )->text(),
                       Core->Params.NeumannSet[i].Equation);
  }
  changed |= convert(Internal->ExtrudeDivisions->text(), Core->Params.Extrude.Divisions);
  changed |= convert(Internal->ExtrudeHeight->text(), Core->Params.Extrude.Size);

  std::stringstream ss(Internal->UnknownsVars->toPlainText().toStdString().c_str());
  std::string line;
  unsigned int j = 0;
  while( std::getline(ss, line))
  {
    if(j<Core->Params.UnknownKeyWords.size())
    {
      changed |= Core->Params.UnknownKeyWords[j] != line;
      Core->Params.UnknownKeyWords[j] = line;
    }
    else
    {
      changed = true;
      Core->Params.UnknownKeyWords.push_back(line);
    }
    j++;
    line.clear();
  }
  if(this->previousRadius != this->currentRadius && Core->Params.BackgroundIsSet() )
  {
    Core->setCylinderRadius(this->currentRadius);
    this->previousRadius = this->currentRadius;
    changed = true;
  }

  if(this->previousInterval != this->currentInterval && Core->Params.BackgroundIsSet())
  {
    Core->setCylinderOuterSpacing(this->currentInterval);
    this->previousInterval = this->currentInterval;
    changed = true;
  }
  if(changed) emit valuesChanged();
}

//-----------------------------------------------------------------------------
void cmbCoreParametersWidget::resetCore(cmbNucCore* Core)
{
#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, DK) \
if(Core->Params.Var##IsSet()){ setValue(Internal->Var, Core->Params.Var); }\
else{ setValue(Internal->Var, DEFAULT); }

  USED_SIMPLE_VARABLE_MACRO()

#undef FUN_SIMPLE

  Internal->OutputFile->setText(Core->h5mFile.c_str());

  this->Internal->background_full_path = Core->Params.BackgroundFullPath;

  this->Internal->OuterEdgeInterval->setValue(this->previousInterval);
  this->Internal->RadiusBox->setValue(this->previousRadius);

  this->Internal->JacketMode->setCurrentIndex(Core->Params.BackgroundMode);

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

void cmbCoreParametersWidget::onRadiusChanged(double v)
{
  this->currentRadius = v;
  this->onDrawCylinder();
}

void cmbCoreParametersWidget::onIntervalChanged(int v)
{
  this->currentInterval = v;
  this->onDrawCylinder();
}

void cmbCoreParametersWidget::onDrawCylinder()
{
  if(this->Internal->JacketMode->currentIndex() == cmbNucCoreParams::Generate)
  {
    emit drawCylinder(this->currentRadius, this->currentInterval);
  }
  else
  {
    emit clearCylinder();
  }
}

void cmbCoreParametersWidget::onCalculateCylinderDefaults()
{
  double initRadius;
  int ei = 0;
  Core->GetDefaults()->getEdgeInterval(ei);
  if(!Core->GetDefaults()->getEdgeInterval(ei)) ei = 10;
  double ductsize[2];
  Core->GetDefaults()->getDuctThickness(ductsize[0],ductsize[1]);
  if(Core->IsHexType())
  {
    initRadius = Core->getLattice().Grid.size() * ductsize[0];
    this->Internal->OuterEdgeInterval->setValue(Core->getLattice().Grid.size()*ei*12);
  }
  else
  {
    double ti = Core->getLattice().Grid[0].size() * ductsize[0];
    double tj = Core->getLattice().Grid.size() * ductsize[1];
    double tr = std::sqrt(ti*ti+tj*tj);
    initRadius = tr*0.5 + std::sqrt( ductsize[0]*ductsize[0]+ductsize[1]*ductsize[0])*0.5;
    this->Internal->OuterEdgeInterval->setValue(std::max(Core->getLattice().Grid.size(),Core->getLattice().Grid[0].size())
                                                *ei*4);
  }

  this->Internal->RadiusBox->setValue(initRadius);
}

void cmbCoreParametersWidget::controlDisplayBackgroundControls(int index)
{
  this->Internal->FileName->setVisible(index != cmbNucCoreParams::None);
  this->Internal->GenerateControls->setVisible(index == cmbNucCoreParams::Generate);
  if(index == cmbNucCoreParams::Generate)
  {
    double ductsize[2];
    Core->GetDefaults()->getDuctThickness(ductsize[0],ductsize[1]);
    this->Internal->GenerateControls->setVisible(true);
    if(this->currentRadius != 0 && this->currentInterval != 0)
    {
      this->onDrawCylinder();
    }
    if(this->currentRadius == 0)
    {
      double ir =0;
      if(Core->IsHexType())
      {
        ir = Core->getLattice().Grid.size() * ductsize[0];
      }
      else
      {
        double ti = Core->getLattice().Grid[0].size() * ductsize[0];
        double tj = Core->getLattice().Grid.size() * ductsize[1];
        double tr = std::sqrt(ti*ti+tj*tj);
        ir = tr*0.5 + std::sqrt( ductsize[0]*ductsize[0]+ductsize[1]*ductsize[0])*0.5;
      }
      this->Internal->RadiusBox->setValue(ir);
    }
    if(this->currentInterval == 0)
    {
      int ei = 0;
      ;
      if(!Core->GetDefaults()->getEdgeInterval(ei)) ei = 10;
      if(Core->IsHexType())
      {
        this->Internal->OuterEdgeInterval->setValue(Core->getLattice().Grid.size()*ei*12);
      }
      else
      {
        this->Internal->OuterEdgeInterval->setValue(std::max(Core->getLattice().Grid.size(),Core->getLattice().Grid[0].size())
                                                    *ei*4);
      }
    }
  }
  else
  {
    emit clearCylinder();
  }
}
