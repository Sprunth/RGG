#include "cmbNucHexLattice.h"

#include <QMessageBox>
#include <QApplication>
#include <QPainter>
#include <QMouseEvent>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <QGraphicsItem>
#include <QMenu>
#include <QAction>
#include <QStyleOptionGraphicsItem>

#include "vtkMath.h"
#include "cmbNucAssembly.h"

cmbNucHexLattice::cmbNucHexLattice(HexLatticeItem::ShapeStyle shape,
    QWidget* parent, Qt::WindowFlags f)
      : QGraphicsView(parent), ItemShape(shape), CurrentAssembly(NULL)
{
  setScene(&this->Canvas);
  setInteractive(true);
  setResizeAnchor(QGraphicsView::AnchorViewCenter);
  setWindowFlags(f);
  setAcceptDrops(true);
  setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

  this->HexGrid.SetNumberOfLayers(1);
  init();
}

cmbNucHexLattice::~cmbNucHexLattice()
{
}

void cmbNucHexLattice::clear()
{
  scene()->clear();
}

void cmbNucHexLattice::init()
{
  this->ActionList << "xx";
  this->rebuild();
}

void cmbNucHexLattice::resetWithGrid(std::vector<std::vector<LatticeCell> >& inGrid)
{
  this->copyGrid(inGrid, this->HexGrid.Grid);
  this->rebuild();
}

void cmbNucHexLattice::applyToGrid(std::vector<std::vector<LatticeCell> >& outGrid)
{
  this->copyGrid(this->HexGrid.Grid, outGrid);
}

void cmbNucHexLattice::copyGrid(std::vector<std::vector<LatticeCell> >& inGrid,
  std::vector<std::vector<LatticeCell> >& outGrid)
{
  outGrid.resize(inGrid.size());
  for(size_t i = 0; i < inGrid.size(); i++)
    {
    outGrid[i].resize(inGrid[i].size());
    for(size_t j = 0; j < inGrid[i].size(); j++)
      {
      outGrid[i][j] = inGrid[i][j];
      }
    }
}

void cmbNucHexLattice::setActions(const QStringList& actions)
{
  this->ActionList = actions;
}

int cmbNucHexLattice::layers()
{
  return HexGrid.numberOfLayers();
}

void cmbNucHexLattice::setLayers(int val)
{
  if(HexGrid.numberOfLayers() == val)
    {
    return;
    }
  HexGrid.SetNumberOfLayers(val);
  this->rebuild();
}

void cmbNucHexLattice::setItemShape(HexLatticeItem::ShapeStyle shapetype)
{
  this->ItemShape = shapetype;
}

void cmbNucHexLattice::setAssembly(cmbNucAssembly* assy)
{
  this->CurrentAssembly = assy;
}

void cmbNucHexLattice::addCell(
  double centerPos[2], double radius, int layer, int cellIdx)
{
  QPolygon polygon;
  polygon << QPoint(2 * radius, 0)
            << QPoint(radius, -radius * 1.73)
            << QPoint(-radius, -radius * 1.73)
            << QPoint(-2 * radius, 0)
            << QPoint(-radius, radius * 1.73)
            << QPoint(radius, radius * 1.73);
  HexLatticeItem* cell = new HexLatticeItem(polygon, layer, cellIdx,
    this->ItemShape);

  cell->setPos(centerPos[0] + this->rect().center().x(),
               centerPos[1] + this->rect().center().y());

  LatticeCell lc = HexGrid.GetCell(layer, cellIdx);
  QColor color(Qt::white);
  if(this->CurrentAssembly)
    {
    PinCell* pc = this->CurrentAssembly->GetPinCell(lc.label);
    color = pc ? pc->GetLegendColor() : Qt::white;
    }
  // update color in hex map
  this->HexGrid.SetCell(layer, cellIdx, lc.label, lc.color);
  cell->setText(lc.label.c_str());
  cell->setColor(color);

  scene()->addItem(cell);
}

void cmbNucHexLattice::rebuild()
{
  clear();
  int numLayers = HexGrid.numberOfLayers();
  if(numLayers <= 0)
    {
    return;
    }

  double centerPos[2], hexRadius, hexDiameter, layerRadius;
  int squareLength = std::min(this->width(), this->height());
  hexDiameter = squareLength / (double)(2 * numLayers + 1);
  hexRadius = hexDiameter / (double)(2 * cos(30.0 * vtkMath::Pi() / 180.0));
  double layerCorners[6][2];
  int cornerIndices[6];

  for(int i = 0; i < numLayers; i++)
    {
    if(i == 0)
      {
      centerPos[0] = 0;
      centerPos[1] = 0;
      this->addCell(centerPos, hexRadius, i, 0);
      }
    else
      {
      layerRadius = hexDiameter * (2 * i);
      int cellIdx = 0;
      for(int c = 0; c < 6; c++)
        {
        //double angle = 30.0*c*vtkMath::Pi()/180.0;
        double angle = 2 * (vtkMath::Pi() / 6.0) * (c + 3.5);
        layerCorners[c][0] = layerRadius * cos(angle);
        layerCorners[c][1] = layerRadius * sin(angle);
        // draw the corner hex
        this->addCell(layerCorners[c], hexRadius, i, cellIdx);
        cornerIndices[c] = cellIdx;
        cellIdx = i==1 ? cellIdx+1 : cellIdx+i;
        }
      if(i < 2)
        {
        continue;
        }

      // for each layer, we should have (numLayers-2) middle hexes
      // between the corners
      double deltx, delty, numSegs = i;
      for(int c = 0; c < 6; c++)
        {
        int idxN = (c + 1) % 6;
        deltx = (layerCorners[idxN][0] - layerCorners[c][0]) / numSegs;
        delty = (layerCorners[idxN][1] - layerCorners[c][1]) / numSegs;

        cellIdx = cornerIndices[c] + 1;
        for(int b = 0; b < i - 1; b++)
          {
          centerPos[0] = layerCorners[c][0] + deltx * (b + 1);
          centerPos[1] = layerCorners[c][1] + delty * (b + 1);
          this->addCell(centerPos, hexRadius, i, cellIdx++);
          }
        }
      }
    }
  this->fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
}

void cmbNucHexLattice::showContextMenu(
  HexLatticeItem *hexitem, QMouseEvent* event)
{
  if(!hexitem)
    {
    return;
    }

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
    hexitem->setText(assignAct->text());
    QColor color(Qt::white);
    if(this->CurrentAssembly)
      {
      PinCell* pc = this->CurrentAssembly->GetPinCell(hexitem->text().toStdString());
      color = pc ? pc->GetLegendColor() : Qt::white;

      hexitem->setColor(color);
      }
    this->HexGrid.SetCell(hexitem->layer(), hexitem->cellIndex(),
      assignAct->text().toStdString(), color);
    }
}

void cmbNucHexLattice::dropEvent(QDropEvent* event)
{
  HexLatticeItem* dest = dynamic_cast<HexLatticeItem*>(this->itemAt(event->pos()));
  if(!dest)
    {
    return;
    }

  dest->setText(event->mimeData()->text());
  QColor color(Qt::white);
  if(this->CurrentAssembly)
    {
    PinCell* pc = this->CurrentAssembly->GetPinCell(dest->text().toStdString());
    color = pc ? pc->GetLegendColor() : Qt::white;

    dest->setColor(color);
    }
  this->HexGrid.SetCell(dest->layer(), dest->cellIndex(),
    dest->text().toStdString(), color);
  event->acceptProposedAction();
 }

void cmbNucHexLattice::mousePressEvent(QMouseEvent* event)
{
  HexLatticeItem* hitem = dynamic_cast<HexLatticeItem*>(this->itemAt(
    this->mapFromGlobal(event->globalPos())));
  if(!hitem)
    {
    return;
    }

  // Context menu on right click
  if(event->button() == Qt::RightButton)
    {
    this->showContextMenu(hitem, event);
    }
  // Drag and drop on left click
  else if(event->button() == Qt::LeftButton)
    {
    QMimeData* mimeData = new QMimeData;
    mimeData->setText(hitem->text());

    QSize size = hitem->boundingRect().size().toSize();
    QPixmap pixmap(size.width() + 1, size.height() + 1);
    pixmap.fill(QColor(255, 255, 255, 0)); //Transparent background
    QPainter imagePainter(&pixmap);
    imagePainter.translate(-hitem->boundingRect().topLeft());

    QStyleOptionGraphicsItem gstyle;

    hitem->paint(&imagePainter, &gstyle, NULL);

    QDrag* drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setPixmap(pixmap.scaledToHeight(40, Qt::SmoothTransformation));
    drag->exec(Qt::CopyAction);

    imagePainter.end();
    }
}
