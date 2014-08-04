#include "cmbNucLatticeWidget.h"

#include "cmbNucDraw2DLattice.h"
#include "cmbNucAssembly.h"
#include "cmbNucCore.h"

#include <QVBoxLayout>

cmbNucLatticeWidget::cmbNucLatticeWidget(QWidget * parent)
:QWidget(parent), lattice(NULL)
{
  QVBoxLayout * box = new  QVBoxLayout();
  draw_control = new cmbNucDraw2DLattice(DrawLatticeItem::Polygon, NULL);
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
  if(lattice != NULL)
  {
    lattice->fillList(actionList);
    draw_control->setActions(actionList);
    draw_control->setLatticeContainer(lattice);
    draw_control->resetWithGrid(this->lattice->getLattice().Grid,
                                this->lattice->getLattice().subType);
  }
  else
  {
    draw_control->clear();
  }
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
  if(lattice != NULL)
  {
    draw_control->setHeight(v);
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

void cmbNucLatticeWidget::set_full_mode(cmbNucDraw2DLattice::CellDrawMode m)
{
  this->draw_control->setFullCellMode(m);
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
  this->draw_control->rebuild();
}
