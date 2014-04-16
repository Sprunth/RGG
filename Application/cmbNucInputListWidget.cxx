
#include "cmbNucInputListWidget.h"
#include "ui_qInputListWidget.h"

#include "cmbNucAssembly.h"
#include "cmbNucCore.h"
#include "cmbNucPartsTreeItem.h"
#include "cmbNucMaterialColors.h"

#include <QLabel>
#include <QPointer>
#include <QTreeWidget>
#include <QAction>
#include <QBrush>
#include <QFileDialog>
#include <QColorDialog>
#include <QHeaderView>

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
    }

  QPointer<QAction> Action_NewAssembly;
  QPointer<QAction> Action_NewPin;
  //QPointer<QAction> Action_NewFrustum;
  //QPointer<QAction> Action_NewCylinder;
  QPointer<QAction> Action_NewDuct;
  QPointer<QAction> Action_DeletePart;

  cmbNucPartsTreeItem* RootCoreNode;
};

//-----------------------------------------------------------------------------
cmbNucInputListWidget::cmbNucInputListWidget(QWidget* _p)
  : QWidget(_p)
{
  this->Internal = new cmbNucInputListWidgetInternal;
  this->Internal->setupUi(this);
  this->Internal->initActions();

  this->Internal->PartsList->setAlternatingRowColors(true);

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

  QObject::connect(this->Internal->newMaterial, SIGNAL(clicked()),
    this, SLOT(onNewMaterial()));
  QObject::connect(this->Internal->delMaterial, SIGNAL(clicked()),
    this, SLOT(onRemoveMaterial()));
  QObject::connect(this->Internal->importMaterial, SIGNAL(clicked()),
    this, SLOT(onImportMaterial()));
  QObject::connect(this->Internal->saveMaterial, SIGNAL(clicked()),
    this, SLOT(onSaveMaterial()));

  QObject::connect(this->Internal->PartsList, SIGNAL(itemSelectionChanged()),
    this, SLOT(onPartsSelectionChanged()), Qt::QueuedConnection);


  QObject::connect(this->Internal->MaterialTree, SIGNAL(itemSelectionChanged()),
    this, SLOT(onMaterialSelectionChanged()), Qt::QueuedConnection);
  QObject::connect(this->Internal->MaterialTree, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
    this, SLOT(onMaterialChanged(QTreeWidgetItem*, int)));
  QObject::connect(this->Internal->MaterialTree, SIGNAL(itemClicked (QTreeWidgetItem*, int)),
    this, SLOT(onMaterialClicked(QTreeWidgetItem*, int)), Qt::QueuedConnection);

  QObject::connect(this->Internal->tabInputs, SIGNAL(currentChanged(int)),
    this, SLOT(onTabChanged(int)));

  QObject::connect(this, SIGNAL(deleteAssembly(QTreeWidgetItem*)),
                   this, SLOT(onDeleteAssembly(QTreeWidgetItem*)));

  QObject::connect(this->Internal->ColorMesh, SIGNAL(toggled(bool)),
                   this, SIGNAL(meshColorChange(bool)));
  QObject::connect(this->Internal->ShowEdges, SIGNAL(toggled(bool)),
                   this, SIGNAL(meshEdgeChange(bool)));

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
QTreeWidget * cmbNucInputListWidget::getModelTree()
{
  return this->Internal->ModelTree;
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::setMeshColorState(bool b)
{
  this->Internal->ColorMesh->setChecked(b);
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::setMeshEdgeState(bool b)
{
  this->Internal->ShowEdges->setChecked(b);
}

//-----------------------------------------------------------------------------
bool cmbNucInputListWidget::getMeshColorState()
{
  return this->Internal->ColorMesh->isChecked();
}

//-----------------------------------------------------------------------------
bool cmbNucInputListWidget::getMeshEdgeState()
{
  return this->Internal->ShowEdges->isChecked();
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
AssyPartObj* cmbNucInputListWidget::getSelectedPart()
{
  cmbNucPartsTreeItem* selItem = this->getSelectedItem(
    this->Internal->PartsList);
  if(!selItem || !selItem->getPartObject())
    {
    return NULL;
    }
  return selItem->getPartObject();
}
//-----------------------------------------------------------------------------
cmbNucPartsTreeItem* cmbNucInputListWidget::getSelectedPartNode()
{
  return this->getSelectedItem(this->Internal->PartsList);
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
    case CMBNUC_ASSY_HEX_DUCT:
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
  this->initPartsTree();
  this->initMaterialsTree();
}
//----------------------------------------------------------------------------
void cmbNucInputListWidget::onTabChanged(int currentTab)
{
  if(currentTab == 0) // parts
    {
    this->onPartsSelectionChanged();
    emit switchToNonModelTab(0);
    }
  else if(currentTab == 1) // materials
    {
    this->onMaterialSelectionChanged();
    emit switchToNonModelTab(1);
    }
  else if(currentTab) //model
    {
    emit(switchToModelTab());
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
  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();

  this->setEnabled(1);
  cmbNucAssembly* assembly = new cmbNucAssembly;
  assembly->AssyLattice.SetGeometryType(
    this->NuclearCore->CoreLattice.GetGeometryType());
  if(this->NuclearCore->CoreLattice.GetGeometryType() == HEXAGONAL)
    {
    assembly->AssyLattice.SetDimensions(1, 0, true);
    assembly->GeometryType = "Hexagonal";
    }
  else
    {
    assembly->GeometryType = "Rectangular";
    }
  assembly->label = QString("Assy").append(
    QString::number(this->NuclearCore->GetNumberOfAssemblies()+1)).toStdString();

  this->NuclearCore->AddAssembly(assembly);

  this->initCoreRootNode();
  this->updateWithAssembly(assembly);
  emit assembliesModified(this->NuclearCore);
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
  emit pinsModified(this->getCurrentAssembly());
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
  cmbNucPartsTreeItem* pItem = NULL;
  PinCell* pincell = NULL;

  enumNucPartsType selType = selObj->GetType();
  std::string selText = selItem->text(0).toStdString();
  switch(selType)
  {
  case CMBNUC_CORE:
    emit deleteCore();
    objRemoved = false;
    break;
  case CMBNUC_ASSEMBLY:
    selItem->setSelected(false);
    emit deleteAssembly(selItem);
    break;
  case CMBNUC_ASSY_PINCELL:
    pincell = dynamic_cast<PinCell*>(selObj);
    if(pincell)
      {
      cmbNucAssembly* assem = this->getCurrentAssembly();
      selItem->setSelected(false);
      assem->RemovePinCell(pincell->label);
      emit pinsModified(assem);
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
  emit checkSavedAndGenerate();
  if(objRemoved)
    {
    emit this->objectRemoved();
    }
}

void cmbNucInputListWidget::onDeleteAssembly(QTreeWidgetItem* item)
{
  this->setCursor(Qt::BusyCursor);
  std::string selText = item->text(0).toStdString();
  this->Internal->PartsList->blockSignals(true);
  delete item;
  this->NuclearCore->RemoveAssembly(selText);
  this->Internal->PartsList->setCurrentItem(this->Internal->RootCoreNode);
  emit assembliesModified(this->NuclearCore);
  this->Internal->PartsList->blockSignals(false);
  this->Internal->RootCoreNode->setSelected(true);
  this->onPartsSelectionChanged();
  this->unsetCursor();
}

//----------------------------------------------------------------------------
void cmbNucInputListWidget::onNewMaterial()
{
  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  size_t numM = matColorMap->MaterialColorMap().size();
  QString matname = QString("material").append(
    QString::number(numM+1));

  this->createMaterialItem(matname, matname, QColor::fromRgbF(1.0, 1.0, 1.0));
}

//----------------------------------------------------------------------------
void cmbNucInputListWidget::createMaterialItem(
  const QString& name, const QString& label, const QColor& color)
{
  QTreeWidgetItem* matRoot = this->Internal->MaterialTree->invisibleRootItem();
  Qt::ItemFlags matFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable |
                         Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
  cmbNucPartsTreeItem* mNode = new cmbNucPartsTreeItem(matRoot, NULL);
  mNode->setText(1, name);
  mNode->setText(2, label);
  QBrush bgBrush(color);
  mNode->setBackground(3, bgBrush);
  mNode->setFlags(matFlags);
  mNode->setCheckState(0, Qt::Checked);

  cmbNucPartsTreeItem* selItem = this->getSelectedItem(
    this->Internal->MaterialTree);
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
    this->Internal->MaterialTree);
  if(!selItem)
    {
    return;
    }
  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  matColorMap->RemoveMaterial(selItem->text(1));
  this->getCurrentAssembly()->RemoveMaterial(selItem->text(1).toStdString());
  delete selItem;
}
//----------------------------------------------------------------------------
void cmbNucInputListWidget::onImportMaterial()
{
  QStringList fileNames =
    QFileDialog::getOpenFileNames(this,
                                 "Open Material File...",
                                 QDir::homePath(),
                                 "INI Files (*.ini);;All Files (*.*)");
  if(fileNames.count() == 0)
    {
    return;
    }
  this->setCursor(Qt::BusyCursor);
  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  foreach(QString fileName, fileNames)
    {
    if(!fileName.isEmpty())
      {
      if(matColorMap->OpenFile(fileName))
        {
        this->initMaterialsTree();// reload the material tree.
        }
      }
    }
  this->unsetCursor();
}
//----------------------------------------------------------------------------
void cmbNucInputListWidget::onSaveMaterial()
{
  QString fileName =
    QFileDialog::getSaveFileName(this,
                                 "Save Material File...",
                                 QDir::homePath(),
                                 "INI Files (*.ini)");
  if(!fileName.isEmpty())
    {
    this->setCursor(Qt::BusyCursor);
    cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
    matColorMap->SaveToFile(fileName);
    this->unsetCursor();
    }
}
//----------------------------------------------------------------------------
void cmbNucInputListWidget::onMaterialClicked(QTreeWidgetItem* item, int col)
{
  if(col != 3)
    {
    return;
    }

  QBrush bgBrush = item->background(col);
  QColor color = QColorDialog::getColor(bgBrush.color(), this,
                                        "Select Color for Material",
                                        QColorDialog::ShowAlphaChannel);
  if(color.isValid() && color != bgBrush.color())
    {
    bgBrush.setColor(color);
    item->setBackground(col, bgBrush);
    cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
    matColorMap->AddMaterial(item->text(1), item->text(2), color);
    emit this->materialColorChanged(item->text(1));
    }
  //if(!item->isSelected())
  //  {
  //  this->Internal->MaterialTree->setCurrentItem(item);
  //  item->setSelected(true);
  //  }
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
    this->Internal->RootCoreNode->setHightlights( this->NuclearCore->changeSinceLastSave(),
                                                  this->NuclearCore->changeSinceLastGenerate());
    connect(this, SIGNAL(checkSavedAndGenerate()),
            this->Internal->RootCoreNode->connection, SLOT(checkSaveAndGenerate()));
    this->Internal->RootCoreNode->setText(0, "Core");
    this->Internal->RootCoreNode->setFlags(itemFlags); // not editable
    this->Internal->RootCoreNode->setChildIndicatorPolicy(
      QTreeWidgetItem::DontShowIndicatorWhenChildless);
    this->Internal->RootCoreNode->setExpanded(true);
    }
}

void cmbNucInputListWidget::updateWithAssembly(cmbNucAssembly* assy, bool select)
{
  this->Internal->PartsList->blockSignals(true);
  // Assembly node
  Qt::ItemFlags itemFlags(
    Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  cmbNucPartsTreeItem* assyNode = new cmbNucPartsTreeItem(
    this->Internal->RootCoreNode, assy);
  connect(this, SIGNAL(checkSavedAndGenerate()),
          assyNode->connection, SLOT(checkSaveAndGenerate()));
  assyNode->setText(0, assy->label.c_str());
  assyNode->setFlags(itemFlags); // not editable
  assyNode->setChildIndicatorPolicy(
    QTreeWidgetItem::DontShowIndicatorWhenChildless);

  assyNode->setHightlights(assy->changeSinceLastSave(),
                           assy->changeSinceLastGenerate());

  /// ******** populate parts tree ********
  QTreeWidgetItem* partsRoot = assyNode;

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

    /// don't need this anymore, since the PinCellEditor is integrated into
    /// Main UI panels

/*
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
*/
    }

  this->Internal->PartsList->blockSignals(false);

  if(select)
    {
    this->Internal->PartsList->setCurrentItem(partsRoot);
    //partsRoot->setSelected(true); // select the assembly
    this->onPartsSelectionChanged();
    }
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
    this->setActionsEnabled(false);
    return;
    }
  this->setActionsEnabled(false);
  this->Internal->Action_NewAssembly->setEnabled(true);
  this->Internal->Action_NewDuct->setEnabled(true);
  this->Internal->Action_NewPin->setEnabled(true);

  switch(selObj->GetType())
  {
  case CMBNUC_ASSEMBLY:
    this->Internal->Action_DeletePart->setEnabled(true);
    break;
  case CMBNUC_CORE:
    this->Internal->Action_NewDuct->setEnabled(false);
    this->Internal->Action_NewPin->setEnabled(false);
    this->Internal->Action_DeletePart->setEnabled(true);
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
void cmbNucInputListWidget::labelChanged(QString newl)
{
  cmbNucPartsTreeItem* selItem =
    this->getSelectedItem(this->Internal->PartsList);
  if(selItem)
  {
    selItem->setText(0,newl);
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
  //cmbNucPartsTreeItem* selItem = this->getSelectedItem(
  //  this->Internal->MaterialTree);
  //this->fireObjectSelectedSignal(selItem);
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::onMaterialChanged(
 QTreeWidgetItem* item, int col)
{
  cmbNucPartsTreeItem* selItem = dynamic_cast<cmbNucPartsTreeItem*>(item);
  if(!selItem || (col != 0 && col != 1 && col != 2))
    {
    return;
    }
  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  QString prematerial, material, label;
  if(col == 0)
    {
    material = selItem->text(1);
    matColorMap->SetMaterialVisibility(material, selItem->checkState(0));
    emit this->materialVisibilityChanged(material);
    return;
    }
  else if(col == 1)
    {
    prematerial = selItem->previousText();
    material = selItem->text(1);
    label = selItem->text(2);
    }
  else// if(col == 2)
    {
    prematerial = material = selItem->text(1);
    label = selItem->text(2);
    }

  if(matColorMap->MaterialColorMap().contains(prematerial))
    {
    if(col == 1)
      {
      matColorMap->RemoveMaterial(prematerial);
      }
    matColorMap->AddMaterial(material, label,
      matColorMap->MaterialColorMap()[material].Color);
    }

  emit this->materialColorChanged(prematerial);
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
void cmbNucInputListWidget::initPartsTree()
{
  QTreeWidget* treeWidget = this->Internal->PartsList;

  treeWidget->blockSignals(true);
  treeWidget->clear();
  treeWidget->setHeaderLabel("Name");
  treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
  treeWidget->setAcceptDrops(false);
  treeWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
  treeWidget->blockSignals(false);
}

//----------------------------------------------------------------------------
void cmbNucInputListWidget::initMaterialsTree()
{
  QTreeWidget* treeWidget = this->Internal->MaterialTree;

  treeWidget->blockSignals(true);
  treeWidget->clear();
  treeWidget->setHeaderLabels(
    QStringList() << tr("Show") << tr("Name") << tr("Label") << tr("Color") );
  treeWidget->setColumnCount(4);
  treeWidget->setAlternatingRowColors(true);
  treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
  treeWidget->header()->setResizeMode(0, QHeaderView::ResizeToContents);
  treeWidget->header()->setResizeMode(1, QHeaderView::ResizeToContents);
  treeWidget->header()->setResizeMode(2, QHeaderView::ResizeToContents);
  treeWidget->setAcceptDrops(false);
  treeWidget->setContextMenuPolicy(Qt::NoContextMenu);

  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  foreach(QString material, matColorMap->MaterialColorMap().keys())
    {
    this->createMaterialItem(material,
      matColorMap->MaterialColorMap()[material].Label,
      matColorMap->MaterialColorMap()[material].Color);
    }

  treeWidget->blockSignals(false);
}

void cmbNucInputListWidget::assemblyModified(cmbNucPartsTreeItem* assyNode)
{
  cmbNucAssembly* assem = this->getCurrentAssembly();
  if(assem)
  {
    assem->setAndTestDiffFromFiles(true);
    assyNode->setHightlights(assem->changeSinceLastSave(),
                             assem->changeSinceLastGenerate());
  }
}

void cmbNucInputListWidget::coreModified()
{
  cmbNucPartsTreeItem* selItem = this->getSelectedPartNode();
  if(selItem && NuclearCore)
  {
    NuclearCore->setAndTestDiffFromFiles(true);
    selItem->setHightlights(NuclearCore->changeSinceLastSave(),
                            NuclearCore->changeSinceLastGenerate());
  }
}

void cmbNucInputListWidget::valueChanged()
{
  AssyPartObj* part = this->getSelectedPart();
  cmbNucPartsTreeItem* selItem = this->getSelectedPartNode();
  if(part == NULL || selItem == NULL)
  {
    return;
  }
  switch(part->GetType())
  {
    case CMBNUC_ASSY_LATTICE:
    case CMBNUC_ASSY_DUCTCELL:
    case CMBNUC_ASSY_PINCELL:
      this->assemblyModified(dynamic_cast<cmbNucPartsTreeItem*>(selItem->parent()));
      break;
    case CMBNUC_ASSY_FRUSTUM_PIN:
    case CMBNUC_ASSY_CYLINDER_PIN:
    case CMBNUC_ASSY_RECT_DUCT:
    case CMBNUC_ASSY_HEX_DUCT:
    case CMBNUC_ASSY_BASEOBJ:
      this->assemblyModified(dynamic_cast<cmbNucPartsTreeItem*>(selItem->parent()->parent()));
      break;
    case CMBNUC_ASSEMBLY:
      this->assemblyModified(selItem);
      break;
    case CMBNUC_CORE:
      this->coreModified();
      break;
  }
}

AssyPartObj* cmbNucInputListWidget::getSelectedCoreOrAssembly()
{
  AssyPartObj* part = this->getSelectedPart();
  cmbNucPartsTreeItem* selItem = this->getSelectedPartNode();
  if(part == NULL || selItem == NULL)
  {
    return NULL;
  }
  switch(part->GetType())
  {
    case CMBNUC_ASSY_LATTICE:
    case CMBNUC_ASSY_DUCTCELL:
    case CMBNUC_ASSY_PINCELL:
      return dynamic_cast<cmbNucPartsTreeItem*>(selItem->parent())->getPartObject();
      break;
    case CMBNUC_ASSY_FRUSTUM_PIN:
    case CMBNUC_ASSY_CYLINDER_PIN:
    case CMBNUC_ASSY_RECT_DUCT:
    case CMBNUC_ASSY_HEX_DUCT:
    case CMBNUC_ASSY_BASEOBJ:
      return dynamic_cast<cmbNucPartsTreeItem*>(selItem->parent()->parent())->getPartObject();
      break;
    case CMBNUC_ASSEMBLY:
    case CMBNUC_CORE:
      return part;
      break;
  }
  return NULL;
}

void cmbNucInputListWidget::clearTable()
{
  this->Internal->PartsList->clear();
}
