#include "cmbNucDraw2DLattice.h"

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
#include <QPixmap>

#include "vtkMath.h"
#include "cmbNucAssembly.h"
#include "cmbNucCore.h"

#include <set>

cmbNucDraw2DLattice::cmbNucDraw2DLattice(DrawLatticeItem::ShapeStyle shape,
    QWidget* p, Qt::WindowFlags f)
      : QGraphicsView(p), CurrentLattice(NULL),
        FullCellMode(HEX_FULL), ItemShape(shape)
{
  changed = false;
  setScene(&this->Canvas);
  setInteractive(true);
  setResizeAnchor(QGraphicsView::AnchorViewCenter);
  setWindowFlags(f);
  setAcceptDrops(true);
  setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

  this->Grid.SetDimensions(0,0);
  init();
}

cmbNucDraw2DLattice::~cmbNucDraw2DLattice()
{
}

void cmbNucDraw2DLattice::clear()
{
  this->CurrentLattice = NULL;
  this->init();
}

void cmbNucDraw2DLattice::init()
{
  this->ActionList << "xx";
  this->Grid.SetDimensions(0,0);
  this->rebuild();
}

void cmbNucDraw2DLattice::reset()
{
  changed = false;
  if(this->CurrentLattice)
  {
    this->Grid = this->CurrentLattice->getLattice();
  }
  this->rebuild();
}

bool cmbNucDraw2DLattice::apply()
{
  bool hasChanged = changed; //TODO
  if(this->CurrentLattice)
  {
    this->CurrentLattice->getLattice() = this->Grid;
  }
  changed = false;
  this->rebuild();
  return hasChanged;
}

void cmbNucDraw2DLattice::setActions(const QStringList& acts)
{
  this->ActionList = acts;
}

int cmbNucDraw2DLattice::layers()
{
  return Grid.GetDimensions().first;
}

void cmbNucDraw2DLattice::setLayers(int val)
{
  std::pair<int, int> wh =Grid.GetDimensions();
  if(val == wh.first) return;
  changed = true;
  Grid.SetDimensions(val, wh.second);
  this->rebuild();
}

void cmbNucDraw2DLattice::setHeight(int val)
{
  std::pair<int, int> wh =Grid.GetDimensions();
  if(val == wh.second) return;
  changed = true;
  Grid.SetDimensions(wh.first,val);
  this->rebuild();
}

void cmbNucDraw2DLattice::setItemShape(DrawLatticeItem::ShapeStyle shapetype)
{
  this->ItemShape = shapetype;
}

void cmbNucDraw2DLattice::setLatticeContainer(LatticeContainer* l)
{
  this->CurrentLattice = l;
  if(l)
  {
    changed = false;
    this->Grid = l->getLattice();
  }
}

void cmbNucDraw2DLattice::setFullCellMode(CellDrawMode m)
{
  FullCellMode = m;
}

void cmbNucDraw2DLattice::addCell( double centerPos[2], double radius,
                                  int layer, int cellIdx, CellDrawMode mode)
{
  Lattice::LatticeCell lc = Grid.GetCell(layer, cellIdx);
  if(!lc.valid) return;
  QPolygon polygon;

  switch(mode)
  {
    case HEX_FULL:
      polygon << QPoint(0, 2 * radius)
              << QPoint(-radius * 1.73, radius )
              << QPoint(-radius * 1.73, -radius)
              << QPoint(0, -2 * radius )
              << QPoint( radius * 1.73, -radius)
              << QPoint( radius * 1.73, radius );
      break;
    case HEX_FULL_30:
      polygon << QPoint( 2 * radius, 0)
              << QPoint(radius, -radius * 1.73 )
              << QPoint(-radius, -radius * 1.73)
              << QPoint( -2 * radius, 0 )
              << QPoint( -radius, radius * 1.73)
              << QPoint( radius, radius * 1.73 );
      break;
    case HEX_SIXTH_VERT_BOTTOM:
      polygon << QPoint( 2 * radius, 0)
              << QPoint(radius, -radius * 1.73 )
              << QPoint(-radius, -radius * 1.73)
              << QPoint( -2 * radius, 0 );
      break;
    case HEX_SIXTH_VERT_CENTER:
      polygon << QPoint( 2 * radius, 0)
              << QPoint(radius, -radius * 1.73 )
              << QPoint(0, 0);
      break;
    case HEX_SIXTH_VERT_TOP:
      polygon << QPoint( 2 * radius, 0)
              << QPoint(radius, -radius * 1.73 )
              << QPoint( -radius, radius * 1.73)
              << QPoint( radius, radius * 1.73 );
      break;
    case HEX_SIXTH_FLAT_CENTER:
      polygon << QPoint(0, 0)
              << QPoint(radius * 0.865, -1.5 * radius)
              << QPoint( radius * 1.73, -radius)
              << QPoint( radius * 1.73, 0 );
      break;
    case HEX_SIXTH_FLAT_TOP:
      polygon << QPoint(0, 2 * radius) //keep
              << QPoint((-radius * 0.865), 1.5 * radius ) //half
              << QPoint(radius*0.865, -1.5 * radius )
              << QPoint( radius * 1.73, -radius)
              << QPoint( radius * 1.73, radius );
      break;
    case HEX_SIXTH_FLAT_BOTTOM:
    case HEX_TWELFTH_BOTTOM:
      polygon << QPoint(-radius * 1.73, 0 )
              << QPoint(-radius * 1.73, -radius)
              << QPoint(0, -2 * radius )
              << QPoint( radius * 1.73, -radius)
              << QPoint( radius * 1.73, 0 );
      break;
    case HEX_TWELFTH_TOP:
      polygon << QPoint(0, 2 * radius) //keep
              << QPoint( -radius * 1.73, radius)
              << QPoint( radius * 1.73, -radius)
              << QPoint( radius * 1.73, radius );
      break;
    case HEX_TWELFTH_CENTER:
      polygon << QPoint(0, 0)
              << QPoint( radius * 1.73, -radius)
              << QPoint( radius * 1.73, 0 );
      break;
      break;
    case RECT:
      polygon << QPoint(-radius,-radius)
              << QPoint(-radius, radius)
              << QPoint( radius, radius)
              << QPoint( radius,-radius);
      break;
  }
  DrawLatticeItem* cell = new DrawLatticeItem(polygon, layer, cellIdx,
    this->ItemShape);

  cell->setPos(centerPos[0] + this->rect().center().x(),
               centerPos[1] + this->rect().center().y());

  QColor color(Qt::white);
  if(this->CurrentLattice)
    {
    AssyPartObj * obj = this->CurrentLattice->getFromLabel(lc.label);
    color = obj ? obj->GetLegendColor() : Qt::white;
    }
  // update color in hex map
  this->Grid.SetCell(layer, cellIdx, lc.label, lc.color, lc.valid);
  cell->setText(lc.label.c_str());
  cell->setColor(color);
  cell->set_available(lc.valid);

  scene()->addItem(cell);
}


void cmbNucDraw2DLattice::rebuild()
{
  scene()->clear();
  int numLayers = this->layers();
  if(numLayers <= 0)
    {
    return;
    }

  double centerPos[2];
  double layerCorners[6][2];
  int cornerIndices[6];
  int squareLength = std::min(this->width(), this->height());

  if(CurrentLattice->IsHexType())
  {
    if(this->Grid.GetGeometrySubType() & ANGLE_60)
    {
      squareLength = std::min(this->width(), static_cast<int>(this->height()*1.3));
    }
    else if(this->Grid.GetGeometrySubType() & ANGLE_30)
    {
      squareLength = std::min(this->width(), static_cast<int>(this->height()*1.6));
    }
    double hexRadius, hexDiameter, layerRadius;
    hexDiameter = squareLength / static_cast<double>(3 * numLayers + 1);
    if(!(this->Grid.GetGeometrySubType() & ANGLE_360))
    {
      hexDiameter *= 1.8;
    }
    hexDiameter = std::max(hexDiameter, 20.0); // Enforce minimum size for hexes
    hexRadius = hexDiameter / static_cast<double>(2 * cos(30.0 * vtkMath::Pi() / 180.0));
    int begin, end;

    for(int i = 0; i < numLayers; i++)
      {
      Grid.getValidRange(i, begin, end);
      if(i == 0)
        {
        centerPos[0] = 0;
        centerPos[1] = 0;
        if(this->Grid.GetGeometrySubType() & ANGLE_360)
          {
          this->addCell(centerPos, hexRadius, i, 0, FullCellMode);
          }
        else if(this->Grid.GetGeometrySubType() & ANGLE_60 &&
                this->Grid.GetGeometrySubType() & FLAT)
          {
          this->addCell(centerPos, hexRadius, i, 0, HEX_SIXTH_FLAT_CENTER);
          }
        else if(this->Grid.GetGeometrySubType() & ANGLE_60 &&
                this->Grid.GetGeometrySubType() & VERTEX)
          {
          this->addCell(centerPos, hexRadius, i, 0, HEX_SIXTH_VERT_CENTER);
          }
        else if(this->Grid.GetGeometrySubType() & ANGLE_30)
          {
            this->addCell(centerPos, hexRadius, i, 0, HEX_TWELFTH_CENTER);
          }
        }
      else
        {
        /*if(this->Grid.subType & ANGLE_360)*/ layerRadius = hexDiameter * (2 * i);
        /*else layerRadius = hexDiameter * i;*/
        int cellIdx = 0;
        for(int c = 0; c < 6; c++)
          {
          double angle = 60.0*((c+4)%6)*vtkMath::Pi()/180.0;
          // draw the corner hex

          if(( this->Grid.GetGeometrySubType() & ANGLE_60 &&
               this->Grid.GetGeometrySubType() & VERTEX ) ||
             ( this->Grid.GetGeometrySubType() & ANGLE_360 && FullCellMode == HEX_FULL_30 ) )
            {
            angle = 2 * (vtkMath::Pi() / 6.0) * ((c+1)%6 + 3.5);
            }
          layerCorners[c][0] = layerRadius * cos(angle);
          layerCorners[c][1] = layerRadius * sin(angle);

          this->addCell(layerCorners[c], hexRadius, i, cellIdx,
                        getHexDrawMode(cellIdx, i, begin, end));
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
            this->addCell(centerPos, hexRadius, i, cellIdx, getHexDrawMode(cellIdx, i, begin, end));
            cellIdx++;
            }
          }
        }
      }
    }
  else
    {
    std::pair<int, int> wh = this->Grid.GetDimensions();
    double tmax = std::max(wh.first, wh.second);
    double radius = squareLength/tmax*0.5;
    for(int i = 0; i < wh.first; ++i)
      {
      for(int j = 0; j < wh.second; ++j)
        {
          centerPos[1] = 2*(wh.first-1-i)*radius;
          centerPos[0] = 2*(j)*radius;
          this->addCell(centerPos, radius, i, j, RECT);
        }
      }
    }
  scene()->setSceneRect(scene()->itemsBoundingRect());
  this->repaint();
}

cmbNucDraw2DLattice::CellDrawMode cmbNucDraw2DLattice::getHexDrawMode(int index, int layer, int start, int end) const
{
  if(this->Grid.GetGeometrySubType() & ANGLE_360)
  {
    return FullCellMode;
  }
  else if( this->Grid.GetGeometrySubType() & ANGLE_60 &&
           this->Grid.GetGeometrySubType() & FLAT )
  {
    if(index == start)
    {
      return HEX_SIXTH_FLAT_TOP;
    }
    else if(index == end)
    {
     return HEX_SIXTH_FLAT_BOTTOM;
    }
    else
    {
      return HEX_FULL;
    }
  }
  else if( this->Grid.GetGeometrySubType() & ANGLE_60 &&
           this->Grid.GetGeometrySubType() & VERTEX )
  {
    if(index == start && layer % 2 == 0) return HEX_SIXTH_VERT_TOP;
    else if(index == end && layer % 2 == 0) return HEX_SIXTH_VERT_BOTTOM;
    return HEX_FULL_30;
  }
  else if(this->Grid.GetGeometrySubType() & ANGLE_30)
  {
    if(index == end) return HEX_TWELFTH_BOTTOM;
    else if(index == start && layer % 2 == 0) return HEX_TWELFTH_TOP;
  }
  return HEX_FULL;
}

void cmbNucDraw2DLattice::showContextMenu(
  DrawLatticeItem *hexitem, QMouseEvent* qme)
{
  if(!hexitem || !hexitem->is_available())
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

  QAction* assignAct = contextMenu.exec(qme->globalPos());
  if(assignAct)
  {
    QString text = this->CurrentLattice->extractLabel(assignAct->text());
    changed |= hexitem->text() != text;
    hexitem->setText(text);
    QColor color(Qt::white);
    if(this->CurrentLattice)
    {
      AssyPartObj * obj = this->CurrentLattice->getFromLabel(text.toStdString());
      color = obj ? obj->GetLegendColor() : Qt::white;

      hexitem->setColor(color);
    }
    this->Grid.SetCell(hexitem->layer(), hexitem->cellIndex(),
                       text.toStdString(), color, true);
  }
}

void cmbNucDraw2DLattice::dropEvent(QDropEvent* qde)
{
  DrawLatticeItem* dest = dynamic_cast<DrawLatticeItem*>(this->itemAt(qde->pos()));
  if(!dest || !dest->is_available())
    {
    return;
    }

  changed |= dest->text() != qde->mimeData()->text();
  dest->setText(qde->mimeData()->text());
  QColor color(Qt::white);
  if(this->CurrentLattice)
    {
    AssyPartObj * obj = this->CurrentLattice->getFromLabel(dest->text().toStdString());
    color = obj ? obj->GetLegendColor() : Qt::white;

    dest->setColor(color);
    }
  this->Grid.SetCell(dest->layer(), dest->cellIndex(),
    dest->text().toStdString(), color, true);
  qde->acceptProposedAction();
 }

void cmbNucDraw2DLattice::resizeEvent( QResizeEvent * qre )
{
  QGraphicsView::resizeEvent(qre);
  this->rebuild();
}

void cmbNucDraw2DLattice::mousePressEvent(QMouseEvent* qme)
{
  DrawLatticeItem* hitem = dynamic_cast<DrawLatticeItem*>(this->itemAt(qme->pos()));
  if(!hitem || !hitem->is_available())
    {
    return;
    }

  // Context menu on right click
  if(qme->button() == Qt::RightButton)
    {
    this->showContextMenu(hitem, qme);
    }
  // Drag and drop on left click
  else if(qme->button() == Qt::LeftButton)
    {
    QMimeData* mimeData = new QMimeData;
    mimeData->setText(hitem->text());

    QSize tmpsize = hitem->boundingRect().size().toSize();
    QPixmap pixmap(tmpsize.width() + 1, tmpsize.height() + 1);
    pixmap.fill(QColor(255, 255, 255, 0)); //Transparent background
    QPainter imagePainter(&pixmap);
    imagePainter.translate(-hitem->boundingRect().topLeft());

    QStyleOptionGraphicsItem gstyle;

    hitem->paint(&imagePainter, &gstyle, NULL);

    QDrag* drag = new QDrag(this);
    drag->setMimeData(mimeData);
#ifdef _WIN32
    drag->setPixmap(pixmap.scaledToHeight(20, Qt::SmoothTransformation));
#else
    drag->setPixmap(pixmap.scaledToHeight(40, Qt::SmoothTransformation));
#endif
    drag->exec(Qt::CopyAction);

    imagePainter.end();
    }
}

void cmbNucDraw2DLattice::createImage(QString fname)
{
  QGraphicsView* view = new QGraphicsView(&Canvas,this);
  view->setSceneRect(Canvas.itemsBoundingRect());
  view->setMinimumHeight(600);
  view->setMinimumWidth(600);
  QPixmap pixMap = QPixmap::grabWidget(view, 0, 0, 600, 600);
  pixMap.toImage().convertToFormat(QImage::Format_RGB32).save(fname);
  delete view;
}
