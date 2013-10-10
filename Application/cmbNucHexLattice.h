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
    cmbNucHexLattice(QWidget* parent=0, Qt::WindowFlags f=0);
    ~cmbNucHexLattice();
    int layers()
      {return HexGrid.numberOfLayers();}
    void setLayers(int val);
    void rebuild();
    void showContextMenu(HexLatticeItem *hexitem,
      QMouseEvent *event);
    void setActions(const QStringList& actions)
    {this->ActionList = actions;}
    void setItemShape(HexLatticeItem::ShapeStyle shapetype)
    { this->ItemShape = shapetype; }

protected:
  virtual void	paintEvent ( QPaintEvent * event );
  virtual void mousePressEvent(QMouseEvent *event);

private slots:
    void clear();
    void init();

    void addCircle();
    void addHexagon(double centerPos[2], double hexRadius,
     int layer, int cellIdx);
    void addText();

private:
    QGraphicsScene canvas;
    HexMap HexGrid;

    QStringList ActionList;
    HexLatticeItem::ShapeStyle ItemShape;
};

#endif
