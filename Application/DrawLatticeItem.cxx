#include "DrawLatticeItem.h"

#include <QFont>
#include <QPainter>

DrawLatticeItem::DrawLatticeItem(const QPolygonF& poly, int l, int cellIdx,
                                 QGraphicsItem* parent)
    : QGraphicsPolygonItem(poly, parent), m_color(Qt::gray),
      m_text("xx"),
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
  return this->m_text;
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

void DrawLatticeItem::paint(QPainter* painter,
  const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  this->setBrush(QBrush((m_available)?m_color:Qt::black));
  painter->setPen(Qt::gray);

  this->Superclass::paint(painter, option, widget);

  if(!this->m_text.isEmpty())
    {
    this->drawText(painter);
    }
}
