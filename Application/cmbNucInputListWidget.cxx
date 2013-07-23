
#include "cmbNucInputListWidget.h"
#include "ui_qInputListWidget.h"

#include "cmbNucAssembly.h"
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
    this->RootDuctsNode = NULL;
  }
  void initActions()
    {
    this->Action_NewPin = new QAction("Create Pin", this->PartsList);
    this->Action_NewFrustum = new QAction("Create Frustum", this->PartsList);
    this->Action_NewCylinder = new QAction("Create Cylinder", this->PartsList);
    this->Action_NewDuct = new QAction("Create Duct", this->PartsList);
    this->Action_DeletePart = new QAction("Delete Selected", this->PartsList);
    this->PartsList->addAction(this->Action_NewPin);
    this->PartsList->addAction(this->Action_NewFrustum);
    this->PartsList->addAction(this->Action_NewCylinder);
    this->PartsList->addAction(this->Action_NewDuct);
    this->PartsList->addAction(this->Action_DeletePart);

    this->Action_NewMaterial = new QAction("Create Material", this->MaterialList);
    this->Action_DeleteMaterial = new QAction("Delete Selected", this->MaterialList);
    this->MaterialList->addAction(this->Action_NewMaterial);
    this->MaterialList->addAction(this->Action_DeleteMaterial);  
    }

  QPointer<QAction> Action_NewPin;
  QPointer<QAction> Action_NewFrustum;
  QPointer<QAction> Action_NewCylinder;
  QPointer<QAction> Action_NewDuct;
  QPointer<QAction> Action_DeletePart;

  QPointer<QAction> Action_DeleteMaterial;
  QPointer<QAction> Action_NewMaterial;

  cmbNucPartsTreeItem* RootDuctsNode;
};

//-----------------------------------------------------------------------------
cmbNucInputListWidget::cmbNucInputListWidget(
  QWidget* _p): QWidget(_p)
{
  this->Internal = new cmbNucInputListWidgetInternal;
  this->Internal->setupUi(this);
  this->Internal->initActions();
  this->Assembly = NULL;

  // set up the UI trees
  QTreeWidget* treeWidget = this->Internal->PartsList;
  // context menu for parts tree
  QObject::connect(this->Internal->Action_NewCylinder, SIGNAL(triggered()),
    this, SLOT(onNewCylinder()));
  QObject::connect(this->Internal->Action_NewDuct, SIGNAL(triggered()),
    this, SLOT(onNewDuct()));
  QObject::connect(this->Internal->Action_NewFrustum, SIGNAL(triggered()),
    this, SLOT(onNewFrustum()));
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
void cmbNucInputListWidget::setAssembly(cmbNucAssembly *assyObj)
{
  if(this->Assembly == assyObj)
    {
    return;
    }
  this->Assembly = assyObj;
  this->updateUI();
}
//-----------------------------------------------------------------------------
cmbNucAssembly* cmbNucInputListWidget::getAssembly()
{
  return this->Assembly;
}

//----------------------------------------------------------------------------
void cmbNucInputListWidget::initUI()
{
  this->setActionsEnabled(false);
  this->Internal->RootDuctsNode = NULL;
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
  this->Internal->Action_NewPin->setEnabled(val);
  this->Internal->Action_NewDuct->setEnabled(val);
  this->Internal->Action_NewFrustum->setEnabled(val);
  this->Internal->Action_NewCylinder->setEnabled(val);
  this->Internal->Action_DeletePart->setEnabled(val);
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

  PinCell* pincell = dynamic_cast<PinCell*>(selItem->getPartObject());
  if(pincell)
    {
    Cylinder* newcy = new Cylinder();
    pincell->cylinders.push_back(newcy);
    cmbNucPartsTreeItem* cNode = new cmbNucPartsTreeItem(selItem, newcy);
    cNode->setText(0, QString("cylinder").append(
      QString::number(pincell->cylinders.size())));
    Qt::ItemFlags itemFlags(
      Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
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
  QTreeWidgetItem* ductsNode = this->Internal->RootDuctsNode;
  Duct* newduct = new Duct();
  this->Assembly->AssyDuct.Ducts.push_back(newduct);
  cmbNucPartsTreeItem* dNode = new cmbNucPartsTreeItem(ductsNode, newduct);
  dNode->setText(0, QString("duct").append(QString::number(
    this->Assembly->AssyDuct.Ducts.size())));
  Qt::ItemFlags itemFlags(
    Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
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

  PinCell* pincell = dynamic_cast<PinCell*>(selItem->getPartObject());
  if(pincell)
    {
    Frustum* newfrust = new Frustum();
    pincell->frustums.push_back(newfrust);
    cmbNucPartsTreeItem* fNode = new cmbNucPartsTreeItem(selItem, newfrust);
    fNode->setText(0, QString("frustum").append(
      QString::number(pincell->frustums.size())));
    Qt::ItemFlags itemFlags(
      Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
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
  this->Assembly->AddPinCell(newpin);
  QString pinname = QString("PinCell").append(
    QString::number(this->Assembly->PinCells.size()));
  newpin->name = newpin->label = pinname.toStdString();
  QTreeWidgetItem* partsRoot = this->Internal->PartsList->invisibleRootItem();
  cmbNucPartsTreeItem* pinNode = new cmbNucPartsTreeItem(partsRoot, newpin);
  pinNode->setText(0, newpin->name.c_str());
  Qt::ItemFlags itemFlags(
    Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
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
  case ASSY_LATTICE:
  case ASSY_DUCTCELL:
    objRemoved = false;
    break;
  case ASSY_PINCELL:
    pincell = dynamic_cast<PinCell*>(selObj);
    if(pincell)
      {
      this->Assembly->RemovePinCell(pincell->label);
      }
    delete selItem;
    break;
  case ASSY_FRUSTUM_PIN:
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
  case ASSY_CYLINDER_PIN:
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
  case ASSY_RECT_DUCT:
    this->Assembly->AssyDuct.RemoveDuct(dynamic_cast<Duct*>(selObj));
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
  this->Assembly->AddMaterial(newmat);
  size_t numM = this->Assembly->Materials.size();
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
    this->Assembly->RemoveMaterial(material->name);
    }
  delete selItem;
  emit this->objectRemoved();
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::updateUI()
{
  this->initUI();
  if(!this->Assembly)
    {
    this->setEnabled(0);
    return;
    }
  this->setEnabled(1);

  Qt::ItemFlags itemFlags(
    Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
    // | Qt::ItemIsDropEnabled | Qt::ItemIsEditable);

  this->Internal->PartsList->blockSignals(true);
  this->Internal->MaterialList->blockSignals(true);

  /// ******** populate parts tree ********
  QTreeWidgetItem* partsRoot = this->Internal->PartsList->invisibleRootItem();

  // lattice
  cmbNucPartsTreeItem* latticeNode = new cmbNucPartsTreeItem(partsRoot,
    &this->Assembly->AssyLattice);
  latticeNode->setText(0, "Lattice");
  latticeNode->setFlags(itemFlags); // not editable

  // ducts
  cmbNucPartsTreeItem* ductsNode = new cmbNucPartsTreeItem(partsRoot,
    &this->Assembly->AssyDuct);
  ductsNode->setText(0, "Ducts");
  ductsNode->setFlags(itemFlags); // not editable
  ductsNode->setChildIndicatorPolicy(
    QTreeWidgetItem::DontShowIndicatorWhenChildless);
  for(size_t i = 0; i < this->Assembly->AssyDuct.Ducts.size(); i++)
    {
    Duct *duct = this->Assembly->AssyDuct.Ducts[i];
    cmbNucPartsTreeItem* dNode = new cmbNucPartsTreeItem(ductsNode, duct);
    dNode->setText(0, QString("duct").append(QString::number(i+1)));
    dNode->setFlags(itemFlags); // not editable
    }
  this->Internal->RootDuctsNode = ductsNode;

  // pincells
  for(size_t i = 0; i < this->Assembly->PinCells.size(); i++)
    {
    PinCell *pincell = this->Assembly->PinCells[i];
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

  /// ******** populate materials tree ********
  QTreeWidgetItem* matRoot = this->Internal->MaterialList->invisibleRootItem();
  Qt::ItemFlags matFlags(
    Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);

  for(size_t i = 0; i < this->Assembly->Materials.size(); i++)
    {
    Material *material = this->Assembly->Materials[i];
    cmbNucPartsTreeItem* mNode = new cmbNucPartsTreeItem(matRoot, material);
    mNode->setText(0, material->name.c_str());
    mNode->setFlags(matFlags);
    }

  this->Internal->PartsList->expandAll();
  this->Internal->MaterialList->expandAll();
  this->Internal->PartsList->blockSignals(false);
  this->Internal->MaterialList->blockSignals(false);
  latticeNode->setSelected(true);
  this->onPartsSelectionChanged(); 
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::onPartsSelectionChanged()
{
  cmbNucPartsTreeItem* selItem = this->getSelectedItem(
    this->Internal->PartsList);
  this->updateContextMenu(selItem ? selItem->getPartObject() : NULL);
  this->fireObjectSelectedSignal(selItem);
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::updateContextMenu(AssyPartObj* selObj)
{
  if(!selObj)
    {
    this->Internal->Action_NewPin->setEnabled(true);
    this->Internal->Action_NewDuct->setEnabled(true);
    this->Internal->Action_NewFrustum->setEnabled(false);
    this->Internal->Action_NewCylinder->setEnabled(false);
    this->Internal->Action_DeletePart->setEnabled(false);
    return;
    }
  this->setActionsEnabled(false);
  // create new duct and pincells are always available
  this->Internal->Action_NewPin->setEnabled(true);
  this->Internal->Action_NewDuct->setEnabled(true);

  switch(selObj->GetType())
  {
  case ASSY_LATTICE:
  case ASSY_DUCTCELL:
    break;
  case ASSY_PINCELL:
    this->Internal->Action_NewFrustum->setEnabled(true);
    this->Internal->Action_NewCylinder->setEnabled(true);
    this->Internal->Action_DeletePart->setEnabled(true);
    break;
  case ASSY_FRUSTUM_PIN:
    this->Internal->Action_DeletePart->setEnabled(true);
    break;
  case ASSY_CYLINDER_PIN:
    this->Internal->Action_DeletePart->setEnabled(true);
    break;
  case ASSY_RECT_DUCT:
    // keep at lease one duct
    this->Internal->Action_DeletePart->setEnabled(
      this->Assembly->AssyDuct.Ducts.size()>1 ? true : false);
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
