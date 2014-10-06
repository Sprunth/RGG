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
  enum CellDrawMode{RECT, HEX_FULL, HEX_FULL_30,
                    HEX_SIXTH_FLAT_BOTTOM, HEX_SIXTH_FLAT_CENTER, HEX_SIXTH_FLAT_TOP,
                    HEX_SIXTH_VERT_BOTTOM, HEX_SIXTH_VERT_CENTER, HEX_SIXTH_VERT_TOP,
                    HEX_TWELFTH_BOTTOM, HEX_TWELFTH_CENTER, HEX_TWELFTH_TOP};
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
  void setFullCellMode(CellDrawMode m);

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

  void addCell(double centerPos[2], double radius, int layer, int cellIdx, CellDrawMode mode);

private:
  LatticeContainer* CurrentLattice;
  QGraphicsScene Canvas;
  Lattice Grid;

  CellDrawMode FullCellMode;

  QStringList ActionList;
  DrawLatticeItem::ShapeStyle ItemShape;

  CellDrawMode getHexDrawMode(int index, int layer, int begin, int end) const;
};

#endif
