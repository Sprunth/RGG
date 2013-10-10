
#include <QtGui>

#include "HexLatticeItem.h"
#include "cmbNucHexLattice.h"

HexLatticeItem::HexLatticeItem(const QPolygonF & polygon,
  int layer, int cellIdx, HexLatticeItem::ShapeStyle shape,
  QGraphicsItem * parent)
    : QGraphicsPolygonItem(polygon, parent), m_color(Qt::gray),
      m_text("xx"), m_shape(shape),
      m_layer(layer), m_cellIndex(cellIdx)
{
    //setCursor(Qt::OpenHandCursor);

  //setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
}

//QRectF HexLatticeItem::boundingRect() const
//{
//   return QRectF(-15.5, -15.5, 34, 34);
//}

void HexLatticeItem::setColor(const QColor& acolor)
{
  this->m_color = acolor;
}

void HexLatticeItem::setText(const QString& atext)
{
  if(this->m_text == atext)
    {
    return;
    }
  this->m_text = atext;
  this->update();
}
void HexLatticeItem::setShape(HexLatticeItem::ShapeStyle sstyle)
{
  this->m_shape = sstyle;
}

void HexLatticeItem::drawText(QPainter *painter)
{
    QRectF textRect = boundingRect();
    int flags = Qt::AlignCenter | Qt::AlignCenter | Qt::TextWordWrap;

    QFont font;
    font.setPixelSize(15);
    painter->setPen(Qt::black);
    painter->setFont(font);
    painter->drawText(textRect, flags, m_text);
}

void HexLatticeItem::drawCircle(QPainter *painter)
{
    painter->setBrush(QBrush(m_color));
    QRectF myRect = boundingRect();
    qreal edge = std::min(myRect.width(), myRect.height())-5;
    QRectF circleRect(
      myRect.top(),myRect.left(),edge,edge);
    painter->drawEllipse(circleRect);
}

void HexLatticeItem::paint(QPainter *painter,
  const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  this->setBrush(QBrush(m_color));
  this->setPen(Qt::NoPen);
  if(this->m_shape == HexLatticeItem::Circle)
    {
    this->drawCircle(painter);
    }
  else if(this->m_shape == HexLatticeItem::Hexagon)
    {
    this->Superclass::paint(painter, option, widget);
    }

  if(!this->m_text.isEmpty())
    {
    this->drawText(painter);
    }
}

void HexLatticeItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    //setCursor(Qt::ClosedHandCursor);
  //cmbNucHexLattice* lattice = qobject_cast<cmbNucHexLattice*>(
  //  scene()->views()[0]);
  //if(lattice)
  //  {
  //  lattice->showContextMenu(this, event);
  //  }
}

void HexLatticeItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
/*
   if (QLineF(event->screenPos(), event->buttonDownScreenPos(Qt::LeftButton))
        .length() < QApplication::startDragDistance()) {
        return;
    }

    QDrag *drag = new QDrag(event->widget());
    QMimeData *mime = new QMimeData;
    drag->setMimeData(mime);

    static int n = 0;
    if (n++ > 2 && (qrand() % 3) == 0) {
        QImage image(":/images/head.png");
        mime->setImageData(image);

        drag->setPixmap(QPixmap::fromImage(image).scaled(30, 40));
        drag->setHotSpot(QPoint(15, 30));

    } else {
        mime->setColorData(color);
        mime->setText(QString("#%1%2%3")
                      .arg(color.red(), 2, 16, QLatin1Char('0'))
                      .arg(color.green(), 2, 16, QLatin1Char('0'))
                      .arg(color.blue(), 2, 16, QLatin1Char('0')));

        QPixmap pixmap(34, 34);
        pixmap.fill(Qt::white);

        QPainter painter(&pixmap);
        painter.translate(15, 15);
        painter.setRenderHint(QPainter::Antialiasing);
        paint(&painter, 0, 0);
        painter.end();

        pixmap.setMask(pixmap.createHeuristicMask());

        drag->setPixmap(pixmap);
        drag->setHotSpot(QPoint(15, 20));
    }

    drag->exec();
    setCursor(Qt::OpenHandCursor);
    */
}

void HexLatticeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *)
{
    //setCursor(Qt::OpenHandCursor);
}
