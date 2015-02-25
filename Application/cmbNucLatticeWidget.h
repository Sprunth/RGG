#ifndef __cmbNucLatticeWidget_h
#define __cmbNucLatticeWidget_h

#include <QWidget>
#include "cmbNucDraw2DLattice.h"

class cmbNucDraw2DLattice;
class cmbNucAssembly;
class cmbNucCore;
class AssyPartObj;
class LatticeContainer;

class cmbNucLatticeWidget : public QWidget
{
  Q_OBJECT

public:
  cmbNucLatticeWidget(QWidget * parent);
  virtual ~cmbNucLatticeWidget();

public slots:
  void setLattice(LatticeContainer * l);
  void setLatticeXorLayers(int v);
  void setLatticeY(int v);
  void apply();
  void reset();
  void redraw();
  void set_full_mode(Lattice::CellDrawMode m);
  void clear();
  void createImage(QString);
  void updateActionList();
signals:
  void valuesChanged();
  void objGeometryChanged(AssyPartObj* selObj);
protected:
  LatticeContainer * lattice;
  cmbNucDraw2DLattice * draw_control;
};
#endif
