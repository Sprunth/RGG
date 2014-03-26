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
  this->GeoFilt = vtkGeometryFilter::New();
  this->List = l;
  QObject::connect(this->List, SIGNAL(itemSelectionChanged()),
                   this, SLOT(onSelectionChanged()), Qt::QueuedConnection);
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
  this->MoabReader->Update();
  vtkSmartPointer<vtkMultiBlockDataSet> tmp = this->MoabReader->GetOutput();
  DataSets.resize(tmp->GetNumberOfBlocks());
  for (unsigned int i = 0; i < tmp->GetNumberOfBlocks(); ++i)
  {
    const char * name = tmp->GetMetaData(i)->Get(vtkCompositeDataSet::NAME());
    qDebug() << name;
    this->GeoFilt->SetInputData(tmp->GetBlock(i));
    this->GeoFilt->Update();
    DataSets[i].TakeReference(this->GeoFilt->GetOutputDataObject(0)->NewInstance());
    DataSets[i]->DeepCopy(this->GeoFilt->GetOutputDataObject(0));
    QTreeWidgetItem * atwi = new meshOptionItem(name, i);
    List->addTopLevelItem(atwi);
    if(i == 0)
    {
      atwi->setSelected(true);
    }
  }
  this->Data = DataSets[0];
}

void cmbNucCoregen::onSelectionChanged()
{
  QList<QTreeWidgetItem*> selItems = List->selectedItems();
  meshOptionItem* selItem =
      selItems.count()>0 ? dynamic_cast<meshOptionItem*>(selItems.value(0)) : NULL;
  if(selItem)
  {
    this->Data = DataSets[selItem->Id];
    emit(update());
  }
}
