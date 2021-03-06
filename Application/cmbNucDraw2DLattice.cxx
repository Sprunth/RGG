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
        FullCellMode(Lattice::HEX_FULL)
{
  changed = static_cast<int>(NoChange);
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
  this->changed = static_cast<int>(NoChange);
  if(this->CurrentLattice)
  {
    this->Grid = this->CurrentLattice->getLattice();
  }
  this->rebuild();
}

int cmbNucDraw2DLattice::apply()
{
  int hasChanged = this->changed;
  if(this->CurrentLattice && changed)
  {
    this->CurrentLattice->getLattice() = this->Grid;
    this->CurrentLattice->setUsedLabels(this->usedLabelCount);
  }
  this->changed = static_cast<int>(NoChange);
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
  this->changed |= static_cast<int>(SizeChange);
  Grid.SetDimensions(val, wh.second);
  this->rebuild();
}

void cmbNucDraw2DLattice::setHeight(int val)
{
  std::pair<int, int> wh =Grid.GetDimensions();
  if(val == wh.second) return;
  this->changed |= static_cast<int>(SizeChange);
  Grid.SetDimensions(wh.first,val);
  this->rebuild();
}

void cmbNucDraw2DLattice::setLatticeContainer(LatticeContainer* l)
{
  this->CurrentLattice = l;
  if(l)
  {
    this->changed = static_cast<int>(NoChange);
    this->Grid = l->getLattice();
  }
}

void cmbNucDraw2DLattice::addCell( double centerPos[2], double radius,
                                  int layer, int cellIdx,
                                  Lattice::CellDrawMode mode)
{
  Lattice::LatticeCell lc = Grid.GetCell(layer, cellIdx);
  if(!lc.valid) return;
  QPolygon polygon;

  switch(mode)
  {
    case Lattice::HEX_FULL:
      polygon << QPoint(0, 2 * radius)
              << QPoint(-radius * 1.73, radius )
              << QPoint(-radius * 1.73, -radius)
              << QPoint(0, -2 * radius )
              << QPoint( radius * 1.73, -radius)
              << QPoint( radius * 1.73, radius );
      break;
    case Lattice::HEX_FULL_30:
      polygon << QPoint( 2 * radius, 0)
              << QPoint(radius, -radius * 1.73 )
              << QPoint(-radius, -radius * 1.73)
              << QPoint( -2 * radius, 0 )
              << QPoint( -radius, radius * 1.73)
              << QPoint( radius, radius * 1.73 );
      break;
    case Lattice::HEX_SIXTH_VERT_BOTTOM:
      polygon << QPoint( 2 * radius, 0)
              << QPoint(radius, -radius * 1.73 )
              << QPoint(-radius, -radius * 1.73)
              << QPoint( -2 * radius, 0 );
      break;
    case Lattice::HEX_SIXTH_VERT_CENTER:
      polygon << QPoint( 2 * radius, 0)
              << QPoint(radius, -radius * 1.73 )
              << QPoint(0, 0);
      break;
    case Lattice::HEX_SIXTH_VERT_TOP:
      polygon << QPoint( 2 * radius, 0)
              << QPoint(radius, -radius * 1.73 )
              << QPoint( -radius, radius * 1.73)
              << QPoint( radius, radius * 1.73 );
      break;
    case Lattice::HEX_SIXTH_FLAT_CENTER:
      polygon << QPoint(0, 0)
              << QPoint(radius * 0.865, -1.5 * radius)
              << QPoint( radius * 1.73, -radius)
              << QPoint( radius * 1.73, 0 );
      break;
    case Lattice::HEX_SIXTH_FLAT_TOP:
      polygon << QPoint(0, 2 * radius) //keep
              << QPoint((-radius * 0.865), 1.5 * radius ) //half
              << QPoint(radius*0.865, -1.5 * radius )
              << QPoint( radius * 1.73, -radius)
              << QPoint( radius * 1.73, radius );
      break;
    case Lattice::HEX_SIXTH_FLAT_BOTTOM:
    case Lattice::HEX_TWELFTH_BOTTOM:
      polygon << QPoint(-radius * 1.73, 0 )
              << QPoint(-radius * 1.73, -radius)
              << QPoint(0, -2 * radius )
              << QPoint( radius * 1.73, -radius)
              << QPoint( radius * 1.73, 0 );
      break;
    case Lattice::HEX_TWELFTH_TOP:
      polygon << QPoint(0, 2 * radius) //keep
              << QPoint( -radius * 1.73, radius)
              << QPoint( radius * 1.73, -radius)
              << QPoint( radius * 1.73, radius );
      break;
    case Lattice::HEX_TWELFTH_CENTER:
      polygon << QPoint(0, 0)
              << QPoint( radius * 1.73, -radius)
              << QPoint( radius * 1.73, 0 );
      break;
      break;
    case Lattice::RECT:
      polygon << QPoint(-radius,-radius)
              << QPoint(-radius, radius)
              << QPoint( radius, radius)
              << QPoint( radius,-radius);
      break;
  }
  DrawLatticeItem* cell = new DrawLatticeItem(polygon, layer, cellIdx);

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
  QString text(lc.label.c_str());
  usedLabelCount[text]++;
  cell->setText(lc.label.c_str());
  cell->setColor(color);
  cell->set_available(lc.valid);

  scene()->addItem(cell);
}


void cmbNucDraw2DLattice::rebuild()
{
  scene()->clear();
  usedLabelCount.clear();
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
    double hexRadius, hexDiameter, layerRadius;
    if(this->Grid.GetGeometrySubType() & ANGLE_60 ||
       this->Grid.GetGeometrySubType() & ANGLE_30)
    {
      if(this->Grid.GetGeometrySubType() & ANGLE_60 &&
         this->Grid.GetGeometrySubType() & FLAT)
      {
        double n = 1/(1.75*numLayers - 0.5);
        double t1 = this->width()*(n);
        double t2 = this->height()*n/0.86602540378443864676372317075294;
        hexDiameter = std::min(t1, t2);

      }
      else if(this->Grid.GetGeometrySubType() & ANGLE_60 &&
              this->Grid.GetGeometrySubType() & VERTEX)
      {
        double n = 2*numLayers - 0.4;
        double t1 = this->width()/(0.86602540378443864676372317075294*0.86602540378443864676372317075294*n);
        double nl = 0;
        if(numLayers%2 == 0) nl = numLayers*0.5*1.5 - 0.5;//even
        else nl = (numLayers-1)*0.5*1.5 + 0.5;//odd
        double t2 = this->height()/(2*(nl+0.1)*0.86602540378443864676372317075294);
        hexDiameter = std::min(t1, t2);
      }
      else
      {
        double n = 1/(1.75*numLayers - 0.5);
        double t1 = this->width()*(n);
        double t2 = this->height()*(n*1.8)/0.86602540378443864676372317075294;
        hexDiameter = std::min(t1, t2);
      }
    }
    else
    {
      hexDiameter = squareLength / static_cast<double>(3 * numLayers + 1.0);
    }
    hexDiameter = std::max(hexDiameter, 20.0); // Enforce minimum size for hexes
    hexRadius = hexDiameter / 2.0; //static_cast<double>(2 * cos(30.0 * vtkMath::Pi() / 180.0));
    int begin, end;
    double rad2 = 0.86602540378443864676372317075294*hexRadius;

    for(int i = 0; i < numLayers; i++)
    {
      Grid.getValidRange(i, begin, end);
      if(i == 0)
      {
        centerPos[0] = 0;
        centerPos[1] = 0;
        this->addCell(centerPos, hexRadius, i, 0, this->Grid.getDrawMode(0, i));
      }
      else
      {
        /*if(this->Grid.subType & ANGLE_360)*/ layerRadius = rad2 * (4 * i);
        /*else layerRadius = hexDiameter * i;*/
        int cellIdx = 0;
        for(int c = 0; c < 6; c++)
        {
          double angle = 60.0*((c+4)%6)*vtkMath::Pi()/180.0;
          // draw the corner hex

          if( this->Grid.GetGeometrySubType() & ANGLE_60 &&
              this->Grid.GetGeometrySubType() & VERTEX )
          {
            angle = 2 * (vtkMath::Pi() / 6.0) * ((c+1)%6 + 3.5);
          }
          else if( this->Grid.GetGeometrySubType() & ANGLE_360 &&
                   this->Grid.getFullCellMode() == Lattice::HEX_FULL_30 )
          {
            angle = 2 * (vtkMath::Pi() / 6.0) * ((c)%6 + 3.5);
          }
          layerCorners[c][0] = layerRadius * cos(angle);
          layerCorners[c][1] = layerRadius * sin(angle);

          this->addCell(layerCorners[c], hexRadius, i, cellIdx,
                        this->Grid.getDrawMode(cellIdx, i));
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
            this->addCell(centerPos, hexRadius, i, cellIdx,
                          this->Grid.getDrawMode(cellIdx, i));
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
    radius = std::max(radius, 20.0);
    for(int i = 0; i < wh.first; ++i)
    {
      for(int j = 0; j < wh.second; ++j)
      {
          centerPos[1] = 2*(wh.first-1-i)*radius;
          centerPos[0] = 2*(j)*radius;
          this->addCell(centerPos, radius, i, j, this->Grid.getDrawMode(i, j));
      }
    }
  }
  scene()->setSceneRect(scene()->itemsBoundingRect());
  this->repaint();
}

void cmbNucDraw2DLattice::showContextMenu( DrawLatticeItem *hexitem,
                                          QMouseEvent* qme )
{
  if(!hexitem || !hexitem->is_available())
  {
    return;
  }

  QMenu contextMenu(this);
  contextMenu.setObjectName("Replace_Menu");
  QMenu * replaceMenu = new QMenu("Replace All With", &contextMenu);
  replaceMenu->setObjectName("Replace_All_With");
  QMenu * fillRing = new QMenu("Fill Ring All With", &contextMenu);
  fillRing->setObjectName("Fill_Ring_All_With");
  QAction* pAction = NULL;
  // available parts
  foreach(QString strAct, this->ActionList)
  {
    pAction = new QAction(strAct, this);
    pAction->setData(0);
    contextMenu.addAction(pAction);
    pAction = new QAction(strAct, this);
    pAction->setData(1);
    replaceMenu->addAction(pAction);
    pAction = new QAction(strAct, this);
    pAction->setData(2);
    fillRing->addAction(pAction);
  }
  contextMenu.addMenu( replaceMenu );
  contextMenu.addMenu( fillRing );

  QAction* assignAct = contextMenu.exec(qme->globalPos());
  if(assignAct)
  {
    QString text = this->CurrentLattice->extractLabel(assignAct->text());
    if(assignAct->data().toInt() == 0)
    {
      this->changed |= (hexitem->text() != text)?(static_cast<int>(ContentChange)):(static_cast<int>(NoChange));
      usedLabelCount[hexitem->text()]--;
      usedLabelCount[text]++;
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
    else if(assignAct->data().toInt() == 1)
    {
      if(hexitem->text() != text)
      {
        this->changed |= static_cast<int>(ContentChange);
        this->Grid.replaceLabel(hexitem->text().toStdString(), text.toStdString());
        this->rebuild();
        this->repaint();
      }
    }
    else if(assignAct->data().toInt() == 2)
    {
      bool v = this->Grid.fillRing(hexitem->layer(), hexitem->cellIndex(), text.toStdString());
      if(v) this->changed |= static_cast<int>(ContentChange);
      this->rebuild();
      this->repaint();
    }
  }
}

void cmbNucDraw2DLattice::dropEvent(QDropEvent* qde)
{
  DrawLatticeItem* dest = dynamic_cast<DrawLatticeItem*>(this->itemAt(qde->pos()));
  if(!dest || !dest->is_available())
  {
    return;
  }

  this->changed |= (dest->text() != qde->mimeData()->text())?(static_cast<int>(ContentChange)):(static_cast<int>(NoChange));
  usedLabelCount[dest->text()]--;
  usedLabelCount[qde->mimeData()->text()]++;
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
