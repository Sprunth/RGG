
#include "cmbNucPartsTreeItem.h"
#include "cmbNucAssembly.h"
#include "cmbNucCore.h"
#include "QFileInfo.h"
#include <QFont>
#include <QBrush>

//-----------------------------------------------------------------------------
cmbNucPartsTreeItem::cmbNucPartsTreeItem(
  QTreeWidgetItem* pNode, AssyPartObj* obj)
:  FileChanged(false), NeedGeneration(false),
   QTreeWidgetItem(pNode), PartObject(obj)
{
  this->connection = new cmbNucPartsTreeItemConnection();
  this->connection->v = this;
  std::string fname = this->PartObject->getFileName();
  if(fname.empty())
  {
    this->setText(1, "");
  }
  else
  {
    QFileInfo fi(fname.c_str());
    this->setText(1, fi.fileName());
  }
}

//-----------------------------------------------------------------------------
cmbNucPartsTreeItem::~cmbNucPartsTreeItem()
{
  delete this->connection;
}
//-----------------------------------------------------------------------------
void cmbNucPartsTreeItem::setText ( int column, const QString & text )
{
  this->QTreeWidgetItem::setText(column, text);
  if(column == 0)
    {
    this->PreviousText = text;
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
  QFont tmp_font = this->font(0);
  tmp_font.setBold(FileChanged||NeedGeneration);
  this->setFont(0, tmp_font);

  if( FileChanged )
  {
    this->setText( 0, ("!* " + PartObject->getLabel() + " *!").c_str() );
  }
  else
  {
    this->setText( 0, (PartObject->getLabel()).c_str() );
  }

  std::string fname = this->PartObject->getFileName();
  if(fname.empty())
  {
    this->setText(1, "");
  }
  else
  {
    QFileInfo fi(fname.c_str());
    this->setText(1, fi.fileName());
  }

  if(NeedGeneration)
  {
    QBrush b (Qt::red);
    setForeground( 0 , b );
  }
  else
  {
    QBrush b;
    setForeground( 0 , b );
  }
}

QVariant cmbNucPartsTreeItem::data( int index, int role ) const
{
  if(role == Qt::ToolTipRole && index == 1)
  {
    std::string fname = this->PartObject->getFileName();
    if(!fname.empty())
    {
      return QVariant(fname.c_str());
    }
  }
  return this->QTreeWidgetItem::data(index, role);
}
