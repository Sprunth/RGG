#include "cmbNucCoregen.h"
#include "cmbNucMainWindow.h"
#include <vtkActor.h>
#include <vtkAlgorithm.h>
#include <vtkDataObjectTreeIterator.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkInformation.h>
#include <vtkInteractorObserver.h>
#include <vtkNew.h>
#include <vtkProperty.h>
#include "moab_wrapper/vtkMoabReader.h"
#include "vtkCmbLayeredConeSource.h"
#include "vtkGeometryFilter.h"
#include "moab_wrapper/extract_subset.h"
#include <iostream>
#include <QDebug>

cmbNucCoregen::cmbNucCoregen(QComboBox * l)
{
  this->MoabReader = vtkSmartPointer<vtkMoabReader>::New();
  this->List = l;
  QObject::connect(this->List, SIGNAL(currentIndexChanged(int)),
                   this,       SLOT(onSelectionChanged(int)), Qt::UniqueConnection);
}

cmbNucCoregen::~cmbNucCoregen()
{
}

void cmbNucCoregen::clear()
{
  List->blockSignals(true);
  this->List->clear();
  this->MoabReader = vtkSmartPointer<vtkMoabReader>::New();
  this->Data = NULL;
  this->DataSets.clear();
  List->blockSignals(false);
  this->FileName = QString();
}

vtkSmartPointer<vtkDataObject>
cmbNucCoregen::getData()
{
  return this->Data;
}

void cmbNucCoregen::openFile(QString file)
{
  if(file.isEmpty()) return;
  this->FileName = file;
  this->MoabReader->SetFileName(file.toStdString().c_str());
  this->MoabReader->Modified();
  this->MoabReader->Update();
  vtkSmartPointer<vtkMultiBlockDataSet> tmp = this->MoabReader->GetOutput();  DataSets.resize(tmp->GetNumberOfBlocks());
  this->GeoFilt.resize(tmp->GetNumberOfBlocks(), NULL);
  List->blockSignals(true);
  List->clear();
  for (unsigned int i = 0; i < tmp->GetNumberOfBlocks(); ++i)
  {
    if(this->GeoFilt[i] == NULL) this->GeoFilt[i] = vtkSmartPointer<vtkGeometryFilter>::New();
    const char * name = tmp->GetMetaData(i)->Get(vtkCompositeDataSet::NAME());
    this->GeoFilt[i]->SetInputData(tmp->GetBlock(i));
    this->GeoFilt[i]->Update();
    DataSets[i].TakeReference(this->GeoFilt[i]->GetOutputDataObject(0)->NewInstance());
    DataSets[i]->DeepCopy(this->GeoFilt[i]->GetOutputDataObject(0));
    List->addItem(name);
    if(i == 0)
    {
      this->selectedType = i;
    }
  }
  List->addItem("No Color");
  List->blockSignals(false);
  List->setCurrentIndex(5);
  emit fileOpen(true);
}

void cmbNucCoregen::exportVisible(QString outFname, std::vector<std::string> const& remove )
{
  if(FileName.isEmpty()) return;
  extract_subset::extract(FileName.toStdString(), outFname.toStdString(), remove);
}

void cmbNucCoregen::onSelectionChanged( int sel )
{
  List->blockSignals(true);
  if(sel == 6)
  {
    this->Data = DataSets[0];
    this->color = false;
    this->selectedType = 0;
  }
  else
  {
    this->Data = DataSets[sel];
    this->color = true;
    this->selectedType = sel;
  }
  emit(update());
  List->blockSignals(false);
}
