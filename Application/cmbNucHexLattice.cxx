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

cmbNucHexLattice::cmbNucHexLattice(QWidget* parent, Qt::WindowFlags f) :
    QGraphicsView(parent), ItemShape(HexLatticeItem::Circle)
{
    setScene(&canvas);
    setInteractive(true);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setWindowFlags(f);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    HexGrid.SetNumberOfLayers(1);
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
  this->ActionList << "testAct1" << "testAct2";
  this->rebuild();
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

void cmbNucHexLattice::addHexagon(double centerPos[2],
  double hexRadius, int layer, int cellIdx)
{
  QPolygon polygon;
  polygon << QPoint(2*hexRadius,0)
            << QPoint(hexRadius,-hexRadius*173/100)
            << QPoint(-hexRadius,-hexRadius*173/100)
            << QPoint(-2*hexRadius,0)
            << QPoint(-hexRadius,hexRadius*173/100)
            << QPoint(hexRadius,hexRadius*173/100);
  HexLatticeItem* i = new HexLatticeItem(polygon, layer, cellIdx,
    this->ItemShape);

//  QGraphicsPolygonItem* i = canvas.addPolygon(polygon);
  i->setText(HexGrid.GetCell(layer, cellIdx).c_str());
  scene()->addItem(i);
  int center_x = this->rect().center().x();
  int center_y = this->rect().center().y();
  //i->setFlag(QGraphicsItem::ItemIsMovable);
  //i->setZValue(qrand()%256);
  i->setPos(centerPos[0]+center_x,centerPos[1]+center_y);
}

void cmbNucHexLattice::addText()
{
    QGraphicsTextItem* i = scene()->addText("QCanvasText");
    i->setFlag(QGraphicsItem::ItemIsMovable);
    i->setPos(qrand()%int(scene()->width()),qrand()%int(scene()->height()));
    i->setZValue(qrand()%256);
}

void cmbNucHexLattice::addCircle()
{
    QAbstractGraphicsShapeItem* i = scene()->addEllipse(QRectF(0,0,50,50));
    i->setFlag(QGraphicsItem::ItemIsMovable);
    i->setPen(Qt::NoPen);
    i->setBrush( QColor(qrand()%32*8,qrand()%32*8,qrand()%32*8) );
    i->setPos(qrand()%int(scene()->width()),qrand()%int(scene()->height()));
    i->setZValue(qrand()%256);
}

void cmbNucHexLattice::paintEvent(QPaintEvent * pEvent)
{
    //QPainter p(this);
    this->Superclass::paintEvent(pEvent);
}

void cmbNucHexLattice::rebuild()
{
  clear();
  int numLayers = HexGrid.numberOfLayers();
  if( numLayers <= 0)
    {
    return;
    }
  double centerPos[2], hexRadius, hexDiameter, layerRadius;
  int sqareLength = this->width()>this->height() ?
    this->height() : this->width();
  hexDiameter = sqareLength/(double)(2*numLayers+1);
  hexRadius = hexDiameter/(double)(2*cos(30.0*vtkMath::Pi()/180.0));
  double layerCorners[6][2];

  for (int i=0; i < numLayers; i++)
    {
    if(i==0)
      {
      centerPos[0]=0; centerPos[1] = 0;
      addHexagon(centerPos, hexRadius, i, 0);
      }
    else
      {
      layerRadius = hexDiameter*(2*i);
      int cellIdx = 0;
      for(int c=0; c<6 ;c++)
        {
        //double angle = 30.0*c*vtkMath::Pi()/180.0;
        double angle = 2 * (vtkMath::Pi() / 6.0) * (c + 0.5);
        layerCorners[c][0] = layerRadius * cos(angle);
        layerCorners[c][1] = layerRadius * sin(angle);
        // draw the corner hex
        addHexagon(layerCorners[c], hexRadius, i, cellIdx++);
        }
      if( i < 2 )
        {
        continue;
        }

      // for each layer, we should have (numLayers-2) middle hexes
      // between the corners
      double deltx, delty, numSegs = i;
      for(int c=0; c<6 ;c++)
        {
        int idxN = c==5 ? 0 : c+1;
        deltx = (layerCorners[idxN][0]-layerCorners[c][0])/numSegs;
        delty = (layerCorners[idxN][1]-layerCorners[c][1])/numSegs;

        for(int b=0; b<i-1; b++)
          {
          centerPos[0] = layerCorners[c][0] + deltx*(b+1);
          centerPos[1] = layerCorners[c][1] + delty*(b+1);
          addHexagon(centerPos, hexRadius, i, cellIdx++);
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

void cmbNucHexLattice::mousePressEvent(QMouseEvent *event)
{
  QGraphicsItem* gitem = this->itemAt(this->mapFromGlobal(
    event->globalPos()));
  if(!gitem)
    {
    return;
    }
  HexLatticeItem* hitem = dynamic_cast<HexLatticeItem*>(gitem);
  if(hitem)
  {
  this->showContextMenu(hitem, event);
  }
}
