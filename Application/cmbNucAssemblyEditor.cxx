
#include "cmbNucAssemblyEditor.h"

#include "cmbNucDragLabel.h"

#include <QtGui>

cmbNucAssemblyEditor::cmbNucAssemblyEditor(QWidget *parent)
  : QFrame(parent)
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
  this->clear(false);
}

void cmbNucAssemblyEditor::setAssembly(cmbNucAssembly *assembly)
{
  if(this->Assembly == assembly)
    {
    return;
    }
  this->Assembly = assembly;
  this->resetUI();
}
void cmbNucAssemblyEditor::clear(bool updateUI)
{
  for(size_t i = 0; i < this->LatticeLayout->rowCount(); i++)
    {
    for(size_t j = 0; j < this->LatticeLayout->columnCount(); j++)
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
void cmbNucAssemblyEditor::resetUI()
{
  this->clear();
  if(!this->Assembly)
    {
    this->setEnabled(0);
    return;
    }
  this->setEnabled(1);
  int x = this->Assembly->AssyLattice.GetDimensions().first;
  int y = this->Assembly->AssyLattice.GetDimensions().second;
  if(this->LatticeLayout->columnCount()!=y ||
    this->LatticeLayout->rowCount()!=x)
    {
    delete this->LatticeLayout;
    this->LatticeLayout = new QGridLayout(this);
    this->setLayout(this->LatticeLayout);
    }
  for(size_t i = 0; i < x; i++)
    {
    for(size_t j = 0; j < y; j++)
      {
      std::string pinlabel = this->Assembly->AssyLattice.Grid[i][j];
      cmbNucDragLabel *wordLabel = new cmbNucDragLabel(
        pinlabel.c_str(), this);
      this->LatticeLayout->addWidget(wordLabel, i, j);
      }
    }
}

void cmbNucAssemblyEditor::mousePressEvent(QMouseEvent *event)
{
    QLabel *child = static_cast<QLabel*>(childAt(event->pos()));
    if (!child)
        return;

    QMimeData *mimeData = new QMimeData;
    mimeData->setText(child->text());

    QPixmap pixmap(child->size());
    child->render(&pixmap);

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setPixmap(pixmap);

    Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);

    child->setText("xx");

    if (dropAction == Qt::MoveAction)
        child->close();
}

const QRect cmbNucAssemblyEditor::targetRect(const QPoint &position) const
{
  return QRect(position.x()/ pieceWidth(), position.y()/
    pieceHeight(), pieceWidth(), pieceHeight());
}

int cmbNucAssemblyEditor::pieceWidth() const
{
  return this->width()/
    this->Assembly->AssyLattice.GetDimensions().first;
}
int cmbNucAssemblyEditor::pieceHeight() const
{
  return this->height()/
    this->Assembly->AssyLattice.GetDimensions().second;
}
cmbNucDragLabel* cmbNucAssemblyEditor::findLabel(const QRect &pieceRect)
{
  int i = pieceRect.x()/pieceWidth();
  int j = pieceRect.y()/pieceHeight();
  QLayoutItem* item = this->LatticeLayout->itemAtPosition(i, j);
  if(item && item->widget())
    {
    return dynamic_cast<cmbNucDragLabel*>(item->widget());
    }
  return NULL;
}