
#ifndef __HexLatticeItem_h
#define __HexLatticeItem_h

#include <QGraphicsPolygonItem>

class HexLatticeItem : public QGraphicsPolygonItem
{
  typedef QGraphicsPolygonItem Superclass;

public:
  enum ShapeStyle
    {
    Circle = 0,
    Hexagon = 1
    };

    HexLatticeItem(const QPolygonF& polygon, int layer, int cellIdx,
                   HexLatticeItem::ShapeStyle shape=HexLatticeItem::Circle,
                   QGraphicsItem* parent = 0);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    const QString& text() const;
    void setColor(const QColor& color);
    void setText(const QString& text);
    void setShape(HexLatticeItem::ShapeStyle sstyle);
    int layer();
    int cellIndex();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
    void drawCircle(QPainter* painter);
    void drawText(QPainter* painter);

private:
    QColor m_color;
    QString m_text;
    ShapeStyle m_shape;
    int m_layer;
    int m_cellIndex;
};

#endif
