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

#include "vtkMath.h"

cmbNucHexLattice::cmbNucHexLattice(HexLatticeItem::ShapeStyle shape,
    QWidget* parent, Qt::WindowFlags f)
      : QGraphicsView(parent), ItemShape(shape)
{
  setScene(&this->Canvas);
  setInteractive(true);
  setResizeAnchor(QGraphicsView::AnchorViewCenter);
  setWindowFlags(f);
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

void cmbNucHexLattice::addCell(
  double centerPos[2], double radius, int layer, int cellIdx)
{
  QPolygon polygon;
  polygon << QPoint(2 * radius, 0)
            << QPoint(radius, -radius * 173/100)
            << QPoint(-radius, -radius * 173/100)
            << QPoint(-2 * radius, 0)
            << QPoint(-radius, radius * 173/100)
            << QPoint(radius, radius * 173/100);
  HexLatticeItem* cell = new HexLatticeItem(polygon, layer, cellIdx,
    this->ItemShape);
  cell->setText(HexGrid.GetCell(layer, cellIdx).c_str());
  scene()->addItem(cell);
  int center_x = this->rect().center().x();
  int center_y = this->rect().center().y();
  cell->setPos(centerPos[0] + center_x, centerPos[1] + center_y);
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
        double angle = 2 * (vtkMath::Pi() / 6.0) * (c + 0.5);
        layerCorners[c][0] = layerRadius * cos(angle);
        layerCorners[c][1] = layerRadius * sin(angle);
        // draw the corner hex
        this->addCell(layerCorners[c], hexRadius, i, cellIdx++);
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

        for(int b = 0; b < i - 1; b++)
          {
          centerPos[0] = layerCorners[c][0] + deltx * (b + 1);
          centerPos[1] = layerCorners[c][1] + delty * (b + 1);
          this->addCell(centerPos, hexRadius, i, cellIdx++);
          }
        }
      }
    }
  this->fitInView(scene()->sceneRect());
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
    this->HexGrid.SetCell(
      hexitem->layer(), hexitem->cellIndex(),
      assignAct->text().toStdString());
    }
}

void cmbNucHexLattice::mousePressEvent(QMouseEvent* event)
{
  QGraphicsItem* gitem = this->itemAt(this->mapFromGlobal(
    event->globalPos()));
  if(!gitem)
    {
    return;
    }
  HexLatticeItem* hitem = dynamic_cast<HexLatticeItem*>(gitem);
  if(!hitem)
    {
    return;
    }

  // Context menu on right click
  if(event->button() == Qt::RightButton)
    {
    this->showContextMenu(hitem, event);
    }
  // TODO drag and drop on left click?
}
