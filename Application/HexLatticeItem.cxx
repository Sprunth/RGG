#include "HexLatticeItem.h"
#include "cmbNucHexLattice.h"

HexLatticeItem::HexLatticeItem(const QPolygonF& polygon, int layer, int cellIdx,
  HexLatticeItem::ShapeStyle shape, QGraphicsItem* parent)
    : QGraphicsPolygonItem(polygon, parent), m_color(Qt::gray),
      m_text("xx"), m_shape(shape),
      m_layer(layer), m_cellIndex(cellIdx), m_available(true)
{
  this->setAcceptDrops(true);
}

bool HexLatticeItem::is_available()
{
  return m_available;
}

void HexLatticeItem::set_available(bool b)
{
  m_available = b;
}

const QString& HexLatticeItem::text() const
{
  //if(m_available)
  return this->m_text;
  //return QString("");
}

void HexLatticeItem::setColor(const QColor& color)
{
  this->m_color = color;
}

void HexLatticeItem::setText(const QString& text)
{
  if(this->m_text == text)
    {
    return;
    }
  this->m_text = text;
  this->update();
}

void HexLatticeItem::setShape(HexLatticeItem::ShapeStyle sstyle)
{
  this->m_shape = sstyle;
}

int HexLatticeItem::layer()
{
  return m_layer;
}

int HexLatticeItem::cellIndex()
{
  return m_cellIndex;
}

void HexLatticeItem::drawText(QPainter *painter)
{
  QRectF textRect = boundingRect();
  int flags = Qt::AlignCenter | Qt::AlignCenter | Qt::TextWordWrap;

  QColor textColor = (this->m_color.lightnessF() < 0.5) ? Qt::white : Qt::black;

  QFont font;
  font.setPixelSize(12);
  painter->setPen(textColor);
  painter->setFont(font);
  painter->drawText(textRect, flags, m_text);
}

void HexLatticeItem::drawCircle(QPainter *painter)
{
  painter->setPen(Qt::gray);
  painter->setBrush(QBrush(m_color));
  QRectF myRect = boundingRect();
  qreal edge = std::min(myRect.width(), myRect.height()) - 1;
  QRectF circleRect(myRect.left() + 6, myRect.top() + 1, edge, edge);
  painter->drawEllipse(circleRect);
}

void HexLatticeItem::paint(QPainter* painter,
  const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  this->setBrush(QBrush((m_available)?m_color:Qt::black));
  painter->setPen(Qt::gray);

  if(this->m_shape == HexLatticeItem::Circle)
    {
    this->drawCircle(painter);
    }
  else if(this->m_shape == HexLatticeItem::Hexagon || this->m_shape == HexLatticeItem::Rectangle)
    {
    this->Superclass::paint(painter, option, widget);
    }

  if(!this->m_text.isEmpty())
    {
    this->drawText(painter);
    }
}
