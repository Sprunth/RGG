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

class MeshTreeItem : public QTreeWidgetItem
{
public:
  MeshTreeItem(QTreeWidgetItem* pNode, size_t r, int s, vtkSmartPointer<vtkDataObject> d)
  :QTreeWidgetItem(pNode)
  {
    data = d;
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
  this->selectedType = mitem->rootId;
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
  assert(mitem->subId<this->SubPartVisible[mitem->rootId].size());
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
    return this->MeshDisplayedMaterial.size();
  }
}

void cmbNucCoregen::getColor(int i, QColor & color, bool & visible)
{
  int offset = this->MeshDisplayedMaterial.size()-1;
  int idx = (this->subSection == -1)?(i+offset)%(this->MeshDisplayedMaterial.size()):i;
  bool tmpv = this->SubPartVisible[this->selectedType][(this->subSection == -1)?idx:this->subSection];
  if(this->color)
  {
    if(this->selectedType == 3 || this->selectedType == 5)
    {
      if(static_cast<size_t>(i) < this->MeshDisplayedMaterial.size() && this->MeshDisplayedMaterial[idx] != NULL)
      {
        color = this->MeshDisplayedMaterial[idx]->getColor();
        visible = this->MeshDisplayedMaterial[idx]->isVisible() && tmpv;
        return;
      }
      else
      {
        color = cmbNucMaterialColors::instance()->getUnknownMaterial()->getColor();
        visible = cmbNucMaterialColors::instance()->getUnknownMaterial()->isVisible() && tmpv;
        return;
      }
    }
    unsigned int cind = ((isSubSection())?subSection:i)%(numAssemblyDefaultColors-1);

    color = QColor( defaultAssemblyColors[cind][0],
                    defaultAssemblyColors[cind][1],
                    defaultAssemblyColors[cind][2]);
    visible = tmpv;
    return;
  }
  color = QColor( 255, 255, 255 );
  visible = tmpv;
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

void cmbNucCoregen::openFile(QString file)
{
  if(file.isEmpty()) return;
  this->FileName = file;
  this->MoabReader->SetFileName(file.toStdString().c_str());
  this->MoabReader->Modified();
  this->MoabReader->Update();
  vtkSmartPointer<vtkMultiBlockDataSet> tmp = this->MoabReader->GetOutput();  DataSets.resize(tmp->GetNumberOfBlocks());
  this->GeoFilt.resize(tmp->GetNumberOfBlocks(), NULL);
  QList<QTreeWidgetItem*> roots;

  Qt::ItemFlags matFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable |
                         Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
  int unknown = 0;
  this->SubPartVisible.resize(tmp->GetNumberOfBlocks());

  for (unsigned int i = 0; i < tmp->GetNumberOfBlocks(); ++i)
  {
    if(this->GeoFilt[i] == NULL) this->GeoFilt[i] = vtkSmartPointer<vtkGeometryFilter>::New();
    const char * pname = tmp->GetMetaData(i)->Get(vtkCompositeDataSet::NAME());
    this->GeoFilt[i]->SetInputData(tmp->GetBlock(i));
    this->GeoFilt[i]->Update();
    DataSets[i].TakeReference(this->GeoFilt[i]->GetOutputDataObject(0)->NewInstance());
    DataSets[i]->DeepCopy(this->GeoFilt[i]->GetOutputDataObject(0));
    vtkSmartPointer<vtkDataObject> dataObj = DataSets[i];
    if(dataObj == NULL) continue;
    //QTreeWidgetItem * root = new QTreeWidgetItem();
    MeshTreeItem * root = new MeshTreeItem(NULL, i, -1, DataSets[i]);
    root->setText(0, QString(pname));
    vtkMultiBlockDataSet* sec = vtkMultiBlockDataSet::SafeDownCast(dataObj);
    if(sec == NULL) continue;
    int offset = sec->GetNumberOfBlocks()-1;
    QString gname;
    root->materials.resize(sec->GetNumberOfBlocks());
    this->SubPartVisible[i].resize(sec->GetNumberOfBlocks());
    for(unsigned int idx=0; idx < sec->GetNumberOfBlocks(); idx++)
    {
      const char * name = sec->GetMetaData(idx)->Get(vtkCompositeDataSet::NAME());
      QString label(name);
      QString matname = cmbNucMaterialColors::createMaterialLabel(name);
      if(!matname.isEmpty() &&
         cmbNucMaterialColors::instance()->getMaterialByName(matname) == cmbNucMaterialColors::instance()->getUnknownMaterial())
      {
        cmbNucMaterialColors::instance()->AddMaterial(matname,matname);
      }
      if(label.isEmpty())
      {
        label = "Unlabeled " + QString::number(unknown++);
      }
      MeshTreeItem* node = new MeshTreeItem(root, i, static_cast<int>(idx), sec->GetBlock(idx));
      node->setText(2,gname);
      node->setText(1, label);
      node->setFlags(matFlags);
      node->setCheckState(0, Qt::Checked);
      assert(idx < this->SubPartVisible[i].size());
      this->SubPartVisible[i][idx] = true;
      assert(this->SubPartVisible[i][idx]);
      if(i == 3 || i==5)
      {
        QPointer<cmbNucMaterial> m = cmbNucMaterialColors::instance()->getMaterialByName(matname);
        node->materials.push_back(m);
        root->materials[idx] = m;
      }
    }
    roots.append(root);
  }
  emit fileOpen(true);
  emit components(roots);
}

void cmbNucCoregen::exportVisible(QString outFname, std::vector<std::string> const& remove )
{
  if(FileName.isEmpty()) return;
  extract_subset::extract(FileName.toStdString(), outFname.toStdString(), this->SubPartVisible, remove);
}

void cmbNucCoregen::setColor(bool c)
{
  this->color = c;
  emit update();
}
