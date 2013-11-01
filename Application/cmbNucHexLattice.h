#ifndef __cmbNucHexLattice_h
#define __cmbNucHexLattice_h

#include <QGraphicsView>
#include <QGraphicsScene>

#include "cmbNucPartDefinition.h"
#include "HexLatticeItem.h"

class QMouseEvent;

class cmbNucHexLattice : public QGraphicsView {
    Q_OBJECT
    typedef QGraphicsView Superclass;

public:
    cmbNucHexLattice(HexLatticeItem::ShapeStyle shape = HexLatticeItem::Circle,
                     QWidget* parent = 0, Qt::WindowFlags f = 0);
    ~cmbNucHexLattice();

    int layers();
    void setLayers(int val);
    void rebuild();
    void showContextMenu(HexLatticeItem* hexitem, QMouseEvent* event);
    void setActions(const QStringList& actions);
    void setItemShape(HexLatticeItem::ShapeStyle shapetype);

protected:
  virtual void mousePressEvent(QMouseEvent* event);

private slots:
    void clear();
    void init();

    void addCell(double centerPos[2], double radius, int layer, int cellIdx);

private:
    QGraphicsScene Canvas;
    HexMap HexGrid;

    QStringList ActionList;
    HexLatticeItem::ShapeStyle ItemShape;
};

#endif
