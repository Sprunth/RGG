#ifndef __cmbNucLatticeWidget_h
#define __cmbNucLatticeWidget_h

#include <QWidget>

class cmbNucHexLattice;
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
signals:
  void valuesChanged();
  void objGeometryChanged(AssyPartObj* selObj);
protected:
  LatticeContainer * lattice;
  //cmbNucAssembly * assy;
  //cmbNucCore     * core;
  cmbNucHexLattice * draw_control;
};
#endif
