#ifndef __cmbNucDraw2DLattice_h
#define __cmbNucDraw2DLattice_h

#include <QGraphicsView>
#include <QGraphicsScene>

#include "cmbNucPartDefinition.h"
#include "DrawLatticeItem.h"
#include "cmbNucLattice.h"

class QMouseEvent;
class LatticeContainer;

class cmbNucDraw2DLattice : public QGraphicsView {
  Q_OBJECT
  typedef QGraphicsView Superclass;

public:
  cmbNucDraw2DLattice(DrawLatticeItem::ShapeStyle shape = DrawLatticeItem::Circle,
                      QWidget* parent = 0, Qt::WindowFlags f = 0);
  ~cmbNucDraw2DLattice();

  int layers();
  void setLayers(int val);
  void setHeight(int val);
  // build lattice with HexGrid
  void rebuild();
  // build lattice with given Grid[layer][idx]
  void resetWithGrid(std::vector<std::vector<Lattice::LatticeCell> >& inGrid, int subType);
  bool applyToGrid(std::vector<std::vector<Lattice::LatticeCell> >& outGrid);
  void showContextMenu(DrawLatticeItem* hexitem, QMouseEvent* event);
  void setActions(const QStringList& actions);
  void setItemShape(DrawLatticeItem::ShapeStyle shapetype);
  void setLatticeContainer(LatticeContainer* l);

public slots:
  void clear();

protected:
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void dropEvent(QDropEvent* event);
  virtual void resizeEvent( QResizeEvent * event );

private slots:
  void init();
  bool copyGrid(std::vector<std::vector<Lattice::LatticeCell> >& inGrid,
    std::vector<std::vector<Lattice::LatticeCell> >& outGrid);

  void addCell(double centerPos[2], double radius, int layer, int cellIdx, bool hex);

private:
  LatticeContainer* CurrentLattice;
  QGraphicsScene Canvas;
  Lattice Grid;

  QStringList ActionList;
  DrawLatticeItem::ShapeStyle ItemShape;
};

#endif
