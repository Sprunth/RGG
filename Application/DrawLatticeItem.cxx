#include "DrawLatticeItem.h"

#include <QFont>
#include <QPainter>

DrawLatticeItem::DrawLatticeItem(const QPolygonF& poly, int l, int cellIdx,
  DrawLatticeItem::ShapeStyle ss, QGraphicsItem* parent)
    : QGraphicsPolygonItem(poly, parent), m_color(Qt::gray),
      m_text("xx"), m_shape(ss),
      m_layer(l), m_cellIndex(cellIdx), m_available(true)
{
  this->setAcceptDrops(true);
}

bool DrawLatticeItem::is_available()
{
  return m_available;
}

void DrawLatticeItem::set_available(bool b)
{
  m_available = b;
}

const QString& DrawLatticeItem::text() const
{
  //if(m_available)
  return this->m_text;
  //return QString("");
}

void DrawLatticeItem::setColor(const QColor& color)
{
  this->m_color = color;
}

void DrawLatticeItem::setText(const QString& tin)
{
  if(this->m_text == tin)
    {
    return;
    }
  this->m_text = tin;
  this->update();
}

void DrawLatticeItem::setShape(DrawLatticeItem::ShapeStyle sstyle)
{
  this->m_shape = sstyle;
}

int DrawLatticeItem::layer()
{
  return m_layer;
}

int DrawLatticeItem::cellIndex()
{
  return m_cellIndex;
}

void DrawLatticeItem::drawText(QPainter *painter)
{
  QRectF textRect = boundingRect();

  double gray = this->m_color.red()*0.299 +
                this->m_color.green()*0.587 +
                this->m_color.blue()*0.114;

  QColor textColor = ( gray < 186 ) ? Qt::white : Qt::black;

  QFont font;
  font.setPixelSize(12);
  painter->setPen(textColor);
  painter->setFont(font);
  painter->drawText(textRect, Qt::AlignCenter | Qt::AlignCenter | Qt::TextWordWrap, m_text);
}

void DrawLatticeItem::drawCircle(QPainter *painter)
{
  painter->setPen(Qt::gray);
  painter->setBrush(QBrush(m_color));
  QRectF myRect = boundingRect();
  qreal edge = std::min(myRect.width(), myRect.height()) - 1;
  QRectF circleRect(myRect.left() + 6, myRect.top() + 1, edge, edge);
  painter->drawEllipse(circleRect);
}

void DrawLatticeItem::paint(QPainter* painter,
  const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  this->setBrush(QBrush((m_available)?m_color:Qt::black));
  painter->setPen(Qt::gray);

  if(this->m_shape == DrawLatticeItem::Circle)
    {
    this->drawCircle(painter);
    }
  else if(this->m_shape == DrawLatticeItem::Polygon)
    {
    this->Superclass::paint(painter, option, widget);
    }

  if(!this->m_text.isEmpty())
    {
    this->drawText(painter);
    }
}
