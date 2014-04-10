#include "cmbNucCoregen.h"
#include "cmbNucMainWindow.h"
#include <vtkActor.h>
#include <vtkAlgorithm.h>
#include <vtkAxesActor.h>
#include <vtkCamera.h>
#include <vtkCompositeDataDisplayAttributes.h>
#include <vtkCompositePolyDataMapper2.h>
#include <vtkCompositeDataPipeline.h>
#include <vtkDataObjectTreeIterator.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkInformation.h>
#include <vtkInteractorObserver.h>
#include <vtkNew.h>
#include <vtkProperty.h>
#include "vtk_moab_reader/vtkMoabReader.h"
#include "vtkCmbLayeredConeSource.h"
#include "vtkGeometryFilter.h"
#include <iostream>
#include <QDebug>

class meshOptionItem : public QTreeWidgetItem
{
public:
  meshOptionItem(const char* name, unsigned int i)
  {
    this->setText(0,name);
    Id = i;
  }
  unsigned int Id;
};

cmbNucCoregen::cmbNucCoregen(QTreeWidget * l)
{
  this->MoabReader = vtkMoabReader::New();
  this->List = l;
  QObject::connect(this->List, SIGNAL(itemSelectionChanged()),
                   this, SLOT(onSelectionChanged()), Qt::UniqueConnection);
}

cmbNucCoregen::~cmbNucCoregen()
{
}

vtkSmartPointer<vtkDataObject>
cmbNucCoregen::getData()
{
  return this->Data;
}

void cmbNucCoregen::openFile(QString file)
{
  this->MoabReader->SetFileName(file.toStdString().c_str());
  this->MoabReader->Modified();
  this->MoabReader->Update();
  vtkSmartPointer<vtkMultiBlockDataSet> tmp = this->MoabReader->GetOutput();
  DataSets.resize(tmp->GetNumberOfBlocks());
  this->GeoFilt.resize(tmp->GetNumberOfBlocks(), NULL);
  List->clear();
  for (unsigned int i = 0; i < tmp->GetNumberOfBlocks(); ++i)
  {
    if(this->GeoFilt[i] == NULL) this->GeoFilt[i] = vtkGeometryFilter::New();
    const char * name = tmp->GetMetaData(i)->Get(vtkCompositeDataSet::NAME());
    this->GeoFilt[i]->SetInputData(tmp->GetBlock(i));
    this->GeoFilt[i]->Update();
    DataSets[i].TakeReference(this->GeoFilt[i]->GetOutputDataObject(0)->NewInstance());
    DataSets[i]->DeepCopy(this->GeoFilt[i]->GetOutputDataObject(0));
    QTreeWidgetItem * atwi = new meshOptionItem(name, i);
    List->addTopLevelItem(atwi);
    if(i == 0)
    {
      atwi->setSelected(true);
      this->selectedType = i;
    }
  }
  this->Data = DataSets[0];
  color = false;
}

void cmbNucCoregen::onSelectionChanged()
{
  qDebug() << "Selection change";
  List->blockSignals(true);
  QList<QTreeWidgetItem*> selItems = List->selectedItems();
  meshOptionItem* selItem =
      selItems.count()>0 ? dynamic_cast<meshOptionItem*>(selItems.value(0)) : NULL;
  if(selItem)
  {
    this->Data = DataSets[selItem->Id];
    this->color = selItem->Id == 5;
    this->selectedType = selItem->Id;
    emit(update());
  }
  List->blockSignals(false);
}
