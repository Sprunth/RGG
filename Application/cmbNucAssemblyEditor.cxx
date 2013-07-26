
#include "cmbNucAssemblyEditor.h"

#include "cmbNucDragLabel.h"

#include <QtGui>

cmbNucAssemblyEditor::cmbNucAssemblyEditor(QWidget *parent)
  : QFrame(parent), CurrentAssembly(NULL)
{
  setAutoFillBackground(true);
// this->GraphicsView = new QGraphicsView(this);
//  this->LatticeView = new QFrame(this);
  //QVBoxLayout *layout = new QVBoxLayout;
  //layout->addWidget(this->LatticeView);
  //setLayout(layout);

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
  const std::vector<std::vector<std::string> >& Grid,
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
      this->CurrentGrid[k][j] =Grid[k][j];
      }
    }

  this->setEnabled(1);
  this->updateLatticeView((int)this->CurrentGrid.size(),
    (int)this->CurrentGrid[0].size());
}

void cmbNucAssemblyEditor::updateLatticeWithGrid(
  std::vector<std::vector<std::string> >& Grid)
{
  int x = (int)this->CurrentGrid.size();
  int y = (int)this->CurrentGrid[0].size();
  Grid.resize(x);
  for(int k = 0; k < x; k++)
    {
    Grid[k].resize(y);
    }
  for(int k = 0; k < x; k++)
    {
    for(int j = 0; j < y; j++)
      {
      Grid[k][j] = this->CurrentGrid[k][j];
      }
    }  
}

void cmbNucAssemblyEditor::updateLatticeView(int x, int y)
{
  if(this->LatticeLayout->columnCount()!=y ||
    this->LatticeLayout->rowCount()!=x)
    {
    delete this->LatticeLayout;
    this->LatticeLayout = new QGridLayout(this);
    this->setLayout(this->LatticeLayout);
    }
  int availableX = (int)this->CurrentGrid.size();
  int availableY = (int)this->CurrentGrid[0].size();
  this->CurrentGrid.resize(x);
  for(int k = 0; k < x; k++)
    {
    this->CurrentGrid[k].resize(y);
    }

  if(x>availableX )
    {
    for(int k = availableX; k < x; k++)
      {
      for(int j = 0; j < y; j++)
        {
        this->CurrentGrid[k][j] ="xx";
        }
      }
    }
  if(y>availableY)
    {
    for(int k = 0; k < x; k++)
      {
      for(int j = availableY; j < y; j++)
        {
        this->CurrentGrid[k][j] ="xx";
        }
      }
    }
  for(int i = 0; i < x; i++)
    {
    for(int j = 0; j < y; j++)
      {
      std::string pinlabel = this->CurrentGrid[i][j];
      cmbNucDragLabel *wordLabel = new cmbNucDragLabel(
        pinlabel.c_str(), this, i, j);
      this->LatticeLayout->addWidget(wordLabel, y-j-1, i);
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
      cmbNucDragLabel* targeLabel = dynamic_cast<cmbNucDragLabel*>(drag->target());
      if(targeLabel != child)
        {
        child->setText("xx");
        this->CurrentGrid[targeLabel->getX()][targeLabel->getY()] =
          targeLabel->text().toStdString();
        this->CurrentGrid[child->getX()][child->getY()] =
          child->text().toStdString();
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
      child->setText(assignAct->text());
      this->CurrentGrid[child->getX()][child->getY()] =
        child->text().toStdString();
      }
    }
  child->setHighlight(false);
  child->update();
}
