
#include "cmbNucInputListWidget.h"
#include "ui_qInputListWidget.h"

#include "cmbNucAssembly.h"
#include "cmbNucAssemblyLink.h"
#include "cmbNucCore.h"
#include "cmbNucPartsTreeItem.h"
#include "cmbNucMaterialColors.h"
#include "cmbNucDefaults.h"
#include "cmbNucPinLibrary.h"
#include "cmbNucDuctLibrary.h"

#include <QLabel>
#include <QPointer>
#include <QTreeWidget>
#include <QAction>
#include <QBrush>
#include <QFileDialog>
#include <QColorDialog>
#include <QHeaderView>
#include <QItemDelegate>
#include <QMenu>
#include <QHeaderView>
#include <QPainter>
#include <QTreeWidgetItem>

extern bool IS_IN_TESTING_MODE;

class PartsItemDelegate: public QItemDelegate
{
public:
  PartsItemDelegate(QObject* pParent = 0) : QItemDelegate(pParent)
  {
  }

  void paint(QPainter* pPainter, const QStyleOptionViewItem& rOption, const QModelIndex& rIndex) const
  {
    QStyleOptionViewItemV4 ViewOption(rOption);

    QString oldText;

    QColor ItemForegroundColor = rIndex.data(Qt::ForegroundRole).value<QColor>();
    if (ItemForegroundColor.isValid())
    {
      if (ItemForegroundColor != rOption.palette.color(QPalette::WindowText))
      {
        ViewOption.palette.setColor(QPalette::Highlight, ItemForegroundColor);
      }
    }
    QItemDelegate::paint(pPainter, ViewOption, rIndex);
  }

  QSize sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const
  {
    QSize tmp = QItemDelegate::sizeHint(option, index);
    tmp.setHeight(15);
    return tmp;
  }
};

class MaterialItemDelegate: public QItemDelegate
{
public:
  MaterialItemDelegate(QObject* pParent = 0) : QItemDelegate(pParent)
  {
  }

  void paint(QPainter* pPainter, const QStyleOptionViewItem& rOption, const QModelIndex& rIndex) const
  {
    if(rOption.state & QStyle::State_Selected)
    {

      if(rIndex.column() == 3)
      {
        QStyleOptionViewItem unSelect = rOption;
        unSelect.state &= (~QStyle::State_Selected);
        QItemDelegate::paint(pPainter,unSelect,rIndex);
        return;
      }
    }
    QItemDelegate::paint(pPainter,rOption,rIndex);
  }
  QTreeWidget* treeWidget;
};


class cmbNucInputListWidgetInternal :
  public Ui::InputListWidget
{
public:
  cmbNucInputListWidgetInternal()
  {
    previousMeshOrModelTab = -1;
    this->RootCoreNode = NULL;
    TreeItemDelegate = new PartsItemDelegate();
    MaterialDelegate = new MaterialItemDelegate();
  }
  virtual ~cmbNucInputListWidgetInternal()
  {
    delete this->Action_NewAssembly;
    delete this->Action_NewAssemblyLink;
    delete this->Action_NewPin;
    delete this->Action_NewDuct;
    delete this->Action_DeletePart;
    delete this->TreeItemDelegate;
    delete this->MaterialDelegate;
  }
  void initActions()
  {
    this->Action_NewAssembly = new QAction("Create Assembly", this->PartsList);
    this->Action_NewAssemblyLink = new QAction("Create Same As Assembly", this->PartsList);
    this->Action_NewPin = new QAction("Create Pin", this->PartsList);
    this->Action_NewDuct = new QAction("Create Duct", this->PartsList);
    this->Action_DeletePart = new QAction("Delete Selected", this->PartsList);
    this->Action_Clone = new QAction("Clone Selected", this->PartsList);
    this->PartsList->addAction(this->Action_NewAssembly);
    this->PartsList->addAction(this->Action_NewAssemblyLink);
    this->PartsList->addAction(this->Action_NewPin);
    this->PartsList->addAction(this->Action_NewDuct);
    this->PartsList->addAction(this->Action_Clone);
    this->PartsList->addAction(this->Action_DeletePart);
  }

  QPointer<QAction> Action_NewAssembly;
  QPointer<QAction> Action_NewAssemblyLink;
  QPointer<QAction> Action_NewPin;
  QPointer<QAction> Action_NewDuct;
  QPointer<QAction> Action_DeletePart;
  QPointer<QAction> Action_Clone;

  cmbNucPartsTreeItem* RootCoreNode;
  cmbNucPartsTreeItem* AssemblyNode;
  cmbNucPartsTreeItem* PinsNode;
  cmbNucPartsTreeItem* DuctsNode;

  PartsItemDelegate * TreeItemDelegate;
  MaterialItemDelegate * MaterialDelegate;
  int previousMeshOrModelTab; //contains the previous tab except for material
  std::map<std::string, cmbNucPartsTreeItem*> idToNode;
};

//-----------------------------------------------------------------------------
cmbNucInputListWidget::cmbNucInputListWidget(QWidget* _p)
  : QWidget(_p)
{
  this->NuclearCore = NULL;
  this->Internal = new cmbNucInputListWidgetInternal;
  this->Internal->setupUi(this);
  this->Internal->initActions();

  this->modelIsLoaded(false);
  this->Internal->tabInputs->setTabEnabled(2, false);

  this->Internal->PartsList->setAlternatingRowColors(true);
  this->Internal->PartsList->header()->setResizeMode(QHeaderView::ResizeToContents);

  this->setBoundaryLayerControlMode(false);

  // set up the UI trees
  QTreeWidget* treeWidget = this->Internal->PartsList;
  treeWidget->setItemDelegate(this->Internal->TreeItemDelegate);

  this->Internal->MaterialTree->setItemDelegate(this->Internal->MaterialDelegate);
  this->Internal->MaterialDelegate->treeWidget = this->Internal->MaterialTree;

  // context menu for parts tree
  QObject::connect(this->Internal->Action_NewAssembly, SIGNAL(triggered()),
                   this, SLOT(onNewAssembly()));
  QObject::connect(this->Internal->Action_NewAssemblyLink, SIGNAL(triggered()),
                   this, SLOT(onNewAssemblyLink()));
  QObject::connect(this->Internal->Action_NewPin, SIGNAL(triggered()),
                   this, SLOT(onNewPin()));
  QObject::connect(this->Internal->Action_NewDuct, SIGNAL(triggered()),
                   this, SLOT(onNewDuct()));
  QObject::connect(this->Internal->Action_Clone, SIGNAL(triggered()),
                   this, SLOT(onClone()));
  QObject::connect(this->Internal->Action_DeletePart, SIGNAL(triggered()),
                   this, SLOT(onRemoveSelectedPart()));

  QObject::connect(this->Internal->newMaterial, SIGNAL(clicked()),
                   cmbNucMaterialColors::instance(), SLOT(CreateNewMaterial()));
  QObject::connect(this->Internal->delMaterial, SIGNAL(clicked()),
                   cmbNucMaterialColors::instance(), SLOT(deleteSelected()));
  QObject::connect(this->Internal->hideLabels, SIGNAL(clicked(bool)),
                   this,                       SLOT(hideLabels(bool)));
  QObject::connect(this->Internal->importMaterial, SIGNAL(clicked()),
                   this, SLOT(onImportMaterial()));
  QObject::connect(this->Internal->saveMaterial, SIGNAL(clicked()),
                   this, SLOT(onSaveMaterial()));
  QObject::connect(this->Internal->materialDisplayed, SIGNAL(currentIndexChanged(int)),
                   cmbNucMaterialColors::instance(), SLOT(controlShow(int)));

  QObject::connect(this->Internal->PartsList, SIGNAL(itemSelectionChanged()),
                   this, SLOT(onPartsSelectionChanged()), Qt::QueuedConnection);

  QObject::connect(this->Internal->MaterialTree, SIGNAL(itemClicked (QTreeWidgetItem*, int)),
                   this, SLOT(onMaterialClicked(QTreeWidgetItem*, int)), Qt::QueuedConnection);

  QObject::connect(this->Internal->MeshComponents, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem * )),
                   this, SIGNAL(subMeshSelected(QTreeWidgetItem *)), Qt::QueuedConnection);

  QObject::connect(this->Internal->MeshComponents, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
                   this, SIGNAL(meshValueChanged(QTreeWidgetItem*)));

  QObject::connect(this->Internal->tabInputs, SIGNAL(currentChanged(int)),
                   this, SLOT(onTabChanged(int)));

  QObject::connect(this, SIGNAL(deleteAssembly(QTreeWidgetItem*)),
                   this, SLOT(onDeleteAssembly(QTreeWidgetItem*)));

  QObject::connect(this->Internal->color_control, SIGNAL(currentIndexChanged(int)),
                   this, SIGNAL(sendColorControl(int)));
  QObject::connect(this->Internal->edge_control, SIGNAL(clicked(bool)),
                   this, SIGNAL(sendEdgeControl(bool)));

  QObject::connect(this->Internal->drawBoundaryLayer,  SIGNAL(clicked(bool)),
                   this, SIGNAL(drawBoundaryControl(bool)));

  QObject::connect(this->Internal->meshMajorSet, SIGNAL(currentIndexChanged(int)),
                   this, SIGNAL(majorMeshSelection(int)));

  QObject::connect(this->Internal->resetCamera, SIGNAL(clicked()), this, SIGNAL(resetMeshCamera()));

  this->initUI();
}

//-----------------------------------------------------------------------------
cmbNucInputListWidget::~cmbNucInputListWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::setPartOptions(QMenu * qm) const
{
  qm->clear();
  qm->addAction(this->Internal->Action_NewAssembly);
  qm->addAction(this->Internal->Action_NewAssemblyLink);
  qm->addAction(this->Internal->Action_NewPin);
  qm->addAction(this->Internal->Action_NewDuct);
  qm->addAction(this->Internal->Action_Clone);
  qm->addAction(this->Internal->Action_DeletePart);
}

void cmbNucInputListWidget::clear()
{
  this->clearTable();
  this->setCore(NULL);
  this->setEnabled(false);
  this->initPartsTree();
  if(this->Internal->RootCoreNode)
  {
    this->Internal->RootCoreNode = NULL;
    this->Internal->AssemblyNode = NULL;
    this->Internal->PinsNode = NULL;
    this->Internal->DuctsNode = NULL;
  }
  this->modelIsLoaded(false);
  this->Internal->tabInputs->setTabEnabled(2, false);
}

bool cmbNucInputListWidget::onlyMeshLoaded()
{
  return this->isEnabled() && !this->Internal->tabInputs->isTabEnabled(0);
}

void cmbNucInputListWidget::meshIsLoaded(bool v)
{
  if(this->onlyMeshLoaded() || v) this->setEnabled(v);
  if(this->onlyMeshLoaded() || v) this->Internal->tabInputs->setTabEnabled(1, v);
  this->Internal->tabInputs->setTabEnabled(2, v);
  if(v) this->initMaterialsTree();
  if(v && onlyMeshLoaded()) this->Internal->tabInputs->setCurrentIndex(2);
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::setCore(cmbNucCore* core)
{
  if(this->NuclearCore == core)
    {
    return;
    }
  if(this->NuclearCore!=NULL)
  {
    disconnect(this->NuclearCore->GetConnection(), SIGNAL(dataChangedSig()),
               this, SLOT(repaintList()));
  }
  this->NuclearCore = core;
  if(this->NuclearCore!=NULL)
  {
    connect( this->NuclearCore->GetConnection(), SIGNAL(dataChangedSig()),
             this, SLOT(repaintList()) );
  }
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
    case CMBNUC_ASSY_FRUSTUM_PIN:
    case CMBNUC_ASSY_CYLINDER_PIN:
    case CMBNUC_ASSY_DUCT:
      return( NULL );
      break;
    case CMBNUC_ASSEMBLY_LINK:
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
  cmbNucAssembly * assy = NULL;
  if(assyItem && assyItem->getPartObject())
  {
    AssyPartObj* selObj = assyItem->getPartObject();
    if(selObj->GetType() == CMBNUC_ASSEMBLY)
    {
      assy = dynamic_cast<cmbNucAssembly*>(selObj);
    }
    else if(selObj->GetType() == CMBNUC_ASSEMBLY_LINK)
    {
      assy = dynamic_cast<cmbNucAssemblyLink*>(selObj)->getLink();
    }
  }
  return assy;
}

//----------------------------------------------------------------------------
void cmbNucInputListWidget::initUI()
{
  this->setActionsEnabled(false);
  if(this->Internal->RootCoreNode)
  {
    delete this->Internal->RootCoreNode; // Other nodes are children of this one
                                         // so they will be deleted
    this->Internal->RootCoreNode = NULL;
    this->Internal->AssemblyNode = NULL;
    this->Internal->DuctsNode = NULL;
    this->Internal->PinsNode = NULL;
  }
  this->initPartsTree();
  this->initMaterialsTree();
}
//----------------------------------------------------------------------------
void cmbNucInputListWidget::onTabChanged(int currentTab)
{
  if(currentTab == 0)
  {
    if(this->Internal->previousMeshOrModelTab != 0) emit raiseModelDock();
    this->Internal->previousMeshOrModelTab = 0;
  }
  if(currentTab == 1) // materials
  {
    int i = this->Internal->materialDisplayed->currentIndex();
    cmbNucMaterialColors::instance()->controlShow(i);
  }
  if(currentTab == 2)
  {
    if(this->Internal->previousMeshOrModelTab != 2) emit raiseMeshDock();
    this->Internal->previousMeshOrModelTab = 2;
  }
 }
//----------------------------------------------------------------------------
void cmbNucInputListWidget::setActionsEnabled(bool val)
{
  this->Internal->Action_NewAssembly->setEnabled(val);
  this->Internal->Action_NewAssemblyLink->setEnabled(val);
  this->Internal->Action_NewPin->setEnabled(val);
  this->Internal->Action_NewDuct->setEnabled(val);
  this->Internal->Action_Clone->setEnabled(val);
  this->Internal->Action_DeletePart->setEnabled(val);
}
//----------------------------------------------------------------------------
void cmbNucInputListWidget::onNewAssembly()
{
  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  double r,g,b;
  matColorMap->CalcRGB(r,g,b);

  this->setEnabled(1);
  cmbNucAssembly* assembly = new cmbNucAssembly;
  assembly->SetLegendColor(QColor::fromRgbF(r,g,b));
  if(this->NuclearCore->IsHexType())
  {
    assembly->setGeometryLabel("Hexagonal");
    assembly->getLattice().SetDimensions(1, 0, true);
  }
  else
  {
    assembly->setGeometryLabel("Rectangular");
  }
  int count = 1;
  QString label = "Assy_0";
  while(!this->NuclearCore->label_unique(label))
  {
    label = ("Assy_" + QString::number(count++));
  }
  assembly->setLabel(label.toStdString());

  this->NuclearCore->AddAssembly(assembly);
  assembly->computeDefaults();
  assembly->setFromDefaults(this->NuclearCore->GetDefaults());

  double thickX, thickY, h;
  assembly->getDefaults()->getDuctThickness(thickX,thickY);
  assembly->getDefaults()->getHeight(h);

  this->initCoreRootNode();
  this->updateWithAssembly(assembly);
  emit assembliesModified(this->NuclearCore);
}

//----------------------------------------------------------------------------
void cmbNucInputListWidget::onNewAssemblyLink()
{
  cmbNucAssembly * assy = this->getCurrentAssembly();
  if(assy == NULL)
  {
    return;
  }

  cmbNucAssemblyLink * link = new cmbNucAssemblyLink(assy, "0", "0");

  int count = 1;
  QString label = "Assy_0";
  while(!this->NuclearCore->label_unique(label))
  {
    label = ("Assy_" + QString::number(count++));
  }
  link->setLabel(label.toStdString());

  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  double r,g,b;
  matColorMap->CalcRGB(r,g,b);
  link->SetLegendColor(QColor::fromRgbF(r,g,b));

  if(!this->NuclearCore->AddAssemblyLink(link))
  {
    delete link;
    return;
  }
  this->initCoreRootNode();
  this->updateWithAssemblyLink(link);
  this->NuclearCore->GetConnection()->justFileChanged();
  emit assembliesModified(this->NuclearCore);
}

void cmbNucInputListWidget::onNewDuct()
{
  DuctCell * cd = new DuctCell();
  QPointer<cmbNucDefaults> defaults = this->NuclearCore->GetDefaults();
  double d1,d2, h;
  defaults->getDuctThickness(d1,d2);
  defaults->getHeight(h);
  cd->AddDuct(new Duct(h, d1, d2));
  size_t i = this->NuclearCore->getDuctLibrary()->GetNumberOfDuctCells()+1;
  QString ductname = QString("Duct_").append(QString::number(i));
  while(this->NuclearCore->getDuctLibrary()->nameConflicts(ductname.toStdString()))
  {
    ductname = QString("Duct_").append(QString::number(++i));
  }
  cd->setName(ductname.toStdString());
  this->NuclearCore->getDuctLibrary()->addDuct(cd);
  this->initCoreRootNode();
  this->updateWithDuct(cd, true);
  //this->NuclearCore->setAndTestDiffFromFiles(true);
  emit assembliesModified(this->NuclearCore);
  emit checkSavedAndGenerate();
}

//----------------------------------------------------------------------------
void cmbNucInputListWidget::onNewPin()
{
  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  double rgb[3];
  matColorMap->CalcRGB(rgb[0],rgb[1],rgb[2]);

  cmbNucAssembly * assy = this->getCurrentAssembly();
  double h, r = 0.5;
  this->NuclearCore->GetDefaults()->getHeight(h);
  if(assy != NULL) assy->calculateRadius(r);
  PinCell* newpin = new PinCell();
  newpin->SetLegendColor(QColor::fromRgbF(rgb[0],rgb[1],rgb[2]));
  newpin->AddPart(new Cylinder(r, 0, h));
  size_t i = this->NuclearCore->getPinLibrary()->GetNumberOfPinCells()+1;
  QString pinname = QString("PinCell").append(QString::number(i));
  while(this->NuclearCore->getPinLibrary()->labelConflicts(pinname.toStdString()) ||
        this->NuclearCore->getPinLibrary()->nameConflicts(pinname.toStdString()))
  {
    pinname = QString("PinCell").append(QString::number(++i));
  }
  {
    std::string tmp = pinname.toStdString();
    newpin->setName(tmp);
    newpin->setLabel(tmp);
  }
  this->NuclearCore->getPinLibrary()->addPin(&newpin,
                                             cmbNucPinLibrary::KeepOriginal);
  this->initCoreRootNode();
  this->updateWithPin(newpin, true);
  //this->NuclearCore->setAndTestDiffFromFiles(true);
  emit assembliesModified(this->NuclearCore);
  emit checkSavedAndGenerate();
}

//----------------------------------------------------------------------------
void cmbNucInputListWidget::onRemoveSelectedPart()
{
  cmbNucPartsTreeItem* selItem = this->getSelectedItem(this->Internal->PartsList);
  if(!selItem || !selItem->getPartObject())
  {
    return;
  }
  AssyPartObj* selObj = selItem->getPartObject();

  enumNucPartsType selType = selObj->GetType();
  std::string selText = selItem->text(0).toStdString();
  switch(selType)
  {
    case CMBNUC_CORE:
      emit deleteCore();
      break;
    case CMBNUC_ASSEMBLY:
      selItem->setSelected(false);
      emit deleteAssembly(selItem);
      break;
    case CMBNUC_ASSY_PINCELL:
    {
      PinCell* pincell = dynamic_cast<PinCell*>(selObj);
      if(pincell)
      {
        this->Internal->PartsList->blockSignals(true);
        cmbNucAssembly* assem = this->getCurrentAssembly();
        QTreeWidgetItem * p = selItem->parent();
        this->NuclearCore->getPinLibrary()->removePincell(pincell);
        delete selItem;
        this->Internal->PartsList->setCurrentItem(p);
        emit pincellDeleted();
        emit pinsModified(assem);
        this->Internal->PartsList->blockSignals(false);
        this->onPartsSelectionChanged();
      }
      break;
    }
    case CMBNUC_ASSY_DUCTCELL:
    {
      DuctCell* ductcell = dynamic_cast<DuctCell*>(selObj);
      if(ductcell)
      {
        this->setCursor(Qt::BusyCursor);
        this->Internal->PartsList->blockSignals(true);
        this->NuclearCore->getDuctLibrary()->removeDuctcell(ductcell);
        delete selItem;
        this->Internal->PartsList->setCurrentItem(this->Internal->DuctsNode);
        emit assembliesModified(this->NuclearCore);
        this->Internal->PartsList->blockSignals(false);
        this->Internal->DuctsNode->setSelected(true);
        this->onPartsSelectionChanged();
        this->unsetCursor();
      }
      break;
    }
    case CMBNUC_ASSEMBLY_LINK:
      this->onDeleteAssemblyLink(selItem);
      break;
    case CMBNUC_ASSY_DUCT:
    case CMBNUC_ASSY_CYLINDER_PIN:
    case CMBNUC_ASSY_FRUSTUM_PIN:
    default:
      break;
  }
  emit checkSavedAndGenerate();
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

void cmbNucInputListWidget::onDeleteAssemblyLink(QTreeWidgetItem* item)
{
  this->setCursor(Qt::BusyCursor);
  std::string selText = item->text(0).toStdString();
  this->Internal->PartsList->blockSignals(true);
  delete item;
  this->NuclearCore->RemoveAssemblyLink(selText);
  this->Internal->PartsList->setCurrentItem(this->Internal->RootCoreNode);
  emit assembliesModified(this->NuclearCore);
  this->Internal->PartsList->blockSignals(false);
  this->Internal->RootCoreNode->setSelected(true);
  this->onPartsSelectionChanged();
  this->unsetCursor();
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
  QColorDialog::ColorDialogOptions options = QColorDialog::ShowAlphaChannel;
  if(IS_IN_TESTING_MODE)
  {
    options |= QColorDialog::DontUseNativeDialog;
  }
  QColor color = QColorDialog::getColor(bgBrush.color(), this,
                                        "Select Color for Material", options);
  if(color.isValid() && color != bgBrush.color())
  {
    bgBrush.setColor(color);
    item->setBackground(col, bgBrush);
  }
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
  this->updateWithPinLibrary(this->NuclearCore->getPinLibrary());
  this->updateWithDuctLibrary(this->NuclearCore->getDuctLibrary());
  // Assembly nodes
  for(int i=0; i<this->NuclearCore->GetNumberOfAssemblies(); i++)
  {
    this->updateWithAssembly(this->NuclearCore->GetAssembly(i),
      (!selCore && i == (this->NuclearCore->GetNumberOfAssemblies()-1)));
  }
  for(int i=0; i<this->NuclearCore->GetNumberOfAssemblyLinks(); i++)
  {
    this->updateWithAssemblyLink(this->NuclearCore->GetAssemblyLink(i),
                                 (!selCore && i == (this->NuclearCore->GetNumberOfAssemblyLinks()-1)));
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
    this->Internal->RootCoreNode->setHighlights( this->NuclearCore->changeSinceLastSave(),
                                                 this->NuclearCore->changeSinceLastGenerate());
    connect(this, SIGNAL(checkSavedAndGenerate()),
            this->Internal->RootCoreNode->connection, SLOT(checkSaveAndGenerate()));
    this->Internal->RootCoreNode->setText(0, "Core");
    this->Internal->RootCoreNode->setFlags(itemFlags); // not editable
    this->Internal->RootCoreNode->setChildIndicatorPolicy(
      QTreeWidgetItem::DontShowIndicatorWhenChildless);
    this->Internal->RootCoreNode->setExpanded(true);

    this->Internal->PinsNode = new cmbNucPartsTreeItem(this->Internal->RootCoreNode, NULL);
    this->Internal->PinsNode->setText(0, "Pins");
    this->Internal->PinsNode->setFlags(itemFlags); // not editable
    this->Internal->PinsNode->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);

    this->Internal->DuctsNode = new cmbNucPartsTreeItem(this->Internal->RootCoreNode, NULL);
    this->Internal->DuctsNode->setText(0, "Ducts");
    this->Internal->DuctsNode->setFlags(itemFlags); // not editable
    this->Internal->DuctsNode->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);

    this->Internal->AssemblyNode = new cmbNucPartsTreeItem(this->Internal->RootCoreNode, NULL);
    this->Internal->AssemblyNode->setText(0, "Assemblies");
    this->Internal->AssemblyNode->setFlags(itemFlags); // not editable
    this->Internal->AssemblyNode->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
    this->Internal->AssemblyNode->setExpanded(false);
  }
}

void cmbNucInputListWidget::setBoundaryLayerControlMode(bool enabled)
{
  this->Internal->drawBoundaryLayer->setEnabled(enabled);
}

void cmbNucInputListWidget::updateWithAssembly()
{
  QList<QTreeWidgetItem *> tmpl = this->Internal->AssemblyNode->takeChildren();
  for(QList<QTreeWidgetItem *>::iterator i = tmpl.begin(); i != tmpl.end(); ++i)
  {
    delete *i;
  }
  this->Internal->idToNode.clear();
  for(int i=0; i<this->NuclearCore->GetNumberOfAssemblies(); i++)
  {
    this->updateWithAssembly(this->NuclearCore->GetAssembly(i), false);
  }
  this->updateWithAssemblyLink();
}

void cmbNucInputListWidget::updateWithAssemblyLink()
{
  //TODO:
  for(int i=0; i<this->NuclearCore->GetNumberOfAssemblyLinks(); i++)
  {
    this->updateWithAssemblyLink(this->NuclearCore->GetAssemblyLink(i), false);
  }
}

void cmbNucInputListWidget::updateWithAssembly(cmbNucAssembly* assy, bool select)
{
  this->Internal->PartsList->blockSignals(true);
  // Assembly node
  Qt::ItemFlags itemFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  cmbNucPartsTreeItem* assyNode = new cmbNucPartsTreeItem(this->Internal->AssemblyNode,
                                                          assy);
  this->Internal->idToNode[assy->getLabel()] = assyNode;
  connect(this,                      SIGNAL(checkSavedAndGenerate()),
          assyNode->connection,      SLOT(checkSaveAndGenerate()));
  connect(assy->GetConnection(),     SIGNAL(dataChangedSig()),
          assyNode->connection,      SLOT(checkSaveAndGenerate()));
  connect(assy->GetConnection(),     SIGNAL(dataChangedSig()),
          this->Internal->PartsList, SLOT(update()));
  assyNode->setText(0, assy->getLabel().c_str());
  assyNode->setFlags(itemFlags); // not editable
  assyNode->setChildIndicatorPolicy(
    QTreeWidgetItem::DontShowIndicatorWhenChildless);

  assyNode->setHighlights(assy->changeSinceLastGenerate());

  this->Internal->PartsList->blockSignals(false);

  if(select)
  {
    this->Internal->PartsList->setCurrentItem(assyNode);
    this->onPartsSelectionChanged();
  }
}

void cmbNucInputListWidget::updateWithAssemblyLink(cmbNucAssemblyLink* link, bool select)
{
  this->Internal->PartsList->blockSignals(true);
  // Assembly node
  Qt::ItemFlags itemFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  std::map<std::string, cmbNucPartsTreeItem*>::const_iterator node =
        this->Internal->idToNode.find(link->getLink()->getLabel());
  if( node == this->Internal->idToNode.end())
  {
    return;
  }
  cmbNucPartsTreeItem* assyNode = new cmbNucPartsTreeItem(node->second, link);
  std::string label = link->getLabel();
  assyNode->setText(0, label.c_str());
  assyNode->setFlags(itemFlags); // not editable
  assyNode->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  this->Internal->PartsList->blockSignals(false);

  if(select)
  {
    this->Internal->PartsList->setCurrentItem(assyNode);
    this->onPartsSelectionChanged();
  }
}

void cmbNucInputListWidget::updateWithPin(PinCell * pincell, bool select)
{
  this->Internal->PartsList->blockSignals(true);
  Qt::ItemFlags itemFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  cmbNucPartsTreeItem* pinNode = new cmbNucPartsTreeItem(this->Internal->PinsNode, pincell);
  std::string label = pincell->getName() + " (" + pincell->getLabel() + ")";
  pinNode->setText(0, QString::fromStdString(label));
  pinNode->setFlags(itemFlags); // not editable
  pinNode->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  this->Internal->PartsList->blockSignals(false);

  if(select)
  {
    this->Internal->PartsList->setCurrentItem(pinNode);
    this->onPartsSelectionChanged();
  }
}

void cmbNucInputListWidget::updateWithPinLibrary()
{
  this->updateWithPinLibrary(this->NuclearCore->getPinLibrary());
}

void cmbNucInputListWidget::updateWithPinLibrary(cmbNucPinLibrary * pl)
{
  QList<QTreeWidgetItem *> tmpl = this->Internal->PinsNode->takeChildren();
  for(QList<QTreeWidgetItem *>::iterator i = tmpl.begin(); i != tmpl.end(); ++i)
  {
    delete *i;
  }
  for(size_t i = 0; i < pl->GetNumberOfPinCells(); i++)
  {
    this->updateWithPin(pl->GetPinCell(static_cast<int>(i)));
  }
}

void cmbNucInputListWidget::updateWithDuctLibrary()
{
  this->updateWithDuctLibrary(this->NuclearCore->getDuctLibrary());
}

void cmbNucInputListWidget::updateWithDuctLibrary(cmbNucDuctLibrary * dl)
{
  QList<QTreeWidgetItem *> tmpl = this->Internal->DuctsNode->takeChildren();
  for(QList<QTreeWidgetItem *>::iterator i = tmpl.begin(); i != tmpl.end(); ++i)
  {
    delete *i;
  }
  for(size_t i = 0; i < dl->GetNumberOfDuctCells(); i++)
  {
    this->updateWithDuct(dl->GetDuctCell(static_cast<int>(i)));
  }
}

void cmbNucInputListWidget::updateWithDuct(DuctCell * dc, bool select)
{
  this->Internal->PartsList->blockSignals(true);
  Qt::ItemFlags itemFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  cmbNucPartsTreeItem* ductNode = new cmbNucPartsTreeItem(this->Internal->DuctsNode, dc);
  std::string label = dc->getName();
  ductNode->setText(0, QString::fromStdString(label));
  ductNode->setFlags(itemFlags); // not editable
  ductNode->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  this->Internal->PartsList->blockSignals(false);

  if(select)
  {
    this->Internal->PartsList->setCurrentItem(ductNode);
    this->onPartsSelectionChanged();
  }
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::onPartsSelectionChanged()
{
  cmbNucPartsTreeItem* selItem = this->getSelectedItem(this->Internal->PartsList);
  this->updateContextMenu(selItem ? selItem->getPartObject() : NULL,selItem);
  this->fireObjectSelectedSignal(selItem);
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget
::updateContextMenu(AssyPartObj* selObj, const cmbNucPartsTreeItem* selItem)
{
  if(!selObj)
  {
    this->setActionsEnabled(false);

    if(selItem == NULL)
    {
      //DO nothing
    }
    else if(selItem == this->Internal->PinsNode)
    {
      this->Internal->Action_NewPin->setEnabled(true);
    }
    else if(selItem == this->Internal->DuctsNode)
    {
      this->Internal->Action_NewDuct->setEnabled(true);
    }
    else if(selItem == this->Internal->AssemblyNode)
    {
      this->Internal->Action_NewAssembly->setEnabled(true);
    }

    return;
  }
  this->setActionsEnabled(false);
  this->Internal->Action_NewAssembly->setEnabled(true);
  this->Internal->Action_NewPin->setEnabled(true);
  this->Internal->Action_NewDuct->setEnabled(true);
  this->setBoundaryLayerControlMode(false);

  switch(selObj->GetType())
  {
    case CMBNUC_ASSEMBLY:
      this->setBoundaryLayerControlMode(true);
      this->Internal->Action_DeletePart->setEnabled(true);
      this->Internal->Action_Clone->setEnabled(true);
      this->Internal->Action_NewAssemblyLink->setEnabled(true);
      break;
    case CMBNUC_ASSEMBLY_LINK:
      this->setBoundaryLayerControlMode(true);
      this->Internal->Action_DeletePart->setEnabled(true);
      this->Internal->Action_Clone->setEnabled(true);
      break;
    case CMBNUC_CORE:
      this->setBoundaryLayerControlMode(true);
      this->Internal->Action_NewPin->setEnabled(false);
      this->Internal->Action_NewDuct->setEnabled(false);
      this->Internal->Action_DeletePart->setEnabled(true);
      break;
    case CMBNUC_ASSY_PINCELL:
      this->Internal->Action_DeletePart->setEnabled(true);
      this->Internal->Action_Clone->setEnabled(true);
      break;
    case CMBNUC_ASSY_DUCTCELL:
    {
      DuctCell * dc = static_cast<DuctCell *>(selObj);
      this->Internal->Action_DeletePart->setEnabled(!dc->isUsed());
      this->Internal->Action_Clone->setEnabled(true);
      break;
    }
    case CMBNUC_ASSY_FRUSTUM_PIN:
    case CMBNUC_ASSY_CYLINDER_PIN:
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
  treeWidget->setHeaderLabels(QStringList() << tr("Name") << tr("") << tr("") << tr("FileName"));
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
  matColorMap->buildTree(treeWidget);

  treeWidget->blockSignals(false);
}

void cmbNucInputListWidget::assemblyModified(cmbNucPartsTreeItem* assyNode)
{
  cmbNucAssembly* assem = this->getCurrentAssembly();
  if(assem)
  {
    assem->setAndTestDiffFromFiles(true);
    NuclearCore->setAndTestDiffFromFiles(true);
    assyNode->setHighlights(assem->changeSinceLastGenerate());
    Internal->RootCoreNode->setHighlights( NuclearCore->changeSinceLastSave(),
                                           NuclearCore->changeSinceLastGenerate());
    this->Internal->PartsList->repaint();
  }
}

void cmbNucInputListWidget::repaintList()
{
  if(Internal->RootCoreNode != NULL)
    Internal->RootCoreNode->setHighlights( NuclearCore->changeSinceLastSave(),
                                           NuclearCore->changeSinceLastGenerate() );
  this->Internal->PartsList->repaint();
}

void cmbNucInputListWidget::coreModified()
{
  cmbNucPartsTreeItem* selItem = this->getSelectedPartNode();
  if(selItem && NuclearCore)
  {
    NuclearCore->setAndTestDiffFromFiles(true);
    selItem->setHighlights( NuclearCore->changeSinceLastSave(),
                            NuclearCore->changeSinceLastGenerate());
    this->Internal->PartsList->repaint();
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
    case CMBNUC_ASSY_DUCT:
    case CMBNUC_ASSY_BASEOBJ:
      this->assemblyModified(dynamic_cast<cmbNucPartsTreeItem*>(selItem->parent()->parent()));
      break;
    case CMBNUC_ASSEMBLY:
      this->assemblyModified(selItem);
      break;
    case CMBNUC_CORE:
    case CMBNUC_ASSEMBLY_LINK:
      this->coreModified();
      break;
  }
}

void cmbNucInputListWidget::clearTable()
{
  this->Internal->PartsList->clear();
  this->Internal->MeshComponents->clear();
}

void cmbNucInputListWidget::hideLabels(bool v)
{
  this->Internal->MaterialTree->setColumnHidden(2,v);
}

void cmbNucInputListWidget::updateMeshTable(QList<QTreeWidgetItem*> meshParts)
{
  QTreeWidget* treeWidget = this->Internal->MeshComponents;

  treeWidget->blockSignals(true);
  treeWidget->clear();
  treeWidget->setHeaderLabels(QStringList() << tr("") << tr(""));
  treeWidget->setHeaderHidden(true);
  treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
  treeWidget->setAcceptDrops(false);
  treeWidget->addTopLevelItems( meshParts );
  treeWidget->resizeColumnToContents(0);
  meshParts.at(0)->setSelected(true);
  treeWidget->blockSignals(false);
  emit(subMeshSelected(meshParts.at(0)));
}

void cmbNucInputListWidget::updateMainMeshComponents(QStringList parts, int sel)
{
  this->Internal->meshMajorSet->blockSignals(true);
  this->Internal->meshMajorSet->clear();
  this->Internal->meshMajorSet->addItems(parts);
  this->Internal->meshMajorSet->blockSignals(false);
  this->Internal->meshMajorSet->setCurrentIndex ( sel );
}

void cmbNucInputListWidget::modelIsLoaded(bool v)
{
  if(v) this->Internal->tabInputs->setCurrentIndex(0);
  this->Internal->tabInputs->setTabEnabled(0, v);
  this->Internal->tabInputs->setTabEnabled(1, v);
}

void cmbNucInputListWidget::selectMeshTab(bool v)
{
  if(v)
  {
    this->Internal->previousMeshOrModelTab = 2; //used to supress signal
    this->Internal->tabInputs->setCurrentIndex(2);
  }
}

void cmbNucInputListWidget::selectModelTab(bool v)
{
  if(v)
  {
    this->Internal->previousMeshOrModelTab = 0; //used to supress signal
    this->Internal->tabInputs->setCurrentIndex(0);
  }
}

void cmbNucInputListWidget::onClone()
{
  cmbNucPartsTreeItem* selItem = this->getSelectedItem(this->Internal->PartsList);
  if(!selItem || !selItem->getPartObject())
  {
    return;
  }
  AssyPartObj* selObj = selItem->getPartObject();

  enumNucPartsType selType = selObj->GetType();
  std::string selText = selItem->text(0).toStdString();
  switch(selType)
  {
    case CMBNUC_CORE:
      //DO Nothing
      break;
    case CMBNUC_ASSEMBLY:
    {
      //clone assembly
      cmbNucAssembly * assy = dynamic_cast<cmbNucAssembly *>(selObj);
      if(assy)
      {
        cmbNucAssembly * clone_assy = assy->clone(this->NuclearCore->getPinLibrary(),
                                                  this->NuclearCore->getDuctLibrary());
        cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
        double r,g,b;
        matColorMap->CalcRGB(r,g,b);

        clone_assy->SetLegendColor(QColor::fromRgbF(r,g,b));
        std::string name = clone_assy->getLabel();
        unsigned int count = 0;
        while(!this->NuclearCore->label_unique(name))
        {
          name = (QString(clone_assy->getLabel().c_str()) + "_" + QString::number(count++)).toStdString();
        }
        clone_assy->setLabel(name);
        clone_assy->setFileName("");
        this->NuclearCore->AddAssembly(clone_assy);
        this->initCoreRootNode();
        this->updateWithAssembly(clone_assy);
        this->NuclearCore->GetConnection()->justFileChanged();
        emit assembliesModified(this->NuclearCore);
      }
      break;
    }
    case CMBNUC_ASSEMBLY_LINK:
    {
      cmbNucAssemblyLink * original = dynamic_cast<cmbNucAssemblyLink *>(selObj);
      if(original != NULL)
      {
        cmbNucAssemblyLink * link = new cmbNucAssemblyLink(original->getLink(),
                                                           original->getMaterialStartID(),
                                                           original->getNeumannStartID());
        cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
        double r,g,b;
        matColorMap->CalcRGB(r,g,b);

        link->SetLegendColor(QColor::fromRgbF(r,g,b));
        std::string name = original->getLabel();
        unsigned int count = 0;
        while(!this->NuclearCore->label_unique(name))
        {
          name = (QString(original->getLabel().c_str()) + "_" + QString::number(count++)).toStdString();
        }
        link->setLabel(name);
        this->NuclearCore->AddAssemblyLink(link);
        this->initCoreRootNode();
        this->updateWithAssemblyLink(link);
        this->NuclearCore->GetConnection()->justFileChanged();
        emit assembliesModified(this->NuclearCore);
      }
      break;
    }
    case CMBNUC_ASSY_PINCELL:
    {
      PinCell* pincell = dynamic_cast<PinCell *>(selObj);
      if(pincell)
      {
        cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
        double rgb[3];
        matColorMap->CalcRGB(rgb[0],rgb[1],rgb[2]);
        PinCell* clonePin = new PinCell();
        clonePin->fill(pincell);
        clonePin->SetLegendColor(QColor::fromRgbF(rgb[0],rgb[1],rgb[2]));
        std::string name = clonePin->getName();
        std::string label = clonePin->getLabel();
        unsigned int count = 0;
        while(this->NuclearCore->getPinLibrary()->labelConflicts(label))
        {
          label = QString(label.c_str()).append(QString("_") + QString::number(count++)).toStdString();
        }
        count = 0;
        while(this->NuclearCore->getPinLibrary()->nameConflicts(name))
        {
          name = QString(name.c_str()).append("_" + QString::number(count++)).toStdString();
        }
        clonePin->setLabel(label);
        clonePin->setName(name);
        this->NuclearCore->getPinLibrary()->addPin(&clonePin, cmbNucPinLibrary::KeepOriginal);
        this->initCoreRootNode();
        this->updateWithPin(clonePin, true);
        //this->NuclearCore->setAndTestDiffFromFiles(true);
        emit assembliesModified(this->NuclearCore);
      }
      break;
    }
    case CMBNUC_ASSY_DUCTCELL:
    {
      DuctCell* ductcell = dynamic_cast<DuctCell *>(selObj);
      if(ductcell)
      {
        DuctCell* cloneDuct = new DuctCell();
        cloneDuct->fill(ductcell);
        std::string name = cloneDuct->getName();
        unsigned int count = 0;
        count = 0;
        while(this->NuclearCore->getDuctLibrary()->nameConflicts(name))
        {
          name = QString(name.c_str()).append("_" + QString::number(count++)).toStdString();
        }
        cloneDuct->setName(name);
        this->NuclearCore->getDuctLibrary()->addDuct(cloneDuct);
        this->initCoreRootNode();
        this->updateWithDuct(cloneDuct, true);
        //this->NuclearCore->setAndTestDiffFromFiles(true);
        emit assembliesModified(this->NuclearCore);
      }
      break;
    }
    case CMBNUC_ASSY_DUCT:
    case CMBNUC_ASSY_CYLINDER_PIN:
    case CMBNUC_ASSY_FRUSTUM_PIN:
    default:
      break;
  }

  emit checkSavedAndGenerate();
}
