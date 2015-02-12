#include "cmbNucPinCell.h"
#include "cmbNucMaterialColors.h"

#include <cassert>
#include <cmath>
#include <algorithm>
#include <set>

#include <QDebug>

void PinConnection::clearOldData()
{
  if(pc)
  {
    pc->CachedData = NULL;
    emit CellMaterialChanged();
  }
}

PinSubPart::PinSubPart(double z1in, double z2in) : Materials(1, new cmbNucMaterialLayer())
{
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
  if(i >= this->Materials.size()) this->SetNumberOfLayers(i+1);
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
    this->SetNumberOfLayers(other->Materials.size());
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

std::vector<Cylinder *> Cylinder
::split( std::vector<double>::const_iterator b,
         std::vector<double>::const_iterator end)
{
  std::vector<Cylinder *> result;
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

std::vector<Frustum *> Frustum
::split( std::vector<double>::const_iterator b,
        std::vector<double>::const_iterator end)
{
  std::vector<Frustum *> result;
  assert(*b == this->z1);
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
      c->r[0] = (*(result.rbegin()))->r[1];
    }
    double t = (this->z2 - c->z2)/(this->z2 - this->z1);
    c->r[1] = t * this->r[0] + (1-t)*this->r[1];
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
  QObject::connect(CellMaterial.GetConnection(), SIGNAL(materialChanged()),
                   this->Connection, SLOT(clearOldData()));
  //When cellmaterial is unkown, it is not used, so dec unknown by one
  this->CellMaterial.getMaterial()->dec();
}

PinCell::~PinCell()
{
  this->Connection->EmitDeleted(this);
  this->deleteObjs(this->Cylinders);
  this->deleteObjs(this->Frustums);
  delete Connection;
}

enumNucPartsType PinCell::GetType() const
{ return CMBNUC_ASSY_PINCELL;}

int PinCell::NumberOfSections() const
{ return this->Cylinders.size() + this->Frustums.size();}

void PinCell::RemoveSection(AssyPartObj* obj)
{
  if(!obj)
  {
    return;
  }
  if(obj->GetType() == CMBNUC_ASSY_CYLINDER_PIN)
  {
    this->RemoveCylinder(dynamic_cast<Cylinder*>(obj));
  }
  else if(obj->GetType() == CMBNUC_ASSY_FRUSTUM_PIN)
  {
    this->RemoveFrustum(dynamic_cast<Frustum*>(obj));
  }
}

void PinCell::RemoveCylinder(Cylinder* cylinder)
{
  //Deleting should automatically remove connection
  this->removeObj(cylinder, this->Cylinders);
}

void PinCell::RemoveFrustum(Frustum* frustum)
{
  //Deleting should automatically remove connection
  this->removeObj(frustum, this->Frustums);
}

double PinCell::Radius(int idx) const
{
  if(!this->Cylinders.empty())
  {
    return this->Cylinders[0]->getNormalizedThickness(idx);
  }
  else if(!this->Frustums.empty())
  {
    return this->Frustums[0]->getNormalizedThickness(idx, Frustum::TOP);
  }
  return 1;
}

QPointer<cmbNucMaterial> PinCell::Material(int layer)
{
  if(!this->Cylinders.empty())
  {
    return this->Cylinders[0]->GetMaterial(layer);
  }
  else if(!this->Frustums.empty())
  {
    return this->Frustums[0]->GetMaterial(layer);
  }
  return cmbNucMaterialColors::instance()->getUnknownMaterial();
}

void PinCell::SetRadius(int idx, double radius)
{
  for(size_t i = 0; i < this->Cylinders.size(); i++){
    this->Cylinders[i]->setNormalizedThickness(idx, radius);
  }
  for(size_t i = 0; i < this->Frustums.size(); i++){
    this->Frustums[i]->setNormalizedThickness(idx, Frustum::TOP, radius);
    this->Frustums[i]->setNormalizedThickness(idx, Frustum::BOTTOM, radius);
  }
}

void PinCell::SetMaterial(int idx, QPointer<cmbNucMaterial> material)
{
  for(size_t i = 0; i < this->Cylinders.size(); i++){
    this->Cylinders[i]->SetMaterial(idx, material);
  }
  for(size_t i = 0; i < this->Frustums.size(); i++){
    this->Frustums[i]->SetMaterial(idx, material);
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
  if(this->Cylinders.size() > 0)
  {
    return this->Cylinders[0]->GetNumberOfLayers();
  }
  else if(this->Frustums.size() > 0)
  {
    return this->Frustums[0]->GetNumberOfLayers();
  }
  return 0;
}

void PinCell::SetNumberOfLayers(int numLayers)
{
  for(size_t i = 0; i < this->Cylinders.size(); i++){
    this->Cylinders[i]->SetNumberOfLayers(numLayers);
  }
  for(size_t i = 0; i < this->Frustums.size(); i++){
    this->Frustums[i]->SetNumberOfLayers(numLayers);
  }
}

void PinCell::AddCylinder(Cylinder* cylinder)
{
  QObject::connect(cylinder->GetConnection(), SIGNAL(Changed()),
                   this->Connection, SIGNAL(Changed()));
  this->Cylinders.push_back(cylinder);
}

void PinCell::AddFrustum(Frustum* frustum)
{
  QObject::connect(frustum->GetConnection(), SIGNAL(Changed()),
                   this->Connection, SIGNAL(Changed()));
  this->Frustums.push_back(frustum);
}

void PinCell::AddPart(PinSubPart * part)
{
  switch(part->GetType())
  {
    case CMBNUC_ASSY_CYLINDER_PIN:
      AddCylinder(dynamic_cast<Cylinder*>(part));
      break;
    case CMBNUC_ASSY_FRUSTUM_PIN:
      AddFrustum(dynamic_cast<Frustum*>(part));
      break;
    default:
      //DO NOTHING
      break;
  }
}

size_t PinCell::NumberOfCylinders() const
{
  return this->Cylinders.size();
}

size_t PinCell::NumberOfFrustums() const
{
  return this->Frustums.size();
}

Cylinder* PinCell::GetCylinder(int i) const
{
  if(static_cast<size_t>(i) < this->Cylinders.size()) return Cylinders[i];
  return NULL;
}

Frustum * PinCell::GetFrustum(int i) const
{
  if(static_cast<size_t>(i) < this->Frustums.size()) return Frustums[i];
  return NULL;
}

PinSubPart* PinCell::GetPart(int i) const
{
  if(static_cast<size_t>(i) < this->Cylinders.size()) return Cylinders[i];
  i = i - static_cast<int>(this->Cylinders.size());
  if(static_cast<size_t>(i) < this->Frustums.size()) return Frustums[i];
  return NULL;
}

size_t PinCell::GetNumberOfParts() const
{
  return this->Cylinders.size() + this->Frustums.size();
}

PinConnection* PinCell::GetConnection() const
{
  return Connection;
}

QSet< cmbNucMaterial* > PinCell::getMaterials()
{
  QSet< cmbNucMaterial* > result;
  for(size_t at = 0; at < this->Cylinders.size(); at++)
  {
    result.unite(Cylinders[at]->getMaterials());
  }
  for(size_t at = 0; at < this->Frustums.size(); at++)
  {
    result.unite(Frustums[at]->getMaterials());
  }
  if(this->cellMaterialSet())
  {
    result.insert(this->CellMaterial.getMaterial());
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

  if(other->Cylinders.size() < this->Cylinders.size())
  {
    changed = true;
    unsigned int i = other->Cylinders.size();
    for(;i < this->Cylinders.size(); ++i)
    {
      delete this->Cylinders[i];
    }
    this->Cylinders.resize(other->Cylinders.size());
  }
  while (other->Cylinders.size() > this->Cylinders.size())
  {
    changed = true;
    Cylinder * c = new Cylinder(0,0,0);
    this->AddCylinder(c);
  }
  for(unsigned int i = 0; i < this->Cylinders.size(); ++i)
  {
    changed |= this->Cylinders[i]->fill(other->Cylinders[i]);
  }

  if(other->Frustums.size() < this->Frustums.size())
  {
    changed = true;
    unsigned int i = other->Frustums.size();
    for(;i < this->Frustums.size(); ++i)
    {
      delete this->Frustums[i];
    }
    this->Frustums.resize(other->Frustums.size());
  }
  while (other->Frustums.size() > this->Frustums.size())
  {
    changed = true;
    double r[2] = {1,1};
    Frustum * f = new Frustum(r, 0, 0);
    this->AddFrustum(f);
  }
  for(unsigned int i = 0; i < this->Frustums.size(); ++i)
  {
    changed |= this->Frustums[i]->fill(other->Frustums[i]);
  }

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
  else if(layer == 0) SetRadius(layer, Radius(layer+1)*0.5);
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
    //Unknown is not really used or visiable for cell material, so decrease by one
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
  double minZ = this->GetPart(0)->z1, maxZ = this->GetPart(0)->z2;
  double maxRadius = std::max(this->GetPart(0)->getRadius(PinSubPart::BOTTOM),
                              this->GetPart(0)->getRadius(PinSubPart::TOP));
  for(int i = 1; i < static_cast<int>(this->GetNumberOfParts()); i++)
  {
    PinSubPart * part = this->GetPart(i);
    if(part->z1 < minZ) minZ = part->z1;
    if(part->z2 > maxZ) maxZ = part->z2;
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
      x = y = std::max(maxRadius, pitchX*0.5/0.86602540378443864676372317075294);
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
  std::set<double> unique_levels;
  for(int i = 0; i < static_cast<int>(this->GetNumberOfParts()); i++)
  {
    unique_levels.insert(this->GetPart(i)->z1);
    unique_levels.insert(this->GetPart(i)->z2);
  }
  std::vector<double> result;
  for(std::set<double>::const_iterator iter = unique_levels.begin(); iter != unique_levels.end(); ++iter)
  {
    result.push_back(*iter);
  }
  std::sort(result.begin(), result.end());
  return result;
}

void PinCell::splitPin(std::vector<double> const& layers)
{
  //TODO: handel when the pin is not alligned with the top and bottom of the duct
  std::vector<Cylinder*> c = this->Cylinders;
  this->Cylinders.clear();
  for(unsigned int i = 0; i < c.size(); ++i)
  {
    double z1 = c[i]->z1, z2 = c[i]->z2;
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
    std::vector<Cylinder*> tmp = c[i]->split(b, e+1);
    for(unsigned int k = 0; k < tmp.size(); ++k)
    {
      this->AddCylinder(tmp[k]);
    }
    delete c[i];
  }
  std::vector<Frustum*> f = this->Frustums;
  this->Frustums.clear();
  for(unsigned int i = 0; i < f.size(); ++i)
  {
    double z1 = f[i]->z1, z2 = f[i]->z2;
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
    std::vector<Frustum*> tmp = f[i]->split(b, e+1);
    for(unsigned int k = 0; k < tmp.size(); ++k)
    {
      this->AddFrustum(tmp[k]);
    }
    delete f[i];
  }
}
