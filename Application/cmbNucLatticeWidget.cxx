#include "cmbNucLatticeWidget.h"

#include "cmbNucDraw2DLattice.h"
#include "cmbNucAssembly.h"
#include "cmbNucCore.h"

#include <QVBoxLayout>

cmbNucLatticeWidget::cmbNucLatticeWidget(QWidget * p)
:QWidget(p), lattice(NULL)
{
  QVBoxLayout * box = new  QVBoxLayout();
  draw_control = new cmbNucDraw2DLattice(DrawLatticeItem::Polygon, NULL);
  draw_control->setObjectName("Drawing_2D_Widget");
  box->addWidget(draw_control);
  this->setLayout(box);
}

cmbNucLatticeWidget::~cmbNucLatticeWidget()
{
  delete draw_control;
}

void cmbNucLatticeWidget::updateActionList()
{
  QStringList actionList;
  if(lattice != NULL)
  {
    lattice->fillList(actionList);
    draw_control->setActions(actionList);
  }
}

void cmbNucLatticeWidget::setLattice(LatticeContainer * l)
{
  lattice = l;
  this->updateActionList();
  if(lattice != NULL)
  {
    draw_control->setLatticeContainer(lattice);
    draw_control->reset();
  }
  else
  {
    draw_control->clear();
  }
}

void cmbNucLatticeWidget::clear()
{
  this->setLattice(NULL);
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
    change = this->draw_control->apply();
  }
  if(change)
  {
    emit(valuesChanged());
    emit(objGeometryChanged(lattice));
  }
}

void cmbNucLatticeWidget::set_full_mode(Lattice::CellDrawMode m)
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

void cmbNucLatticeWidget::createImage(QString fname)
{
  draw_control->createImage(fname);
}
