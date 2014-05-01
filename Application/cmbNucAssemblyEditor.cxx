
#include "cmbNucAssemblyEditor.h"

#include "cmbNucDragLabel.h"
#include "cmbNucAssembly.h"
#include "cmbNucCore.h"
#include <iostream>

#include <QtGui>
#include <QDebug>

cmbNucAssemblyEditor::cmbNucAssemblyEditor(QWidget *parent, cmbNucAssembly* assy)
  : QFrame(parent), CurrentAssembly(assy), CurrentCore(NULL)
{
  setAutoFillBackground(true);

  this->setFrameShape(QFrame::WinPanel);
  this->setFrameShadow(QFrame::Sunken);
  this->setAcceptDrops(true);
  this->LatticeLayout = new QGridLayout(this);
  this->setLayout(this->LatticeLayout);
}

cmbNucAssemblyEditor::~cmbNucAssemblyEditor()
{
  this->clearUI(false);
}

void cmbNucAssemblyEditor::setAssembly(cmbNucAssembly* assy)
{
  this->CurrentAssembly = assy;
  this->CurrentCore = NULL;
}

void cmbNucAssemblyEditor::setCore(cmbNucCore* core)
{
  this->CurrentAssembly = NULL;
  this->CurrentCore = core;
}

void cmbNucAssemblyEditor::clearUI(bool updateUI)
{
  for(int i = 0; i < this->LatticeLayout->rowCount(); i++)
    {
    for(int j = 0; j < this->LatticeLayout->columnCount(); j++)
      {
      QLayoutItem* item = this->LatticeLayout->itemAtPosition(i, j);
      if(item && item->widget())
        {
        delete item->widget();
        this->LatticeLayout->removeItem(item);
        }
      }
    }
  if(updateUI)
    {
    update();
    }
}
void cmbNucAssemblyEditor::resetUI(
  const std::vector<std::vector<LatticeCell> >& Grid,
  QStringList& actions)
{
  this->clearUI();
  if(Grid.size()==0 || Grid[0].size()==0)
    {
    this->setEnabled(0);
    return;
    }
  this->ActionList = actions;

  this->CurrentGrid.resize(Grid.size());
  for(int k = 0; k < Grid.size(); k++)
    {
    this->CurrentGrid[k].resize(Grid[0].size());
    }

  for(size_t k = 0; k < Grid.size(); k++)
    {
    for(size_t j = 0; j < Grid[0].size(); j++)
      {
      this->CurrentGrid[k][j] = Grid[k][j];
      }
    }

  this->setEnabled(1);
  this->updateLatticeView((int)this->CurrentGrid.size(),
    (int)this->CurrentGrid[0].size());
}

bool cmbNucAssemblyEditor::updateLatticeWithGrid(
  std::vector<std::vector<LatticeCell> >& Grid)
{
  int x = (int)this->CurrentGrid.size();
  int y = (int)this->CurrentGrid[0].size();
  bool change = x != Grid.size();
  Grid.resize(x);
  for(int k = 0; k < x; k++)
    {
    change |= y != Grid[k].size();
    Grid[k].resize(y);
    }
  for(int k = 0; k < x; k++)
    {
    for(int j = 0; j < y; j++)
      {
      change |= this->CurrentGrid[k][j].label != Grid[k][j].label;
      Grid[k][j] = this->CurrentGrid[k][j];
      }
    }
  return change;
}

void cmbNucAssemblyEditor::updateLatticeView(int x, int y)
{
  if(this->LatticeLayout->columnCount()!=y ||
    this->LatticeLayout->rowCount()!=x)
    {
    delete this->LatticeLayout;
    this->LatticeLayout = new QGridLayout(this);
    this->LatticeLayout->setSpacing(2);
    this->LatticeLayout->setContentsMargins(1, 1, 1, 1);
    this->LatticeLayout->setOriginCorner(Qt::BottomLeftCorner);
    this->setLayout(this->LatticeLayout);
    }
  int availableX = (int)this->CurrentGrid.size();
  int availableY = (int)this->CurrentGrid[0].size();
  this->CurrentGrid.resize(x);
  for(int k = 0; k < x; k++)
    {
    this->CurrentGrid[k].resize(y);
    }

  if(x>availableX)
    {
    for(int k = availableX; k < x; k++)
      {
      for(int j = 0; j < y; j++)
        {
        this->CurrentGrid[k][j].label = "xx";
        this->CurrentGrid[k][j].color = Qt::white;
        }
      }
    }
  if(y>availableY)
    {
    for(int k = 0; k < x; k++)
      {
      for(int j = availableY; j < y; j++)
        {
        this->CurrentGrid[k][j].label = "xx";
        this->CurrentGrid[k][j].color = Qt::white;
        }
      }
    }

  for(int i = 0; i < x; i++)
    {
    for(int j = 0; j < y; j++)
      {
      LatticeCell lc = this->CurrentGrid[i][j];
      cmbNucDragLabel *wordLabel = new cmbNucDragLabel(
        lc.label.c_str(), this, i, j);
      wordLabel->setBackgroundColor(lc.color);
      this->LatticeLayout->addWidget(wordLabel, i, j);
      }
    }
  update();
}

void cmbNucAssemblyEditor::mousePressEvent(QMouseEvent *event)
{
  cmbNucDragLabel *child = static_cast<cmbNucDragLabel*>(childAt(event->pos()));
  if (!child)
    return;
  child->setHighlight(true);
  child->update();

  if(event->button() == Qt::LeftButton)
    { // drag
    QMimeData *mimeData = new QMimeData;
    mimeData->setText(child->text());

    QPixmap pixmap(child->size());
    child->render(&pixmap);

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setPixmap(pixmap);

    Qt::DropAction dropAction = drag->exec(Qt::CopyAction);
    if(dropAction != Qt::IgnoreAction)
      {
      cmbNucDragLabel* destCell = dynamic_cast<cmbNucDragLabel*>(drag->target());
      if(destCell != child)
        {
        this->CurrentGrid[destCell->getX()][destCell->getY()].label =
          child->text().toStdString();
        this->CurrentGrid[destCell->getX()][destCell->getY()].color =
          child->getBackgroundColor();
        destCell->setBackgroundColor(child->getBackgroundColor());
        destCell->setHighlight(false);
        destCell->update();
        }
      }
    }
  else if(event->button() == Qt::RightButton)
    { // context menu
    QMenu contextMenu(this);
    QAction* pAction = NULL;
    // available parts
    foreach(QString strAct, this->ActionList)
      {
      pAction = new QAction(strAct, this);
      contextMenu.addAction(pAction);
      }

    QAction* assignAct = contextMenu.exec(event->globalPos());
    if(assignAct)
      {
      std::string label = assignAct->text().toStdString();
      child->setText(assignAct->text());

      this->CurrentGrid[child->getX()][child->getY()].label = label;

      if(this->CurrentAssembly)
        {
        PinCell* pc = this->CurrentAssembly->GetPinCell(label);
        QColor color = pc ? pc->GetLegendColor() : Qt::white;

        this->CurrentGrid[child->getX()][child->getY()].color = color;
        child->setBackgroundColor(color);
        }
      else if(this->CurrentCore)
        {
        cmbNucAssembly* assy = this->CurrentCore->GetAssembly(label);
        QColor color = assy ? assy->GetLegendColor() : Qt::white;

        this->CurrentGrid[child->getX()][child->getY()].color = color;
        child->setBackgroundColor(color);
        }
      }
    }
  child->setHighlight(false);
  child->update();
}
