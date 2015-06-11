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

#include "cmbNucMaterialColors.h"

extern int defaultAssemblyColors[][3];
extern int numAssemblyDefaultColors;

namespace
{
  void computeBounds(vtkMultiBlockDataSet * dataset, vtkBoundingBox * box)
  {
    if(dataset == NULL) return;
    // move the assembly to the correct position
    for(size_t idx=0; idx<dataset->GetNumberOfBlocks(); idx++)
    {
      // Brutal. I wish the SetDefaultExecutivePrototype had workd :(
      if(vtkDataObject* objBlock = dataset->GetBlock(static_cast<unsigned int>(idx)))
      {
        if(vtkMultiBlockDataSet* assyPartBlock =
           vtkMultiBlockDataSet::SafeDownCast(objBlock))
        {
          computeBounds(assyPartBlock, box);
        }
        else
        {
          vtkDataSet* part = vtkDataSet::SafeDownCast(objBlock);
          double tmpb[6];
          part->GetBounds(tmpb);
          box->AddBounds(tmpb);
        }
      }
    }
  }
}

class MeshTreeItem : public QTreeWidgetItem
{
public:
  MeshTreeItem(QTreeWidgetItem* pNode, size_t r, int s, vtkSmartPointer<vtkDataObject> data_in)
  :QTreeWidgetItem(pNode)
  {
    if(vtkMultiBlockDataSet::SafeDownCast(data_in) == NULL)
    {
      vtkSmartPointer<vtkMultiBlockDataSet> mb = vtkSmartPointer<vtkMultiBlockDataSet>::New();
      mb->SetNumberOfBlocks(1);
      mb->SetBlock(0, data_in);
      data = mb;
    }
    else
    {
      data = data_in;
    }
    rootId = r;
    subId = s;
  }
  virtual ~MeshTreeItem()
  {}

  size_t rootId;
  int subId;
  std::vector< QPointer<cmbNucMaterial> > materials;
  vtkSmartPointer<vtkDataObject> data;
protected:

};

cmbNucCoregen::cmbNucCoregen()
{
  this->color = true;
  this->clear();
}

cmbNucCoregen::~cmbNucCoregen()
{
}

void cmbNucCoregen::clearMeshDisplayMaterial()
{
  for(size_t i = 0; i < this->MeshDisplayedMaterial.size(); ++i)
  {
    if(this->MeshDisplayedMaterial[i] != NULL)
    {
      this->MeshDisplayedMaterial[i]->dec();
      this->MeshDisplayedMaterial[i]->clearDisplayed(cmbNucMaterial::MESH);
    }
  }
  this->MeshDisplayedMaterial.clear();
}


void cmbNucCoregen::selectionChanged(QTreeWidgetItem * item)
{
  this->clearMeshDisplayMaterial();
  MeshTreeItem * mitem = dynamic_cast<MeshTreeItem *>(item);
  if(mitem == NULL) return;
  this->selectedType = static_cast<unsigned int>(mitem->rootId);
  this->subSection = mitem->subId;
  this->MeshDisplayedMaterial = mitem->materials;
  for(size_t i = 0; i < this->MeshDisplayedMaterial.size(); ++i)
  {
    if(this->MeshDisplayedMaterial[i] != NULL)
    {
      this->MeshDisplayedMaterial[i]->inc();
      this->MeshDisplayedMaterial[i]->setDisplayed(cmbNucMaterial::MESH);
    }
  }
  this->Data = mitem->data;
  emit(update());
}

void cmbNucCoregen::valueChanged(QTreeWidgetItem * item)
{
  MeshTreeItem * mitem = dynamic_cast<MeshTreeItem *>(item);
  if(mitem == NULL) return;
  bool value = item->checkState(0)!=0;
  assert(mitem->rootId<this->SubPartVisible.size());
  assert(static_cast<size_t>(mitem->subId)<this->SubPartVisible[mitem->rootId].size());
  qDebug() << "changing value:" << mitem->rootId << mitem->subId << this->SubPartVisible[mitem->rootId][mitem->subId] << "to" << item << this->SubPartVisible.size() << " " << this->SubPartVisible[mitem->rootId].size();
  this->SubPartVisible[mitem->rootId][mitem->subId] = value;
  emit(update());
}

unsigned int cmbNucCoregen::numberOfParts()
{
  if(this->Data == NULL) return 0;
  if(this->MeshDisplayedMaterial.empty())
  {
    vtkMultiBlockDataSet* sec = vtkMultiBlockDataSet::SafeDownCast(this->Data);
    if(sec != NULL)
    {
      return sec->GetNumberOfBlocks();
    }
    else
    {
      return 1;
    }
  }
  else
  {
    return static_cast<unsigned int>(this->MeshDisplayedMaterial.size());
  }
}

void cmbNucCoregen::getColor(int i, QColor & color_out, bool & visible)
{
  int offset = static_cast<int>(this->MeshDisplayedMaterial.size())-1;
  int idx = (this->subSection == -1)?(i+offset)%(this->MeshDisplayedMaterial.size()):(i);
  bool tmpv = this->SubPartVisible[this->selectedType][(this->subSection == -1)?idx:this->subSection];
  switch(this->color)
  {
    case 1:
    {
      if(static_cast<size_t>(i) < this->MeshDisplayedMaterial.size() && this->MeshDisplayedMaterial[idx] != NULL)
      {
        color_out = this->MeshDisplayedMaterial[idx]->getColor();
        visible = this->MeshDisplayedMaterial[idx]->isVisible() && tmpv;
      }
      else
      {
        color_out = cmbNucMaterialColors::instance()->getUnknownMaterial()->getColor();
        visible = cmbNucMaterialColors::instance()->getUnknownMaterial()->isVisible() && tmpv;
      }
      break;
    }
    case 2:
    {
      unsigned int cind = ((isSubSection())?subSection:idx)%(numAssemblyDefaultColors-1);

      color_out = QColor( defaultAssemblyColors[cind][0],
                          defaultAssemblyColors[cind][1],
                          defaultAssemblyColors[cind][2] );
      visible = tmpv;
      break;
    }
    break;
    default:
      color_out = QColor( 255, 255, 255 );
      visible = tmpv;
  }
}

void cmbNucCoregen::clear()
{
  this->MoabReader = vtkSmartPointer<vtkMoabReader>::New();
  this->Data = NULL;
  this->DataSets.clear();
  this->FileName = QString();
  this->clearMeshDisplayMaterial();
  this->SubPartVisible.clear();
  emit fileOpen(false);
}

vtkSmartPointer<vtkDataObject>
cmbNucCoregen::getData()
{
  return this->Data;
}

void cmbNucCoregen::computeBounds(vtkBoundingBox * box)
{
  vtkMultiBlockDataSet * dataset = vtkMultiBlockDataSet::SafeDownCast(this->DataSets[this->selectedType]);
  if(dataset == NULL) return;
  ::computeBounds(dataset, box);
}

void cmbNucCoregen::rootChanged(int i)
{
  int unknown = 0;
  if(static_cast<size_t>(i) > DataSets.size()) return;
  QList<QTreeWidgetItem*> roots;
  vtkSmartPointer<vtkDataObject> dataObj = DataSets[i];
  if(dataObj == NULL) return;
  MeshTreeItem * root = new MeshTreeItem(NULL, i, -1, DataSets[i]);
  root->setText(0, QString(this->Names[i].c_str()));
  vtkMultiBlockDataSet* sec = vtkMultiBlockDataSet::SafeDownCast(dataObj);
  if(sec == NULL) return;
  QString gname;
  root->materials.resize(sec->GetNumberOfBlocks());
  Qt::ItemFlags matFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable |
                         Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
  for(unsigned int idx=0; idx < sec->GetNumberOfBlocks(); idx++)
  {
    const char * name = sec->GetMetaData(idx)->Get(vtkCompositeDataSet::NAME());
    QString label(name);
    if(label.isEmpty())
    {
      label = "Unlabeled " + QString::number(unknown++);
    }
    MeshTreeItem* node = new MeshTreeItem(root, i, static_cast<int>(idx), sec->GetBlock(idx));
    node->setText(2,gname);
    node->setText(1, label);
    node->setFlags(matFlags);
    node->setCheckState(0, (this->SubPartVisible[i][idx])?(Qt::Checked):(Qt::Unchecked));
    if(i == 3 || i==5)
    {
      QString matname = cmbNucMaterialColors::createMaterialLabel(name);
      QPointer<cmbNucMaterial> m = cmbNucMaterialColors::instance()->getMaterialByName(matname);
      node->materials.push_back(m);
      root->materials[idx] = m;
    }
  }
  roots.append(root);
  emit components(roots);
}

void cmbNucCoregen::openFile(QString file)
{
  if(file.isEmpty()) return;
  this->FileName = file;
  this->MoabReader->SetFileName(file.toStdString().c_str());
  this->MoabReader->Modified();
  this->MoabReader->Update();
  vtkSmartPointer<vtkMultiBlockDataSet> tmp = this->MoabReader->GetOutput();
  DataSets.resize(tmp->GetNumberOfBlocks());
  this->GeoFilt.resize(tmp->GetNumberOfBlocks(), NULL);

  qDebug() << "there are " << tmp->GetNumberOfBlocks() << "blocks";
  this->SubPartVisible.resize(tmp->GetNumberOfBlocks());
  this->Names.resize(tmp->GetNumberOfBlocks());
  QStringList list;

  cmbNucMaterialColors::instance()->blockSignals(true);
  for (unsigned int i = 0; i < tmp->GetNumberOfBlocks(); ++i)
  {
    if(this->GeoFilt[i] == NULL) this->GeoFilt[i] = vtkSmartPointer<vtkGeometryFilter>::New();
    const char * pname = tmp->GetMetaData(i)->Get(vtkCompositeDataSet::NAME());
    this->Names[i] = pname;
    list.append(QString(pname));
    this->GeoFilt[i]->SetInputData(tmp->GetBlock(i));
    this->GeoFilt[i]->Update();
    DataSets[i] = vtkSmartPointer<vtkDataObject>::NewInstance(this->GeoFilt[i]->GetOutputDataObject(0));
    DataSets[i]->DeepCopy(this->GeoFilt[i]->GetOutputDataObject(0));
    vtkMultiBlockDataSet* sec = vtkMultiBlockDataSet::SafeDownCast(DataSets[i]);
    if(sec == NULL) continue;
    qDebug() << "\tFor" << i << "there are " << sec->GetNumberOfBlocks() << "blocks";
    this->SubPartVisible[i].resize(sec->GetNumberOfBlocks(), true);
    for(unsigned int idx=0; idx < sec->GetNumberOfBlocks(); idx++)
    {
      const char * name = sec->GetMetaData(idx)->Get(vtkCompositeDataSet::NAME());
      QString matname = cmbNucMaterialColors::createMaterialLabel(name);
      if(!matname.isEmpty() &&
         cmbNucMaterialColors::instance()->getMaterialByName(matname) ==
         cmbNucMaterialColors::instance()->getUnknownMaterial())
      {
        cmbNucMaterialColors::instance()->AddMaterial(matname,matname);
      }
    }
  }
  cmbNucMaterialColors::instance()->blockSignals(false);
  emit fileOpen(true);
  emit components(list,5);
  emit resetCamera();
  cmbNucMaterialColors::instance()->testShow();
}

void cmbNucCoregen::exportVisible(QString outFname, std::vector<std::string> const& remove )
{
  if(FileName.isEmpty()) return;
  extract_subset::extract(FileName.toStdString(), outFname.toStdString(), this->SubPartVisible, remove);
}

void cmbNucCoregen::setColor(int c)
{
  this->color = c;
  emit update();
}
