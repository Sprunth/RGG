#ifndef __cmbNucHexLattice_h
#define __cmbNucHexLattice_h

#include <QGraphicsView>
#include <QGraphicsScene>

#include "cmbNucPartDefinition.h"
#include "HexLatticeItem.h"

class QMouseEvent;
class cmbNucAssembly;
class cmbNucCore;

class cmbNucHexLattice : public QGraphicsView {
  Q_OBJECT
  typedef QGraphicsView Superclass;

public:
  cmbNucHexLattice(HexLatticeItem::ShapeStyle shape = HexLatticeItem::Circle,
                   QWidget* parent = 0, Qt::WindowFlags f = 0);
  ~cmbNucHexLattice();

  int layers();
  void setLayers(int val);
  // build lattice with HexGrid
  void rebuild();
  // build lattice with given Grid[layer][idx]
  void resetWithGrid(std::vector<std::vector<LatticeCell> >& inGrid);
  void applyToGrid(
    std::vector<std::vector<LatticeCell> >& outGrid);
  void showContextMenu(HexLatticeItem* hexitem, QMouseEvent* event);
  void setActions(const QStringList& actions);
  void setItemShape(HexLatticeItem::ShapeStyle shapetype);
  void setAssembly(cmbNucAssembly* assy);
  void setCore(cmbNucCore* core);

protected:
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void dropEvent(QDropEvent* event);

private slots:
  void clear();
  void init();
  void copyGrid(std::vector<std::vector<LatticeCell> >& inGrid,
    std::vector<std::vector<LatticeCell> >& outGrid);

  void addCell(double centerPos[2], double radius, int layer, int cellIdx);

private:
  cmbNucAssembly* CurrentAssembly;
  cmbNucCore* CurrentCore;
  QGraphicsScene Canvas;
  HexMap HexGrid;

  QStringList ActionList;
  HexLatticeItem::ShapeStyle ItemShape;
};

#endif
