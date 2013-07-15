
#include "cmbNucAssemblyEditor.h"

#include "cmbNucDragLabel.h"

#include <QtGui>

cmbNucAssemblyEditor::cmbNucAssemblyEditor(QWidget *parent)
  : QWidget(parent)
{
  setAutoFillBackground(true);
// this->GraphicsView = new QGraphicsView(this);

  this->LatticeView = new QFrame(this);
  this->LatticeView->setFrameShape(QFrame::WinPanel);
  this->LatticeView->setFrameShadow(QFrame::Sunken);
  this->LatticeView->setAcceptDrops(true);
  this->LatticeLayout = new QGridLayout(this->LatticeView);
  this->LatticeView->setLayout(this->LatticeLayout);
  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(this->LatticeView);
  setLayout(layout);
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
  pieceLocations.clear();
  piecePinCells.clear();
  pieceRects.clear();
  highlightedRect = QRect();

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
    this->LatticeLayout = new QGridLayout(this->LatticeView);
    this->LatticeView->setLayout(this->LatticeLayout);
    }
  for(size_t i = 0; i < x; i++)
    {
    for(size_t j = 0; j < y; j++)
      {
      std::string pinlabel = this->Assembly->AssyLattice.Grid[i][j];
      cmbNucDragLabel *wordLabel = new cmbNucDragLabel(
        pinlabel.c_str(), this->LatticeView);
      this->LatticeLayout->addWidget(wordLabel, i, j);
      }
    }
}
void cmbNucAssemblyEditor::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->mimeData()->hasFormat("image/x-puzzle-piece"))
      event->accept();
  else
      event->ignore();
}

void cmbNucAssemblyEditor::dragLeaveEvent(QDragLeaveEvent *event)
{
  QRect updateRect = highlightedRect;
  highlightedRect = QRect();
  update(updateRect);
  event->accept();
}

void cmbNucAssemblyEditor::dragMoveEvent(QDragMoveEvent *event)
{
  QRect updateRect = highlightedRect.unite(targetSquare(event->pos()));

  if (event->mimeData()->hasFormat("image/x-puzzle-piece")
      && findPiece(targetSquare(event->pos())) == -1) {

      highlightedRect = targetSquare(event->pos());
      event->setDropAction(Qt::MoveAction);
      event->accept();
  } else {
      highlightedRect = QRect();
      event->ignore();
  }

  update(updateRect);
}

void cmbNucAssemblyEditor::dropEvent(QDropEvent *event)
{
  if (event->mimeData()->hasFormat("image/x-puzzle-piece")
      && findPiece(targetSquare(event->pos())) == -1) {

      QByteArray pieceData = event->mimeData()->data("image/x-puzzle-piece");
      QDataStream dataStream(&pieceData, QIODevice::ReadOnly);
      QRect square = targetSquare(event->pos());
      QPixmap pixmap;
      QPoint location;
      dataStream >> pixmap >> location;

      pieceLocations.append(location);
      piecePinCells.append(pixmap);
      pieceRects.append(square);

      highlightedRect = QRect();
      update(square);

      event->setDropAction(Qt::MoveAction);
      event->accept();

      if (location == QPoint(square.x()/pieceSize(), square.y()/pieceSize())) {
          //inPlace++;
          emit pinMoved();
      }
  } else {
      highlightedRect = QRect();
      event->ignore();
  }
}

int cmbNucAssemblyEditor::findPiece(const QRect &pieceRect) const
{
  for (int i = 0; i < pieceRects.size(); ++i) {
      if (pieceRect == pieceRects[i]) {
          return i;
      }
  }
  return -1;
}

void cmbNucAssemblyEditor::mousePressEvent(QMouseEvent *event)
{
  QRect square = targetSquare(event->pos());
  int found = findPiece(square);

  if (found == -1)
      return;

  QPoint location = pieceLocations[found];
  QPixmap pixmap = piecePinCells[found];
  pieceLocations.removeAt(found);
  piecePinCells.removeAt(found);
  pieceRects.removeAt(found);

  if (location == QPoint(square.x()/pieceSize(), square.y()/pieceSize()))
      //inPlace--;

  update(square);

  QByteArray itemData;
  QDataStream dataStream(&itemData, QIODevice::WriteOnly);

  dataStream << pixmap << location;

  QMimeData *mimeData = new QMimeData;
  mimeData->setData("image/x-puzzle-piece", itemData);

  QDrag *drag = new QDrag(this);
  drag->setMimeData(mimeData);
  drag->setHotSpot(event->pos() - square.topLeft());
  drag->setPixmap(pixmap);

  if (!(drag->exec(Qt::MoveAction) == Qt::MoveAction)) {
      pieceLocations.insert(found, location);
      piecePinCells.insert(found, pixmap);
      pieceRects.insert(found, square);
      update(targetSquare(event->pos()));

      if (location == QPoint(square.x()/pieceSize(), square.y()/pieceSize()))
          //inPlace++;
          ;
  }
}

void cmbNucAssemblyEditor::paintEvent(QPaintEvent *event)
{
  QPainter painter;
  painter.begin(this);
  painter.fillRect(event->rect(), Qt::white);

  if (highlightedRect.isValid()) {
      painter.setBrush(QColor("#ffcccc"));
      painter.setPen(Qt::NoPen);
      painter.drawRect(highlightedRect.adjusted(0, 0, -1, -1));
  }

  for (int i = 0; i < pieceRects.size(); ++i) {
      painter.drawPixmap(pieceRects[i], piecePinCells[i]);
  }
  painter.end();
}

const QRect cmbNucAssemblyEditor::targetSquare(const QPoint &position) const
{
  return QRect(position.x()/pieceSize() * pieceSize(), position.y()/
    pieceSize() * pieceSize(), pieceSize(), pieceSize());
}

int cmbNucAssemblyEditor::pieceSize() const
{
  return (this->LatticeView->height()*this->LatticeView->width())/
    (this->Assembly->AssyLattice.GetDimensions().first*
    this->Assembly->AssyLattice.GetDimensions().second);
}
