#include "cmbNucLatticeWidget.h"

#include "cmbNucHexLattice.h"
#include "cmbNucAssembly.h"
#include "cmbNucCore.h"

#include <QVBoxLayout>

cmbNucLatticeWidget::cmbNucLatticeWidget(QWidget * parent)
:QWidget(parent), assy(NULL), core(NULL)
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

void cmbNucLatticeWidget::setAssembly(cmbNucAssembly * l)
{
  assy = l;
  QStringList actionList;
  actionList.append("xx");
  for(size_t i = 0; i < this->assy->GetNumberOfPinCells(); i++)
  {
    PinCell *pincell = this->assy->GetPinCell(i);
    actionList.append(pincell->label.c_str());
  }
  core = NULL;
  draw_control->setActions(actionList);
  draw_control->setAssembly(assy);
  draw_control->resetWithGrid(this->assy->AssyLattice.Grid,
                              this->assy->AssyLattice.subType);

}

void cmbNucLatticeWidget::setCore(cmbNucCore * l)
{
  core = l;
  QStringList actionList;
  actionList.append("xx");

  // build the action list
  for(int i = 0; i < this->core->GetNumberOfAssemblies(); i++)
  {
    cmbNucAssembly *t = this->core->GetAssembly(i);
    actionList.append(t->label.c_str());
  }
  assy = NULL;
  draw_control->setActions(actionList);
  draw_control->setCore(core);
  draw_control->resetWithGrid(this->core->CoreLattice.Grid,
                              this->core->CoreLattice.subType);
}

void
cmbNucLatticeWidget::setLatticeXorLayers(int v)
{
  if(assy != NULL || core != NULL)
  {
    draw_control->setLayers(v);
  }
}

void
cmbNucLatticeWidget::setLatticeY(int v)
{
  //TODO
  if(assy != NULL)
  {
  }
  if(core != NULL)
  {
  }
}

void
cmbNucLatticeWidget::apply()
{
  bool change = false;
  AssyPartObj* obj;
  if(assy != NULL)
  {
    obj = assy;
    change = this->draw_control->applyToGrid(assy->AssyLattice.Grid);
  }
  if(core != NULL)
  {
    obj = core;
    change = this->draw_control->applyToGrid(core->CoreLattice.Grid);
  }
  if(change)
  {
    emit(valuesChanged());
    emit(objGeometryChanged(obj));
  }
}

void
cmbNucLatticeWidget::reset()
{
  if(assy == NULL && core == NULL) return;
}

void
cmbNucLatticeWidget::redraw()
{
  if(assy == NULL && core == NULL) return;
}
