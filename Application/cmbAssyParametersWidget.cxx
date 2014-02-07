
#include "cmbAssyParametersWidget.h"
#include "ui_qAssyParameters.h"

#include "cmbNucAssembly.h"

#include <QLabel>
#include <QPointer>
#include <QtDebug>

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
/*
  QObject::connect(this->Internal->TetMeshSize, SIGNAL(editingFinished()),
    this, SLOT(onTetMeshSizeChanged()));
  QObject::connect(this->Internal->RadialMeshSize, SIGNAL(editingFinished()),
    this, SLOT(onRadialMeshSizeChanged()));
  QObject::connect(this->Internal->AxialMeshSize, SIGNAL(editingFinished()),
    this, SLOT(onAxialMeshSizeChanged()));
  QObject::connect(this->Internal->NeumannSet_StartId, SIGNAL(editingFinished()),
    this, SLOT(onNeumannSetStartIdChanged()));
  QObject::connect(this->Internal->MaterialSet_StartId, SIGNAL(editingFinished()),
    this, SLOT(onMaterialSetStartIdChanged()));
  QObject::connect(this->Internal->EdgeInterval, SIGNAL(editingFinished()),
    this, SLOT(onEdgeIntervalChanged()));
  QObject::connect(this->Internal->MergeTolerance, SIGNAL(editingFinished()),
    this, SLOT(onMergeToleranceChanged()));
  QObject::connect(this->Internal->CreateFiles, SIGNAL(editingFinished()),
    this, SLOT(onCreateFilesChanged()));

  QObject::connect(this->Internal->RotateXYZ, SIGNAL(currentIndexChanged(int)),
    this, SLOT(onRotateXYZChanged(int)));
  QObject::connect(this->Internal->RotateAngle, SIGNAL(editingFinished()),
    this, SLOT(onRotateAngleChanged()));

  QObject::connect(this->Internal->CenterXYZ, SIGNAL(currentIndexChanged(int)),
    this, SLOT(onCenterXYZChanged(int)));
  QObject::connect(this->Internal->MoveXYZ, SIGNAL(currentIndexChanged(int)),
    this, SLOT(onMoveXYZChanged(int)));
  QObject::connect(this->Internal->CreateSideSetYesNo, SIGNAL(currentIndexChanged(int)),
    this, SLOT(onCreateSideSetChanged(int)));
  QObject::connect(this->Internal->InfoOnOff, SIGNAL(currentIndexChanged(int)),
    this, SLOT(onInfoOnOffChanged(int)));
  QObject::connect(this->Internal->SectionXYZ, SIGNAL(currentIndexChanged(int)),
    this, SLOT(onSectionXYZChanged(int)));
  QObject::connect(this->Internal->SectionOffset, SIGNAL(editingFinished()),
    this, SLOT(onSectionOffsetChanged()));
  QObject::connect(this->Internal->SectionReverse, SIGNAL(toggled(bool)),
    this, SLOT(onSectionReverseChanged(bool)));
*/
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

//-----------------------------------------------------------------------------
void cmbAssyParametersWidget::applyToAssembly(cmbNucAssembly* assy)
{
  bool ok = false;
  cmbAssyParameters* parameters = assy->GetParameters();
  //parameters->TetMeshSize = QString::number
  this->Internal->TetMeshSize->setText(
    parameters->isValueSet(parameters->TetMeshSize) ?
    QString::number(parameters->TetMeshSize) : "");
  this->Internal->RadialMeshSize->setText(
    parameters->isValueSet(parameters->RadialMeshSize) ?
    QString::number(parameters->RadialMeshSize) : "");
  this->Internal->AxialMeshSize->setText(
    parameters->isValueSet(parameters->AxialMeshSize) ?
    QString::number(parameters->AxialMeshSize) : "");
  this->Internal->NeumannSet_StartId->setText(
    parameters->isValueSet(parameters->NeumannSet_StartId) ?
    QString::number(parameters->NeumannSet_StartId) : "");
  this->Internal->MaterialSet_StartId->setText(
    parameters->isValueSet(parameters->MaterialSet_StartId) ?
    QString::number(parameters->MaterialSet_StartId) : "");
  this->Internal->EdgeInterval->setText(
    parameters->isValueSet(parameters->EdgeInterval) ?
    QString::number(parameters->EdgeInterval) : "");
  this->Internal->MergeTolerance->setText(
    parameters->isValueSet(parameters->MergeTolerance) ?
    QString::number(parameters->MergeTolerance) : "");
  this->Internal->CreateFiles->setText(
    parameters->isValueSet(parameters->CreateFiles) ?
    QString::number(parameters->CreateFiles) : "");

  this->Internal->RotateXYZ->setCurrentIndex(
    this->Internal->RotateXYZ->findText(parameters->RotateXYZ.c_str()));
  this->Internal->RotateAngle->setText(
    parameters->isValueSet(parameters->RotateAngle) ?
    QString::number(parameters->RotateAngle) : "");
  this->Internal->CenterXYZ->setCurrentIndex(
    this->Internal->RotateXYZ->findText(parameters->CenterXYZ.c_str()));
  this->Internal->MoveXYZ->setCurrentIndex(
    this->Internal->RotateXYZ->findText(parameters->MoveXYZ.c_str()));
  this->Internal->CreateSideSetYesNo->setCurrentIndex(
    this->Internal->RotateXYZ->findText(parameters->CreateSideset.c_str()));
  this->Internal->InfoOnOff->setCurrentIndex(
    this->Internal->RotateXYZ->findText(parameters->Info.c_str()));
  this->Internal->SectionXYZ->setCurrentIndex(
    this->Internal->RotateXYZ->findText(parameters->SectionXYZ.c_str()));
  this->Internal->SectionOffset->setText(
    parameters->isValueSet(parameters->SectionOffset) ?
    QString::number(parameters->SectionOffset) : "");
  this->Internal->SectionReverse->setChecked(parameters->SectionReverse);
}

//-----------------------------------------------------------------------------
void cmbAssyParametersWidget::resetAssembly(cmbNucAssembly* assy)
{
  cmbAssyParameters* parameters = assy->GetParameters();
  this->Internal->TetMeshSize->setText(
    parameters->isValueSet(parameters->TetMeshSize) ?
    QString::number(parameters->TetMeshSize) : "");
  this->Internal->RadialMeshSize->setText(
    parameters->isValueSet(parameters->RadialMeshSize) ?
    QString::number(parameters->RadialMeshSize) : "");
  this->Internal->AxialMeshSize->setText(
    parameters->isValueSet(parameters->AxialMeshSize) ?
    QString::number(parameters->AxialMeshSize) : "");
  this->Internal->NeumannSet_StartId->setText(
    parameters->isValueSet(parameters->NeumannSet_StartId) ?
    QString::number(parameters->NeumannSet_StartId) : "");
  this->Internal->MaterialSet_StartId->setText(
    parameters->isValueSet(parameters->MaterialSet_StartId) ?
    QString::number(parameters->MaterialSet_StartId) : "");
  this->Internal->EdgeInterval->setText(
    parameters->isValueSet(parameters->EdgeInterval) ?
    QString::number(parameters->EdgeInterval) : "");
  this->Internal->MergeTolerance->setText(
    parameters->isValueSet(parameters->MergeTolerance) ?
    QString::number(parameters->MergeTolerance) : "");
  this->Internal->CreateFiles->setText(
    parameters->isValueSet(parameters->CreateFiles) ?
    QString::number(parameters->CreateFiles) : "");

  this->Internal->RotateXYZ->setCurrentIndex(
    this->Internal->RotateXYZ->findText(parameters->RotateXYZ.c_str()));
  this->Internal->RotateAngle->setText(
    parameters->isValueSet(parameters->RotateAngle) ?
    QString::number(parameters->RotateAngle) : "");
  this->Internal->CenterXYZ->setCurrentIndex(
    this->Internal->RotateXYZ->findText(parameters->CenterXYZ.c_str()));
  this->Internal->MoveXYZ->setCurrentIndex(
    this->Internal->RotateXYZ->findText(parameters->MoveXYZ.c_str()));
  this->Internal->CreateSideSetYesNo->setCurrentIndex(
    this->Internal->RotateXYZ->findText(parameters->CreateSideset.c_str()));
  this->Internal->InfoOnOff->setCurrentIndex(
    this->Internal->RotateXYZ->findText(parameters->Info.c_str()));
  this->Internal->SectionXYZ->setCurrentIndex(
    this->Internal->RotateXYZ->findText(parameters->SectionXYZ.c_str()));
  this->Internal->SectionOffset->setText(
    parameters->isValueSet(parameters->SectionOffset) ?
    QString::number(parameters->SectionOffset) : "");
  this->Internal->SectionReverse->setChecked(parameters->SectionReverse);

}
