#ifndef __DrawLatticeItem_h
#define __DrawLatticeItem_h

#include <QGraphicsPolygonItem>

class DrawLatticeItem : public QGraphicsPolygonItem
{
  typedef QGraphicsPolygonItem Superclass;

public:
  enum ShapeStyle
    {
    Circle = 0,
    Hexagon = 1,
    Rectangle = 2
    };

    DrawLatticeItem(const QPolygonF& polygon, int layer, int cellIdx,
                   DrawLatticeItem::ShapeStyle shape=DrawLatticeItem::Circle,
                   QGraphicsItem* parent = 0);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    const QString& text() const;
    void setColor(const QColor& color);
    void setText(const QString& text);
    void setShape(DrawLatticeItem::ShapeStyle sstyle);
    int layer();
    int cellIndex();

    bool is_available();
    void set_available(bool b);

protected:
    void drawCircle(QPainter* painter);
    void drawText(QPainter* painter);

private:
    QColor m_color;
    QString m_text;
    ShapeStyle m_shape;
    int m_layer;
    int m_cellIndex;
    bool m_available;
};

#endif
