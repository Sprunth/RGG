#ifndef __cmbNucLatticeWidget_h
#define __cmbNucLatticeWidget_h

#include <QWidget>

class cmbNucHexLattice;
class cmbNucAssembly;
class cmbNucCore;
class AssyPartObj;

class cmbNucLatticeWidget : public QWidget
{
  Q_OBJECT

public:
  cmbNucLatticeWidget(QWidget * parent);
  virtual ~cmbNucLatticeWidget();

public slots:
  void setAssembly(cmbNucAssembly * l);
  void setCore(cmbNucCore * l);
  void setLatticeXorLayers(int v);
  void setLatticeY(int v);
  void apply();
  void reset();
  void redraw();
signals:
  void valuesChanged();
  void objGeometryChanged(AssyPartObj* selObj);
protected:
  cmbNucAssembly * assy;
  cmbNucCore     * core;
  cmbNucHexLattice * draw_control;
};
#endif
