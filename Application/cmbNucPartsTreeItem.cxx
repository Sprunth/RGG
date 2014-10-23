
#include "cmbNucPartsTreeItem.h"
#include "cmbNucAssembly.h"
#include "cmbNucCore.h"
#include <QFileInfo>
#include <QFont>
#include <QBrush>
#include <QStyle>
#include <QIcon>

//-----------------------------------------------------------------------------
cmbNucPartsTreeItem::cmbNucPartsTreeItem(
  QTreeWidgetItem* pNode, AssyPartObj* obj)
:  QTreeWidgetItem(pNode),
   FileChanged(false), NeedGeneration(false),
   PartObject(obj)
{
  this->connection = new cmbNucPartsTreeItemConnection();
  this->connection->v = this;
  std::string fname = this->PartObject->getFileName();
  if(fname.empty())
  {
    this->setText(3, "");
  }
  else
  {
    QFileInfo fi(fname.c_str());
    this->setText(3, fi.fileName());
  }
}

//-----------------------------------------------------------------------------
cmbNucPartsTreeItem::~cmbNucPartsTreeItem()
{
  delete this->connection;
}
//-----------------------------------------------------------------------------
void cmbNucPartsTreeItem::setText ( int column, const QString & tin )
{
  this->QTreeWidgetItem::setText(column, tin);
  if(column == 0)
    {
    this->PreviousText = tin;
    }
}
//-----------------------------------------------------------------------------
void cmbNucPartsTreeItem::setData (
  int column, int role, const QVariant & value )
{
  if(column == 0 && role == Qt::EditRole) // keep the original text
    {
    this->PreviousText = this->text(column);
    }
  this->QTreeWidgetItem::setData(column, role, value);
}

void cmbNucPartsTreeItemConnection::checkSaveAndGenerate()
{
  v->checkSaveAndGenerate();
}

void cmbNucPartsTreeItem::checkSaveAndGenerate()
{
  enumNucPartsType selType = PartObject->GetType();
  bool need_to_save = false;
  bool need_to_generate = false;
  switch(selType)
    {
    case CMBNUC_CORE:
      {
      cmbNucCore * core = dynamic_cast<cmbNucCore*>(PartObject);
      need_to_save = core->changeSinceLastSave();
      need_to_generate = core->changeSinceLastGenerate();
      }
      break;
    case CMBNUC_ASSEMBLY:
      {
      cmbNucAssembly * assy = dynamic_cast<cmbNucAssembly*>(PartObject);
      need_to_save = assy->changeSinceLastSave();
      need_to_generate = assy->changeSinceLastGenerate();
      }
      break;
    default:
      return;
    }
  this->setHightlights(need_to_save, need_to_generate);
}

bool cmbNucPartsTreeItem::fileChanged() const
{
  return FileChanged;
}

bool cmbNucPartsTreeItem::needGeneration() const
{
  return NeedGeneration;
}

void cmbNucPartsTreeItem::setHightlights(bool fc, bool ng)
{
  FileChanged = fc;
  NeedGeneration = ng;
  QFont f = font(1);
  f.setPixelSize(20);
  this->setFont(1, f);
  f = font(2);
  f.setPixelSize(20);
  this->setFont(2, f);
  this->setTextAlignment(1,Qt::AlignHCenter|Qt::AlignVCenter);
  this->setTextAlignment(2,Qt::AlignHCenter|Qt::AlignVCenter);
  if( FileChanged )
  {
    this->setText( 1, QChar(9999) );
    QBrush b;
    setForeground( 1 , b );
  }
  else
  {
    QBrush b (Qt::green);
    setForeground( 1 , b );
    this->setText( 1, QChar(0x25FC) );
  }

  std::string fname = this->PartObject->getFileName();
  if(fname.empty())
  {
    this->setText(3, "");
  }
  else
  {
    QFileInfo fi(fname.c_str());
    this->setText(3, fi.fileName());
  }

  if(NeedGeneration)
  {
    QBrush b;
    setForeground( 2 , b );
    this->setText( 2, QChar(10007) );
  }
  else
  {
    QBrush b (Qt::green);
    setForeground( 2 , b );
    this->setText( 2, QChar(0x25FC) );
  }
}

QVariant cmbNucPartsTreeItem::data( int index, int role ) const
{
  if(role == Qt::ToolTipRole && index == 3)
  {
    std::string fname = this->PartObject->getFileName();
    if(!fname.empty())
    {
      return QVariant(fname.c_str());
    }
  }
  if(role == Qt::ToolTipRole && index == 1)
  {
    if(FileChanged)
    {
      return QVariant("Needs to be saved");
    }
    else
    {
      return QVariant("Saved");
    }
  }
  if(role == Qt::ToolTipRole && index == 2)
  {
    if(NeedGeneration)
    {
      return QVariant("MeshKit needs to be run");
    }
    else
    {
      return QVariant("MeshKit files up to date");
    }
  }
  return this->QTreeWidgetItem::data(index, role);
}
