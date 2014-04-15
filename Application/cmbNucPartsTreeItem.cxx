
#include "cmbNucPartsTreeItem.h"
#include "cmbNucAssembly.h"
#include "cmbNucCore.h"
#include <QFont>

//-----------------------------------------------------------------------------
cmbNucPartsTreeItem::cmbNucPartsTreeItem(
  QTreeWidgetItem* pNode, AssyPartObj* obj)
: QTreeWidgetItem(pNode), PartObject(obj)
{
  this->connection = new cmbNucPartsTreeItemConnection();
  this->connection->v = this;
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

void cmbNucPartsTreeItem::setHightlights(bool fileChange, bool needGeneration)
{
  QFont tmp_font = this->font(0);
  tmp_font.setBold(fileChange);
  tmp_font.setUnderline(fileChange);
  this->setFont(0, tmp_font);
}
