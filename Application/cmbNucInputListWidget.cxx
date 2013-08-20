
#include "cmbNucInputListWidget.h"
#include "ui_qInputListWidget.h"

#include "cmbNucAssembly.h"
#include "cmbNucCore.h"
#include "cmbNucPartsTreeItem.h"

#include <QLabel>
#include <QPointer>
#include <QTreeWidget>
#include <QAction>

class cmbNucInputListWidgetInternal : 
  public Ui::InputListWidget
{
public:
  cmbNucInputListWidgetInternal()
  {
    this->RootCoreNode = NULL;
  }
  void initActions()
    {
    this->Action_NewAssembly = new QAction("Create Assembly", this->PartsList);
    this->Action_NewPin = new QAction("Create Pin", this->PartsList);
    //this->Action_NewFrustum = new QAction("Create Frustum", this->PartsList);
    //this->Action_NewCylinder = new QAction("Create Cylinder", this->PartsList);
    this->Action_NewDuct = new QAction("Create Duct", this->PartsList);
    this->Action_DeletePart = new QAction("Delete Selected", this->PartsList);
    this->PartsList->addAction(this->Action_NewAssembly);
    this->PartsList->addAction(this->Action_NewPin);
    //this->PartsList->addAction(this->Action_NewFrustum);
    //this->PartsList->addAction(this->Action_NewCylinder);
    this->PartsList->addAction(this->Action_NewDuct);
    this->PartsList->addAction(this->Action_DeletePart);

    this->Action_NewMaterial = new QAction("Create Material", this->MaterialList);
    this->Action_DeleteMaterial = new QAction("Delete Selected", this->MaterialList);
    this->MaterialList->addAction(this->Action_NewMaterial);
    this->MaterialList->addAction(this->Action_DeleteMaterial);  
    }

  QPointer<QAction> Action_NewAssembly;
  QPointer<QAction> Action_NewPin;
  //QPointer<QAction> Action_NewFrustum;
  //QPointer<QAction> Action_NewCylinder;
  QPointer<QAction> Action_NewDuct;
  QPointer<QAction> Action_DeletePart;

  QPointer<QAction> Action_DeleteMaterial;
  QPointer<QAction> Action_NewMaterial;

  cmbNucPartsTreeItem* RootCoreNode;
};

//-----------------------------------------------------------------------------
cmbNucInputListWidget::cmbNucInputListWidget(
  QWidget* _p): QWidget(_p)
{
  this->Internal = new cmbNucInputListWidgetInternal;
  this->Internal->setupUi(this);
  this->Internal->initActions();

  this->Internal->PartsList->setAlternatingRowColors(true);
  this->Internal->MaterialList->setAlternatingRowColors(true);
  
  // set up the UI trees
  QTreeWidget* treeWidget = this->Internal->PartsList;

  // context menu for parts tree
  QObject::connect(this->Internal->Action_NewAssembly, SIGNAL(triggered()),
    this, SLOT(onNewAssembly()));
  //QObject::connect(this->Internal->Action_NewCylinder, SIGNAL(triggered()),
  //  this, SLOT(onNewCylinder()));
  QObject::connect(this->Internal->Action_NewDuct, SIGNAL(triggered()),
    this, SLOT(onNewDuct()));
  //QObject::connect(this->Internal->Action_NewFrustum, SIGNAL(triggered()),
  // this, SLOT(onNewFrustum()));
  QObject::connect(this->Internal->Action_NewPin, SIGNAL(triggered()),
    this, SLOT(onNewPin()));
  QObject::connect(this->Internal->Action_DeletePart, SIGNAL(triggered()),
    this, SLOT(onRemoveSelectedPart()));

  QObject::connect(this->Internal->Action_NewMaterial, SIGNAL(triggered()),
    this, SLOT(onNewMaterial()));
  QObject::connect(this->Internal->Action_DeleteMaterial, SIGNAL(triggered()),
    this, SLOT(onRemoveMaterial()));

  //QObject::connect(this->Internal->PartsList,
  //  SIGNAL(dragStarted(QTreeWidget*)),
  //  this, SLOT(onDragStarted(QTreeWidget*)), Qt::QueuedConnection);
  QObject::connect(this->Internal->PartsList, SIGNAL(itemSelectionChanged()),
    this, SLOT(onPartsSelectionChanged()), Qt::QueuedConnection);
  QObject::connect(this->Internal->MaterialList, SIGNAL(itemSelectionChanged()),
    this, SLOT(onMaterialSelectionChanged()), Qt::QueuedConnection);
  QObject::connect(this->Internal->MaterialList, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
    this, SLOT(onMaterialNameChanged(QTreeWidgetItem*, int)));

  QObject::connect(this->Internal->tabInputs, SIGNAL(currentChanged(int)),
    this, SLOT(onTabChanged(int)));

  this->initUI();
}

//-----------------------------------------------------------------------------
cmbNucInputListWidget::~cmbNucInputListWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::setCore(cmbNucCore* core)
{
  if(this->NuclearCore == core)
    {
    return;
    }
  this->NuclearCore = core;
}
//-----------------------------------------------------------------------------
cmbNucPartsTreeItem* cmbNucInputListWidget::getDuctCellNode(
  cmbNucPartsTreeItem* assyNode)
{
  if(!assyNode || assyNode->childCount()==0)
    {
    return NULL;
    }
  for(int i=0; i< assyNode->childCount(); i++)
    {
    cmbNucPartsTreeItem* assyItem = dynamic_cast<cmbNucPartsTreeItem*>(
    assyNode->child(i));
    if(assyItem && assyItem->getPartObject() &&
      assyItem->getPartObject()->GetType() == CMBNUC_ASSY_DUCTCELL)
      {
      return assyItem;
      }
    }
  return NULL;
}
//-----------------------------------------------------------------------------
cmbNucPartsTreeItem* cmbNucInputListWidget::getCurrentAssemblyNode()
{
  cmbNucPartsTreeItem* selItem = this->getSelectedItem(
    this->Internal->PartsList);
  if(!selItem || !selItem->getPartObject())
    {
    return NULL;
    }

  AssyPartObj* selObj = selItem->getPartObject();
  QTreeWidgetItem* pItem=NULL;
  enumNucPartsType selType = selObj->GetType();
  switch(selType)
    {
    case CMBNUC_ASSY_LATTICE:
    case CMBNUC_ASSY_DUCTCELL:
    case CMBNUC_ASSY_PINCELL:
      pItem = selItem->parent();
      break;
    case CMBNUC_ASSY_FRUSTUM_PIN:
    case CMBNUC_ASSY_CYLINDER_PIN:
    case CMBNUC_ASSY_RECT_DUCT:
      pItem = selItem->parent()->parent();
      break;
    case CMBNUC_ASSEMBLY:
      pItem = selItem;
      break;
    case CMBNUC_CORE:
      if(selItem->childCount()==1)
        {
        pItem = selItem->child(0);
        }
      break;
    default:
      break;
    }
  return pItem ? dynamic_cast<cmbNucPartsTreeItem*>(pItem) : NULL;
}

//-----------------------------------------------------------------------------
cmbNucAssembly* cmbNucInputListWidget::getCurrentAssembly()
{
  cmbNucPartsTreeItem* assyItem = this->getCurrentAssemblyNode();
  if(assyItem)
    {
    return dynamic_cast<cmbNucAssembly*>(assyItem->getPartObject());
    }
  return NULL;
}

//----------------------------------------------------------------------------
void cmbNucInputListWidget::initUI()
{
  this->setActionsEnabled(false);
  if(this->Internal->RootCoreNode)
    {
    delete this->Internal->RootCoreNode;
    this->Internal->RootCoreNode = NULL;
    }
  this->initTree(this->Internal->PartsList);
  this->initTree(this->Internal->MaterialList);
}
//----------------------------------------------------------------------------
void cmbNucInputListWidget::onTabChanged(int currentTab)
{
  if(currentTab == 0) // parts
    {
    this->onPartsSelectionChanged();
    }
  else if(currentTab == 1) // materials
    {
    this->onMaterialSelectionChanged();
    }
}
//----------------------------------------------------------------------------
void cmbNucInputListWidget::setActionsEnabled(bool val)
{
  this->Internal->Action_NewAssembly->setEnabled(val);
  this->Internal->Action_NewPin->setEnabled(val);
  this->Internal->Action_NewDuct->setEnabled(val);
  //this->Internal->Action_NewFrustum->setEnabled(val);
  //this->Internal->Action_NewCylinder->setEnabled(val);
  this->Internal->Action_DeletePart->setEnabled(val);
}
//----------------------------------------------------------------------------
void cmbNucInputListWidget::onNewAssembly()
{
    const char  *defaultMaterials[] = 
        {"Fuel1", "Fuel2", "Fuel_uox1", "Fuel_uox2", 
         "MOX_43", "MOX_73", "MOX_87", 
         "Water", "Coolant", "Water_Rod", "Control_Rod", "BARod16", "BARod18", "BARod28", "Graphite", "Cladding", "Gap", "Metal", "MaterialBlock", "end"};
  this->setEnabled(1);
  cmbNucAssembly* assembly = new cmbNucAssembly;
  assembly->label = QString("Assy").append(
    QString::number(this->NuclearCore->GetNumberOfAssemblies()+1)).toStdString();
  std::string s;
  int i;
  Material *mat;
  for (i = 0;; i++)
      {
      s = defaultMaterials[i];
      if (s == "end")
          {
          break;
          }
      mat =  new Material();
      mat->name = s;
      mat->label = s;
      assembly->AddMaterial(mat);
      }

  this->NuclearCore->AddAssembly(assembly);
  this->initCoreRootNode();
  this->updateWithAssembly(assembly);
}

//----------------------------------------------------------------------------
void cmbNucInputListWidget::onNewCylinder()
{
  cmbNucPartsTreeItem* selItem = this->getSelectedItem(
    this->Internal->PartsList);
  if(!selItem || !selItem->getPartObject())
    {
    return;
    }

  cmbNucPartsTreeItem* pinItem = selItem;
  if(selItem->getPartObject()->GetType() == CMBNUC_ASSY_CYLINDER_PIN ||
    selItem->getPartObject()->GetType() == CMBNUC_ASSY_FRUSTUM_PIN)
    {
    pinItem = dynamic_cast<cmbNucPartsTreeItem*>(selItem->parent());
    }
  PinCell* pincell = dynamic_cast<PinCell*>(pinItem->getPartObject());
  if(pincell)
    {
    Cylinder* newcy = new Cylinder();
    pincell->cylinders.push_back(newcy);
    cmbNucPartsTreeItem* cNode = new cmbNucPartsTreeItem(selItem, newcy);
    cNode->setText(0, QString("cylinder").append(
      QString::number(pincell->cylinders.size())));
    Qt::ItemFlags itemFlags(
      Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    cNode->setFlags(itemFlags);
    selItem->setSelected(false);
    selItem->setExpanded(true);
    cNode->setSelected(true);
    this->onPartsSelectionChanged();
    }  
}

//----------------------------------------------------------------------------
void cmbNucInputListWidget::onNewDuct()
{
  QTreeWidgetItem* ductsNode = this->getDuctCellNode(
    this->getCurrentAssemblyNode());
  if(!ductsNode)
    {
    return;
    }
  Duct* newduct = new Duct();
  this->getCurrentAssembly()->AssyDuct.Ducts.push_back(newduct);
  cmbNucPartsTreeItem* dNode = new cmbNucPartsTreeItem(ductsNode, newduct);
  dNode->setText(0, QString("duct").append(QString::number(
    this->getCurrentAssembly()->AssyDuct.Ducts.size())));
  Qt::ItemFlags itemFlags(
    Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  dNode->setFlags(itemFlags); // not editable

  cmbNucPartsTreeItem* selItem = this->getSelectedItem(
    this->Internal->PartsList);
  if(selItem)
    {
    selItem->setSelected(false);
    }
  dNode->setSelected(true);
  this->onPartsSelectionChanged();
}
//----------------------------------------------------------------------------
void cmbNucInputListWidget::onNewFrustum()
{
  cmbNucPartsTreeItem* selItem = this->getSelectedItem(
    this->Internal->PartsList);
  if(!selItem || !selItem->getPartObject())
    {
    return;
    }
  cmbNucPartsTreeItem* pinItem = selItem;
  if(selItem->getPartObject()->GetType() == CMBNUC_ASSY_CYLINDER_PIN ||
    selItem->getPartObject()->GetType() == CMBNUC_ASSY_FRUSTUM_PIN)
    {
    pinItem = dynamic_cast<cmbNucPartsTreeItem*>(selItem->parent());
    }
  PinCell* pincell = dynamic_cast<PinCell*>(pinItem->getPartObject());
  if(pincell)
    {
    Frustum* newfrust = new Frustum();
    pincell->frustums.push_back(newfrust);
    cmbNucPartsTreeItem* fNode = new cmbNucPartsTreeItem(selItem, newfrust);
    fNode->setText(0, QString("frustum").append(
      QString::number(pincell->frustums.size())));
    Qt::ItemFlags itemFlags(
      Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    fNode->setFlags(itemFlags);

    selItem->setSelected(false);
    selItem->setExpanded(true);
    fNode->setSelected(true);

    this->onPartsSelectionChanged();
    }  
}

//----------------------------------------------------------------------------
void cmbNucInputListWidget::onNewPin()
{
  PinCell* newpin = new PinCell();
  QString pinname = QString("PinCell").append(
    QString::number(this->getCurrentAssembly()->PinCells.size()+1));
  newpin->name = newpin->label = pinname.toStdString();
  this->getCurrentAssembly()->AddPinCell(newpin);
  QTreeWidgetItem* partsRoot = this->getCurrentAssemblyNode();
  if(!partsRoot)
    {
    return;
    }
  cmbNucPartsTreeItem* pinNode = new cmbNucPartsTreeItem(partsRoot, newpin);
  pinNode->setText(0, newpin->name.c_str());
  Qt::ItemFlags itemFlags(
    Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  pinNode->setFlags(itemFlags); // not editable
  pinNode->setChildIndicatorPolicy(
    QTreeWidgetItem::DontShowIndicatorWhenChildless);
  cmbNucPartsTreeItem* selItem = this->getSelectedItem(
    this->Internal->PartsList);
  if(selItem)
    {
    selItem->setSelected(false);
    }
  pinNode->setSelected(true);
  this->onPartsSelectionChanged();
}

//----------------------------------------------------------------------------
void cmbNucInputListWidget::onRemoveSelectedPart()
{
  cmbNucPartsTreeItem* selItem = this->getSelectedItem(
    this->Internal->PartsList);
  if(!selItem || !selItem->getPartObject())
    {
    return;
    }
  bool objRemoved = true;
  AssyPartObj* selObj = selItem->getPartObject();
  cmbNucPartsTreeItem* pItem=NULL;
  PinCell* pincell=NULL;

  enumNucPartsType selType = selObj->GetType();
  std::string selText = selItem->text(0).toStdString();
  switch(selType)
  {
  case CMBNUC_ASSEMBLY:
    this->NuclearCore->RemoveAssembly(selText);
    break;
  case CMBNUC_ASSY_PINCELL:
    pincell = dynamic_cast<PinCell*>(selObj);
    if(pincell)
      {
      this->getCurrentAssembly()->RemovePinCell(pincell->label);
      }
    delete selItem;
    break;
  case CMBNUC_ASSY_FRUSTUM_PIN:
  // find pinCell first
    pItem = dynamic_cast<cmbNucPartsTreeItem*>(selItem->parent());
    if(pItem)
      {
      pincell = dynamic_cast<PinCell*>(pItem->getPartObject());
      if(pincell)
        {
        pincell->RemoveFrustum(dynamic_cast<Frustum*>(selObj));
        delete selItem;
        }
      }
    break;
  case CMBNUC_ASSY_CYLINDER_PIN:
    // find pinCell first
    pItem = dynamic_cast<cmbNucPartsTreeItem*>(selItem->parent());
    if(pItem)
      {
      pincell = dynamic_cast<PinCell*>(pItem->getPartObject());
      if(pincell)
        {
        pincell->RemoveCylinder(dynamic_cast<Cylinder*>(selObj));
        delete selItem;
        }
      }
    break;
  case CMBNUC_ASSY_RECT_DUCT:
    this->getCurrentAssembly()->AssyDuct.RemoveDuct(dynamic_cast<Duct*>(selObj));
    delete selItem;
    break;
  default:
    objRemoved = false;
    break;
  }
  if(objRemoved)
    {
    emit this->objectRemoved();
    }
}
//----------------------------------------------------------------------------
void cmbNucInputListWidget::onNewMaterial()
{
  QTreeWidgetItem* matRoot = this->Internal->MaterialList->invisibleRootItem();
  Qt::ItemFlags matFlags(
    Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
  Material* newmat = new Material();
  this->getCurrentAssembly()->AddMaterial(newmat);
  size_t numM = this->getCurrentAssembly()->Materials.size();
  QString matname = QString("material").append(
    QString::number(numM));
  newmat->name = newmat->label = matname.toStdString();
  cmbNucPartsTreeItem* mNode = new cmbNucPartsTreeItem(matRoot, newmat);
  mNode->setText(0, newmat->name.c_str());
  mNode->setFlags(matFlags); // editable
  cmbNucPartsTreeItem* selItem = this->getSelectedItem(
    this->Internal->MaterialList);
  if(selItem)
    {
    selItem->setSelected(false);
    }
  mNode->setSelected(true);
  this->onMaterialSelectionChanged();
}
//----------------------------------------------------------------------------
void cmbNucInputListWidget::onRemoveMaterial()
{
  cmbNucPartsTreeItem* selItem = this->getSelectedItem(
    this->Internal->MaterialList);
  if(!selItem || !selItem->getPartObject())
    {
    return;
    }

  Material* material = dynamic_cast<Material*>(selItem->getPartObject());
  if(material)
    {
    this->getCurrentAssembly()->RemoveMaterial(material->name);
    }
  delete selItem;
  emit this->objectRemoved();
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::updateUI(bool selCore)
{
  this->initUI();
  if(!this->NuclearCore)
    {
    this->setEnabled(0);
    return;
    }
  this->setEnabled(1);
  // Core node
  this->initCoreRootNode();
  // Assembly nodes
  for(int i=0; i<this->NuclearCore->GetNumberOfAssemblies(); i++)
    {
    this->updateWithAssembly(this->NuclearCore->GetAssembly(i),
      (!selCore && i == (this->NuclearCore->GetNumberOfAssemblies()-1)));
    }

  if(selCore)
    {
    this->Internal->RootCoreNode->setSelected(true);
    this->onPartsSelectionChanged();
    }
}
//-----------------------------------------------------------------------------
void cmbNucInputListWidget::initCoreRootNode()
{
  if(this->Internal->RootCoreNode == NULL)
    {
    Qt::ItemFlags itemFlags(
      Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    this->Internal->RootCoreNode = new cmbNucPartsTreeItem(
      this->Internal->PartsList->invisibleRootItem(), this->NuclearCore);
    this->Internal->RootCoreNode->setText(0, "Core");
    this->Internal->RootCoreNode->setFlags(itemFlags); // not editable
    this->Internal->RootCoreNode->setChildIndicatorPolicy(
      QTreeWidgetItem::DontShowIndicatorWhenChildless);
    }
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::updateMaterial(cmbNucAssembly* assy)
{
  if(!assy)
    {
    this->Internal->MaterialList->setEnabled(false);
    return;
    }
  this->Internal->MaterialList->setEnabled(true);
  this->Internal->MaterialList->blockSignals(true);
  this->Internal->MaterialList->clear();
  QTreeWidgetItem* matRoot = this->Internal->MaterialList->invisibleRootItem();
  Qt::ItemFlags matFlags(
    Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);

  for(size_t i = 0; i < assy->Materials.size(); i++)
    {
    Material *material = assy->Materials[i];
    cmbNucPartsTreeItem* mNode = new cmbNucPartsTreeItem(matRoot, material);
    mNode->setText(0, material->name.c_str());
    mNode->setFlags(matFlags);
    }

  this->Internal->MaterialList->expandAll();
  this->Internal->MaterialList->blockSignals(false);
}

void cmbNucInputListWidget::updateWithAssembly(cmbNucAssembly* assy, bool select)
{
  this->Internal->PartsList->blockSignals(true);
  // Assembly node
  Qt::ItemFlags itemFlags(
    Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  cmbNucPartsTreeItem* assyNode = new cmbNucPartsTreeItem(
    this->Internal->RootCoreNode, assy);
  assyNode->setText(0, assy->label.c_str());
  assyNode->setFlags(itemFlags); // not editable
  assyNode->setChildIndicatorPolicy(
    QTreeWidgetItem::DontShowIndicatorWhenChildless);

    // | Qt::ItemIsDropEnabled | Qt::ItemIsEditable);

  /// ******** populate parts tree ********
  QTreeWidgetItem* partsRoot = assyNode;

  // lattice
  cmbNucPartsTreeItem* latticeNode = new cmbNucPartsTreeItem(partsRoot,
    &assy->AssyLattice);
  latticeNode->setText(0, "Lattice");
  latticeNode->setFlags(itemFlags); // not editable

  // ducts
  cmbNucPartsTreeItem* ductsNode = new cmbNucPartsTreeItem(partsRoot,
    &assy->AssyDuct);
  ductsNode->setText(0, "Ducts");
  ductsNode->setFlags(itemFlags); // not editable
  ductsNode->setChildIndicatorPolicy(
    QTreeWidgetItem::DontShowIndicatorWhenChildless);
  for(size_t i = 0; i < assy->AssyDuct.Ducts.size(); i++)
    {
    Duct *duct = assy->AssyDuct.Ducts[i];
    cmbNucPartsTreeItem* dNode = new cmbNucPartsTreeItem(ductsNode, duct);
    dNode->setText(0, QString("duct").append(QString::number(i+1)));
    dNode->setFlags(itemFlags); // not editable
    }

  // pincells
  for(size_t i = 0; i < assy->PinCells.size(); i++)
    {
    PinCell *pincell = assy->PinCells[i];
    cmbNucPartsTreeItem* pinNode = new cmbNucPartsTreeItem(partsRoot, pincell);
    //pinNode->setText(0, QString("PinCell").append(QString::number(i+1)));
    pinNode->setText(0, QString::fromStdString(pincell->label));
    pinNode->setFlags(itemFlags); // not editable
    pinNode->setChildIndicatorPolicy(
      QTreeWidgetItem::DontShowIndicatorWhenChildless);
    for(size_t j = 0; j < pincell->cylinders.size(); j++)
      {
      Cylinder *cylin = pincell->cylinders[j];
      cmbNucPartsTreeItem* cNode = new cmbNucPartsTreeItem(pinNode, cylin);
      cNode->setText(0, QString("cylinder").append(QString::number(j+1)));
      cNode->setFlags(itemFlags);
      }

    for(size_t j = 0; j < pincell->frustums.size(); j++)
      {
      Frustum *frust = pincell->frustums[j];
      cmbNucPartsTreeItem* fNode = new cmbNucPartsTreeItem(pinNode, frust);
      fNode->setText(0, QString("frustum").append(QString::number(j+1)));
      fNode->setFlags(itemFlags);
      }
    }

  this->Internal->PartsList->expandAll();
  this->Internal->PartsList->blockSignals(false);

  /// ******** populate materials tree ********
  this->updateMaterial(assy);

  if(select)
    {
    latticeNode->setSelected(true);
    this->onPartsSelectionChanged(); 
    }
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::onPartsSelectionChanged()
{
  cmbNucPartsTreeItem* selItem = this->getSelectedItem(
    this->Internal->PartsList);
  this->updateContextMenu(selItem ? selItem->getPartObject() : NULL);
  this->updateMaterial(this->getCurrentAssembly());
  this->fireObjectSelectedSignal(selItem);
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::updateContextMenu(AssyPartObj* selObj)
{
  if(!selObj)
    {
    this->setActionsEnabled(false);
    return;
    }
  this->setActionsEnabled(false);
  this->Internal->Action_NewAssembly->setEnabled(true);
  this->Internal->Action_NewDuct->setEnabled(true);
  this->Internal->Action_NewPin->setEnabled(true);

  switch(selObj->GetType())
  {
  case CMBNUC_CORE:
    this->Internal->Action_NewDuct->setEnabled(false);
    this->Internal->Action_NewPin->setEnabled(false);
    break;
  case CMBNUC_ASSY_PINCELL:
    //this->Internal->Action_NewFrustum->setEnabled(true);
    //this->Internal->Action_NewCylinder->setEnabled(true);
    this->Internal->Action_DeletePart->setEnabled(true);
    break;
  case CMBNUC_ASSY_FRUSTUM_PIN:
    //this->Internal->Action_NewFrustum->setEnabled(true);
    //this->Internal->Action_NewCylinder->setEnabled(true);
    this->Internal->Action_DeletePart->setEnabled(true);
    break;
  case CMBNUC_ASSY_CYLINDER_PIN:
    //this->Internal->Action_NewFrustum->setEnabled(true);
    //this->Internal->Action_NewCylinder->setEnabled(true);
    this->Internal->Action_DeletePart->setEnabled(true);
    break;
  case CMBNUC_ASSY_RECT_DUCT:
    // keep at lease one duct
    this->Internal->Action_DeletePart->setEnabled(
      this->getCurrentAssembly()->AssyDuct.Ducts.size()>1 ? true : false);
    break;
  default:
    break;
  }
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::fireObjectSelectedSignal(
  cmbNucPartsTreeItem* selItem)
{
  if(selItem)
    {
    emit this->objectSelected(selItem->getPartObject(),
      selItem->text(0).toStdString().c_str());
    }
  else
    {
    emit this->objectSelected(NULL, NULL);
    }
}
//-----------------------------------------------------------------------------
void cmbNucInputListWidget::onMaterialSelectionChanged()
{
  cmbNucPartsTreeItem* selItem = this->getSelectedItem(
    this->Internal->MaterialList);
  this->fireObjectSelectedSignal(selItem);  
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::onMaterialNameChanged(
 QTreeWidgetItem* item, int col)
{
  cmbNucPartsTreeItem* selItem = dynamic_cast<cmbNucPartsTreeItem*>(item);
  if(Material* material = dynamic_cast<Material*>(selItem->getPartObject()))
    {
    material->name = item->text(col).toStdString();
    }
}

//-----------------------------------------------------------------------------
cmbNucPartsTreeItem* cmbNucInputListWidget::getSelectedItem(
  QTreeWidget* treeWidget)
{
  QList<QTreeWidgetItem*> selItems = treeWidget->selectedItems();
  return selItems.count()>0 ? dynamic_cast<cmbNucPartsTreeItem*>(
    selItems.value(0)) : NULL;
}

//----------------------------------------------------------------------------
void cmbNucInputListWidget::initTree(QTreeWidget* treeWidget)
{
  treeWidget->blockSignals(true);
  treeWidget->clear();
  treeWidget->setHeaderLabel("Name");
  treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
  treeWidget->setAcceptDrops(false);
  treeWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
  treeWidget->blockSignals(false);
}
