#ifndef __cmbNucDraw2DLattice_h
#define __cmbNucDraw2DLattice_h

#include <QGraphicsView>
#include <QGraphicsScene>

#include "cmbNucPartDefinition.h"
#include "DrawLatticeItem.h"
#include "cmbNucLattice.h"

#include <map>

class QMouseEvent;
class LatticeContainer;

class cmbNucDraw2DLattice : public QGraphicsView {
  Q_OBJECT
  typedef QGraphicsView Superclass;

public:
  enum changeMode{NoChange=0, SizeChange = 1, ContentChange = 2};
  cmbNucDraw2DLattice(DrawLatticeItem::ShapeStyle shape = DrawLatticeItem::Circle,
                      QWidget* parent = 0, Qt::WindowFlags f = 0);
  ~cmbNucDraw2DLattice();

  int layers();
  void setLayers(int val);
  void setHeight(int val);
  // build lattice with HexGrid
  void rebuild();
  // build lattice with given Grid[layer][idx]
  void reset();
  int apply();
  void showContextMenu(DrawLatticeItem* hexitem, QMouseEvent* event);
  void setActions(const QStringList& actions);
  void setItemShape(DrawLatticeItem::ShapeStyle shapetype);
  void setLatticeContainer(LatticeContainer* l);
  void setFullCellMode(Lattice::CellDrawMode m);

public slots:
  void clear();
  void createImage(QString name);

protected:
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void dropEvent(QDropEvent* event);
  virtual void resizeEvent( QResizeEvent * event );

private slots:
  void init();

  void addCell(double centerPos[2],
               double radius, int layer, int cellIdx,
               Lattice::CellDrawMode mode);

private:
  LatticeContainer* CurrentLattice;
  QGraphicsScene Canvas;
  Lattice Grid;
  int changed;

  std::map<QString, int> usedLabelCount;

  Lattice::CellDrawMode FullCellMode;

  QStringList ActionList;
  DrawLatticeItem::ShapeStyle ItemShape;
};

#endif
