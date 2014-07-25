#include "cmbNucLatticeWidget.h"

#include "cmbNucHexLattice.h"
#include "cmbNucAssembly.h"
#include "cmbNucCore.h"

#include <QVBoxLayout>

cmbNucLatticeWidget::cmbNucLatticeWidget(QWidget * parent)
:QWidget(parent), lattice(NULL)
{
  QVBoxLayout * box = new  QVBoxLayout();
  draw_control = new cmbNucHexLattice(HexLatticeItem::Hexagon, NULL);
  box->addWidget(draw_control);
  this->setLayout(box);
}

cmbNucLatticeWidget::~cmbNucLatticeWidget()
{
  delete draw_control;
}

void cmbNucLatticeWidget::setLattice(LatticeContainer * l)
{
  lattice = l;
  QStringList actionList;
  actionList.append("xx");
  lattice->fillList(actionList);
  draw_control->setActions(actionList);
  draw_control->setLatticeContainer(lattice);
  draw_control->resetWithGrid(this->lattice->getLattice().Grid,
                              this->lattice->getLattice().subType);
}

void
cmbNucLatticeWidget::setLatticeXorLayers(int v)
{
  if(lattice != NULL )
  {
    draw_control->setLayers(v);
  }
}

void
cmbNucLatticeWidget::setLatticeY(int v)
{
  //TODO
  if(lattice != NULL)
  {
  }
}

void
cmbNucLatticeWidget::apply()
{
  bool change = false;
  if(lattice != NULL)
  {
    change = this->draw_control->applyToGrid(lattice->getLattice().Grid);
  }
  if(change)
  {
    emit(valuesChanged());
    emit(objGeometryChanged(lattice));
  }
}

void
cmbNucLatticeWidget::reset()
{
  if(lattice == NULL) return;
}

void
cmbNucLatticeWidget::redraw()
{
  if(lattice == NULL) return;
}
