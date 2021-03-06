#include "cmbNucPinCell.h"
#include "cmbNucMaterialColors.h"

#include <cassert>
#include <cmath>
#include <algorithm>
#include <set>

#include <QDebug>

PinSubPart::PinSubPart(double z1in, double z2in) : Materials(1, new cmbNucMaterialLayer())
{
  this->Label = "PinPart";
  x=0.0; y=0.0; z1=z1in; z2=z2in;
  Connection = new PinConnection();
  this->setConnection(Materials[0]);
}

PinSubPart::~PinSubPart()
{
  for(unsigned i = 0; i < this->Materials.size(); ++i)
  {
    delete this->Materials[i];
    this->Materials[i] = NULL;
  }
  delete Connection;
}

PinConnection* PinSubPart::GetConnection() const
{
  return Connection;
}

QPointer<cmbNucMaterial> PinSubPart::GetMaterial(int i)
{
  if(i>=0 && static_cast<size_t>(i)<this->Materials.size())
  {
    return this->Materials[i]->getMaterial();
  }
  return NULL;
}

void
PinSubPart::SetMaterial(int i,
                        QPointer<cmbNucMaterial> material)
{
  if(i>=0 && static_cast<size_t>(i)<this->Materials.size())
  {
    this->Materials[i]->changeMaterial( material );
  }
}

void PinSubPart::SetNumberOfLayers(int numLayers)
{
  if(this->Materials.size() == static_cast<size_t>(numLayers)) return;
  for(size_t i = static_cast<size_t>(numLayers); i < this->Materials.size(); ++i)
  {
    delete this->Materials[i];
    this->Materials[i] = NULL;
  }
  this->Materials.resize(numLayers, NULL);
  for(size_t at = 0; at < this->Materials.size(); at++)
  {
    if(this->Materials[at] == NULL) this->Materials[at] = new cmbNucMaterialLayer();
    this->setConnection(this->Materials[at]);
  }
}

void PinSubPart::setMaterialLayer(int i, cmbNucMaterialLayer * m)
{
  if(m == NULL) return;
  if(i < 0) return;
  if(static_cast<std::size_t>(i) >= this->Materials.size()) this->SetNumberOfLayers(i+1);
  *(this->Materials[i]) = *m;
  delete m;
}

std::size_t PinSubPart::GetNumberOfLayers() const
{
  return Materials.size();
}

void PinSubPart::setConnection(cmbNucMaterialLayer * layer)
{
  assert(layer != NULL);
  assert(this->Connection != NULL);
  QObject::connect(layer->GetConnection(), SIGNAL(materialChanged()),
                   this->Connection, SIGNAL(Changed()));
}

QSet< cmbNucMaterial* > PinSubPart::getMaterials()
{
  QSet< cmbNucMaterial* > result;
  for(size_t at = 0; at < this->Materials.size(); at++)
  {
    result.insert(Materials[at]->getMaterial());
  }
  return result;
}

QSet< cmbNucMaterial* > PinSubPart::getOuterMaterials(QPointer<cmbNucMaterial> blMat)
{
  QSet< cmbNucMaterial* > result;
  for(std::vector< cmbNucMaterialLayer * >::const_reverse_iterator i = this->Materials.rbegin();
      i != this->Materials.rend(); ++i)
  {
    QPointer<cmbNucMaterial> mat = (*i)->getMaterial();
    if(mat != blMat)
    {
      result.insert(mat);
      break;
    }
  }
  return result;
}

namespace
{

template<class TYPE> bool setIfDifferent(TYPE const& a, TYPE & b)
{
  if(a != b)
  {
    b = a;
    return true;
  }
  else
  {
    return false;
  }
}

}

bool PinSubPart::fill(PinSubPart const* other)
{
  bool changed = false;
  changed |= setIfDifferent(other->x, this->x);
  changed |= setIfDifferent(other->y, this->y);
  changed |= setIfDifferent(other->z1, this->z1);
  changed |= setIfDifferent(other->z2, this->z2);

  if( other->Materials.size() != this->Materials.size() )
  {
    changed = true;
    this->SetNumberOfLayers(static_cast<int>(other->Materials.size()));
  }

  for(unsigned int i = 0; i < this->Materials.size(); ++i)
  {
    changed |= setIfDifferent(other->Materials[i]->getThickness()[0],
                              this->Materials[i]->getThickness()[0]);
    changed |= setIfDifferent(other->Materials[i]->getThickness()[1],
                              this->Materials[i]->getThickness()[1]);
    //changing material will send it own signal if different
    this->Materials[i]->changeMaterial(other->Materials[i]->getMaterial());
  }

  double otherRs[] = {other->getRadius(PinSubPart::BOTTOM),
                      other->getRadius(PinSubPart::TOP)};
  double thisRs[] = {this->getRadius(PinSubPart::BOTTOM),
                     this->getRadius(PinSubPart::TOP)};

  changed |= setIfDifferent(otherRs[0], thisRs[0]);
  changed |= setIfDifferent(otherRs[1], thisRs[1]);

  this->setRadius(PinSubPart::BOTTOM, thisRs[0]);
  this->setRadius(PinSubPart::TOP, thisRs[1]);

  return changed;
}

bool PinSubPart::equal(PinSubPart const* other) const
{
  if(this->GetType() != other->GetType()) return false;
  if(other->x != this->x) return false;
  if(other->y != this->y) return false;
  if(other->z1 != this->z1) return false;
  if(other->z2 != this->z2) return false;
  if(other->Materials.size() != this->Materials.size()) return false;
  for(unsigned int i = 0; i < this->Materials.size(); ++i)
  {
    if(!(*(other->Materials[i]) == *(this->Materials[i]))) return false;
  }

  return other->getRadius(PinSubPart::BOTTOM) == this->getRadius(PinSubPart::BOTTOM) &&
         other->getRadius(PinSubPart::TOP) == this->getRadius(PinSubPart::TOP);
}

//*********************************************************//

Cylinder::Cylinder(double rin, double z1in, double z2in)
: PinSubPart(z1in, z2in)
{
  r=rin;
}

Cylinder::Cylinder(PinSubPart const* other)
: PinSubPart(0, 10)
{
  r=1;
  this->fill(other);
}

enumNucPartsType Cylinder::GetType() const
{ return CMBNUC_ASSY_CYLINDER_PIN;}

bool Cylinder::operator==(const Cylinder& obj)
{
  return this->x==obj.x && this->y==obj.y &&
  this->z1==obj.z1 && this->z2==obj.z2 &&
  this->r==obj.r;
}

double Cylinder::getNormalizedThickness(int layer)
{
  return this->Materials[layer]->getThickness()[0];
}

void Cylinder::setNormalizedThickness(int layer, double t)
{
  double * thick = this->Materials[layer]->getThickness();
  thick[0] = thick[1] = t;
}

double Cylinder::getRadius(int layer)
{
  return this->Materials[layer]->getThickness()[0]*this->r;
}

PinSubPart * Cylinder::clone() const
{
  PinSubPart * result = new Cylinder(r,z1,z2);
  result->fill(this);
  return result;
}

std::vector<PinSubPart *> Cylinder
::split( std::vector<double>::const_iterator b,
         std::vector<double>::const_iterator end)
{
  std::vector<PinSubPart *> result;
  assert(*b == this->z1);
  for(std::vector<double>::const_iterator iter = b; iter != end; ++iter)
  {
    if(iter + 1 == end)
    {
      assert(*iter == this->z2);
      break;
    }
    Cylinder * c = new Cylinder(this);
    c->z1 = *iter;
    c->z2 = *(iter+1);
    result.push_back(c);
  }
  return result;
}

//*********************************************************//

Frustum::Frustum(double const* rin,
                 double z1in, double z2in)
: PinSubPart(z1in, z2in)
{
  r[TOP]=rin[TOP]; r[BOTTOM] = rin[BOTTOM];
}

Frustum::Frustum(PinSubPart const* other)
: PinSubPart(0, 1)
{
  r[0]= r[1] = 1.0;
  this->fill(other);
}

enumNucPartsType Frustum::GetType() const
{ return CMBNUC_ASSY_FRUSTUM_PIN;}

bool Frustum::operator==(const Frustum& obj)
{
  return this->x==obj.x && this->y==obj.y &&
  this->z1==obj.z1 && this->z2==obj.z2 &&
  this->r[TOP]==obj.r[TOP] && this->r[BOTTOM]==obj.r[BOTTOM];
}

double Frustum::getNormalizedThickness(int layer, Frustum::End end)
{
  return this->Materials[layer]->getThickness()[end];
}

void Frustum::setNormalizedThickness(int layer, Frustum::End end, double t)
{
  double * thick = this->Materials[layer]->getThickness();
  thick[end] = t;
}

double Frustum::getRadius(int layer, Frustum::End end)
{
  return this->Materials[layer]->getThickness()[end]*this->r[end];
}

PinSubPart * Frustum::clone() const
{
  PinSubPart * result = new Frustum(r, z1, z2);
  result->fill(this);
  return result;
}

std::vector<PinSubPart *> Frustum
::split( std::vector<double>::const_iterator b,
         std::vector<double>::const_iterator end)
{
  std::vector<PinSubPart *> result;
  assert(*b == this->z1);
  double previousR = -1;
  for(std::vector<double>::const_iterator iter = b; iter != end; ++iter)
  {
    if(iter + 1 == end)
    {
      assert(*iter == this->z2);
      break;
    }
    Frustum * c = new Frustum(this);
    c->z1 = *iter;
    c->z2 = *(iter+1);
    if(!result.empty())
    {
      c->r[0] = previousR;//(*(result.rbegin()))->r[1];
    }
    double t = (this->z2 - c->z2)/(this->z2 - this->z1);
    previousR = c->r[1] = t * this->r[0] + (1-t)*this->r[1];
    result.push_back(c);
  }
  return result;
}

//*********************************************************//

PinCell::PinCell()
{
  Name=Label="p1";
  legendColor = Qt::white;
  cutaway = false;
  Connection = new PinConnection();
  Connection->pc = this;
  //When cellmaterial is unkown, it is not used, so dec unknown by one
  this->CellMaterial.getMaterial()->dec();
}

PinCell::~PinCell()
{
  this->Connection->EmitDeleted(this);
  this->deleteObjs(this->Parts);
  delete Connection;
}

enumNucPartsType PinCell::GetType() const
{ return CMBNUC_ASSY_PINCELL;}

void PinCell::RemoveSection(AssyPartObj* obj)
{
  if(!obj)
  {
    return;
  }
  this->removeObj(static_cast<PinSubPart*>(obj), this->Parts);
}

double PinCell::Radius(int idx) const
{
  if(this->Parts.empty()) return 1;
  return this->Parts[0]->getNormalizedThickness(idx, Frustum::TOP);
}

QPointer<cmbNucMaterial> PinCell::Material(int layer)
{
  if(this->Parts.empty())
    return cmbNucMaterialColors::instance()->getUnknownMaterial();
  return (*(this->Parts.begin()))->GetMaterial(layer);
}

void PinCell::SetRadius(int idx, double radius)
{
  for(size_t i = 0; i < this->Parts.size(); i++){
    this->Parts[i]->setNormalizedThickness(idx, Frustum::TOP, radius);
    this->Parts[i]->setNormalizedThickness(idx, Frustum::BOTTOM, radius);
  }
}

void PinCell::SetMaterial(int idx, QPointer<cmbNucMaterial> material)
{
  for(size_t i = 0; i < this->Parts.size(); i++){
    this->Parts[i]->SetMaterial(idx, material);
  }
}

QColor PinCell::GetLegendColor() const
{
  return this->legendColor;
}

void PinCell::SetLegendColor(const QColor& color)
{
  this->legendColor = color;
  this->legendColor.setAlpha(255);
}

int PinCell::GetNumberOfLayers()
{
  if(this->Parts.empty()) return 0;
  return static_cast<int>((*(this->Parts.begin()))->GetNumberOfLayers());
}

void PinCell::SetNumberOfLayers(int numLayers)
{
  for(size_t i = 0; i < this->Parts.size(); i++){
    this->Parts[i]->SetNumberOfLayers(numLayers);
  }
}

void PinCell::AddPart(PinSubPart * part)
{
  this->connectSubPart(part);
  this->Parts.push_back(part);
  this->sort();
}

void PinCell::connectSubPart(PinSubPart * part)
{
  QObject::connect(part->GetConnection(), SIGNAL(Changed()),
                   this->Connection, SIGNAL(Changed()));
}

PinSubPart* PinCell::GetPart(int i) const
{
  if(static_cast<size_t>(i) < this->Parts.size()) return this->Parts[i];
  return NULL;
}

size_t PinCell::GetNumberOfParts() const
{
  return this->Parts.size();
}

PinConnection* PinCell::GetConnection() const
{
  return Connection;
}

QSet< cmbNucMaterial* > PinCell::getMaterials()
{
  QSet< cmbNucMaterial* > result;
  for(size_t at = 0; at < this->Parts.size(); at++)
  {
    result.unite(Parts[at]->getMaterials());
  }
  if(this->cellMaterialSet())
  {
    result.insert(this->CellMaterial.getMaterial());
  }
  return result;
}

QSet< cmbNucMaterial* > PinCell
::getOuterMaterials(QPointer<cmbNucMaterial> blMat)
{
  QSet< cmbNucMaterial* > result;
  if(this->cellMaterialSet() && this->CellMaterial.getMaterial() != blMat)
  {
    result.insert(this->CellMaterial.getMaterial());
  }
  else
  {
    for(size_t at = 0; at < this->Parts.size(); at++)
    {
      result.unite(Parts[at]->getOuterMaterials(blMat));
    }
  }
  return result;
}

bool PinCell::fill(PinCell const* other)
{
  bool changed = false;
  changed |= setIfDifferent(other->Name, this->Name);
  changed |= setIfDifferent(other->Label, this->Label);
  if( other->CellMaterial.getMaterial() != this->CellMaterial.getMaterial())
  {
    changed = true;
    this->CellMaterial.changeMaterial(other->CellMaterial.getMaterial());
  }

  if(other->Parts.size() < this->Parts.size())
  {
    changed = true;
    size_t i = other->Parts.size();
    for(;i < this->Parts.size(); ++i)
    {
      delete this->Parts[i];
    }
    this->Parts.resize(other->Parts.size());
  }

  while (other->Parts.size() > this->Parts.size())
  {
    changed = true;
    this->AddPart(other->Parts[this->Parts.size()]->clone());
  }

  for(unsigned int i = 0; i < this->Parts.size(); ++i)
  {
    if(this->Parts[i]->GetType() == other->Parts[i]->GetType())
    {
      changed |= this->Parts[i]->fill(other->Parts[i]);
    }
    else if(!this->Parts[i]->equal(other->Parts[i]))
    {
      changed = true;
      delete this->Parts[i];
      PinSubPart * tmp = other->Parts[i]->clone();
      this->connectSubPart(tmp);
      this->Parts[i] = tmp;
    }
  }
  this->sort();

  return changed;
}

void PinCell::InsertLayer(int layer)
{
  if(layer < 0 && layer > GetNumberOfLayers()) return; //do nothing
  this->SetNumberOfLayers(GetNumberOfLayers()+1);
  for(int i = GetNumberOfLayers()-1; i > layer; --i)
  {
    SetRadius(i, Radius(i-1));
    SetMaterial(i, Material(i-1));
  }
  SetMaterial(layer, cmbNucMaterialColors::instance()->getUnknownMaterial());
  if(layer == GetNumberOfLayers()-1)
  {
    if(GetNumberOfLayers() >= 3)
    {
      SetRadius(layer-1, (Radius(layer-2)+1.0)*0.5);
    }
    else if(GetNumberOfLayers() == 2)
    {
      SetRadius(layer-1, 0.5);
    }
    SetRadius(layer, 1.0);
  }
  else if(layer == 0)
    SetRadius(layer, Radius(layer+1)*0.5);
  else
  {
    double r1 = Radius(layer-1);
    double r2 = Radius(layer+1);
    SetRadius(layer, (r1+r2)*0.5);
  }
}

void PinCell::DeleteLayer(int layer)
{
  if(layer < 0 && layer >= GetNumberOfLayers()) return; //do nothing
  for(int i = layer; i < GetNumberOfLayers()-1; ++i)
  {
    SetRadius(i, Radius(i+1));
    SetMaterial(i, Material(i+1));
  }
  this->SetNumberOfLayers(GetNumberOfLayers()-1);
  SetRadius(GetNumberOfLayers()-1, 1.0);
}

QPointer<cmbNucMaterial> PinCell::getCellMaterial()
{
  return CellMaterial.getMaterial();
}

void PinCell::setCellMaterial(QPointer<cmbNucMaterial> material)
{
  CellMaterial.changeMaterial(material);
  if(material == cmbNucMaterialColors::instance()->getUnknownMaterial())
  {
    //Unknown is not really used or visiable for
    //cell material, so decrease by one
    material->dec();
  }
}

bool PinCell::cellMaterialSet() const
{
  return this->CellMaterial.getMaterial() !=
  cmbNucMaterialColors::instance()->getUnknownMaterial();
}

vtkBoundingBox PinCell::computeBounds(bool isHex)
{
  double minZ = this->GetPart(0)->getZ1(), maxZ = this->GetPart(0)->getZ2();
  double maxRadius = std::max(this->GetPart(0)->getRadius(PinSubPart::BOTTOM),
                              this->GetPart(0)->getRadius(PinSubPart::TOP));
  for(int i = 1; i < static_cast<int>(this->GetNumberOfParts()); i++)
  {
    PinSubPart * part = this->GetPart(i);
    if(part->getZ1() < minZ) minZ = part->getZ1();
    if(part->getZ2() > maxZ) maxZ = part->getZ2();
    double tmp = std::max(this->GetPart(i)->getRadius(PinSubPart::BOTTOM),
                          this->GetPart(i)->getRadius(PinSubPart::TOP));
    if(tmp > maxRadius) maxRadius = tmp;
  }
  double x = maxRadius, y = maxRadius;
  if(cellMaterialSet())
  {
    double pitchX, pitchY;
    pitchX = pitchY = maxRadius * 2.5;
    if(isHex)
    {
      x = y = std::max(maxRadius,
                       pitchX*0.5/0.86602540378443864676372317075294);
    }
    else
    {
      x = std::max(maxRadius, pitchX*0.5);
      y = std::max(maxRadius, pitchY*0.5);
    }
  }
  return vtkBoundingBox(-x,x,-y,y,minZ,maxZ);
}

std::vector<double> PinCell::getPinLayers() const
{
  //TODO: Simplify this, we can now assume order
  std::set<double> unique_levels;
  for(int i = 0; i < static_cast<int>(this->GetNumberOfParts()); i++)
  {
    unique_levels.insert(this->GetPart(i)->getZ1());
    unique_levels.insert(this->GetPart(i)->getZ2());
  }
  std::vector<double> result;
  for(std::set<double>::const_iterator iter = unique_levels.begin();
      iter != unique_levels.end(); ++iter)
  {
    result.push_back(*iter);
  }
  std::sort(result.begin(), result.end());
  return result;
}

void PinCell::splitPin(std::vector<double> const& layers)
{
  //TODO: handel when the pin is not alligned with the top and bottom of the duct
  std::vector<PinSubPart*> newParts;
  for(unsigned int i = 0; i < Parts.size(); ++i)
  {
    PinSubPart * ati = this->Parts[i];
    double z1 = ati->getZ1(), z2 = ati->getZ2();
    std::vector<double>::const_iterator b = layers.begin();
    for(; b!= layers.end(); ++b)
    {
      if(*b == z1) break;
    }
    assert(b != layers.end());
    std::vector<double>::const_iterator e = b+1;
    for(; e!= layers.end(); ++e)
    {
      if(*e == z2) break;
    }
    assert(e != layers.end());
    std::vector<PinSubPart*> tmp = ati->split(b, e+1);
    newParts.insert(newParts.end(), tmp.begin(), tmp.end());
    delete ati;
    this->Parts[i] = NULL;
  }
  this->Parts.resize(0);
  for(unsigned int i = 0; i < newParts.size(); ++i)
  {
    this->AddPart(newParts[i]);
  }
}

void PinCell::setHeight(double nh)
{
  if(this->Parts.empty()) return;
  double z0 = this->GetPart(0)->getZ1();
  double oldH = (*(this->Parts.rbegin()))->getZ2() - z0;
  if(oldH == nh) return;
  for(unsigned int i = 0; i < this->GetNumberOfParts(); ++i)
  {
    PinSubPart* p = this->GetPart(i);
    double l = p->getZ2() - p->getZ1();
    p->setZ1(z0);
    z0 = z0 + l/oldH * nh;
    p->setZ2(z0);
  }
}

bool PinCell::operator==(PinCell const& other)
{
  if(other.Parts.size() != this->Parts.size()) return false;
  for(unsigned int i = 0; i < this->Parts.size(); ++i)
  {
    if(!this->Parts[i]->equal(other.Parts[i])) return false;
  }
  return other.CellMaterial == this->CellMaterial;
}

namespace
{
  bool sort_by_z1(const PinSubPart * a, const PinSubPart * b)
  {
    return a->getZ1() < b->getZ1();
  }
}

void PinCell::sort()
{
  std::sort(Parts.begin(), Parts.end(), sort_by_z1);
}

double PinCell::getZ0() const
{
  if(this->Parts.empty()) return 0;
  return (*(this->Parts.begin()))->getZ1();
}

void PinCell::removeFakeBoundaryLayer(std::string blname)
{
  //For now only support the outer most
  int outer = GetNumberOfLayers()-1;
  QPointer<cmbNucMaterial> m = Material(outer);
  if(m != NULL && m->getLabel().toStdString() == blname)
  {
    for(unsigned int i = 0; i < this->GetNumberOfParts(); ++i)
    {
      PinSubPart* p = this->GetPart(i);
      double br = p->getRadius( outer-1, PinSubPart::BOTTOM);
      double tr = p->getRadius( outer-1, PinSubPart::TOP);
      p->setRadius(PinSubPart::BOTTOM, br);
      p->setRadius(PinSubPart::TOP, tr);
    }

    double r = Radius(outer-1);
    for(int i = 0; i < GetNumberOfLayers(); ++i)
    {
      SetRadius(i, Radius(i)/r);
    }
    this->SetNumberOfLayers(GetNumberOfLayers()-1);
  }
}
