#include "cmbNucDuctCell.h"
#include "cmbNucMaterialColors.h"

#include <cmath>
#include <cassert>
#include <set>

void DuctConnection::sendChange()
{
  emit Changed();
}

void DuctConnection::sendDelete()
{
  emit Deleted();
}

/*******************************************************************************/

Duct::Duct(double height, double thickX, double thickY)
{
  Connection = new DuctConnection();
  x=0.0;y=0.0;z1=0.0;z2=height;
  SetNumberOfLayers(1);
  thickness[0] = thickX;
  thickness[1] = thickY;
}

Duct::Duct(Duct * previous, bool resize)
{
  Connection = new DuctConnection();
  x=previous->x;
  y=previous->y;
  double tz1 = previous->z1;
  double tz2 = previous->z2;
  double tl = previous->z2 - previous->z1;
  if(resize)
  {
    previous->z2 = tz1 + tl*0.5;
    z1=previous->z2;
    z2=tz2;
  }
  else
  {
    z1=tz1;
    z2=tz2;
  }
  this->SetNumberOfLayers(previous->NumberOfLayers());
  thickness[0] = previous->thickness[0];
  thickness[1] = previous->thickness[1];
  for(unsigned int i = 0; i < previous->NumberOfLayers(); ++i)
  {
    this->setMaterial(i, previous->getMaterial(i));
    double * prev =  previous->getNormThick(i);
    double * me = this->getNormThick(i);
    me[0] = prev[0];
    me[1] = prev[1];
  }
}

Duct::~Duct()
{
  delete Connection;
}

DuctConnection * Duct::GetConnection()
{
  return Connection;
}

enumNucPartsType Duct::GetType() const
{ return CMBNUC_ASSY_DUCT;}

double Duct::GetLayerThick(size_t layer, size_t t) const
{
  return Materials[layer].getThickness()[t] * this->thickness[t];
}

void Duct::insertLayer( int a )
{
  if(static_cast<size_t>(a) > this->Materials.size()) return; //Do nothing
  this->SetNumberOfLayers(this->Materials.size()+1);
  if(static_cast<size_t>(a) == this->Materials.size()-1)
  {
    this->getNormThick(a)[0] = 1.0;
    this->getNormThick(a)[1] = 1.0;
    if(this->Materials.size() > 1)
    {
      double other[2] = {0,0};
      if(this->Materials.size() > 2)
      {
        other[0] =this->getNormThick(a-2)[0];
        other[1] =this->getNormThick(a-2)[1];
      }
      this->getNormThick(a-1)[0] = (other[0]+1)*0.5;
      this->getNormThick(a-1)[1] = (other[1]+1)*0.5;
    }
    return;
  }
  for(int i = static_cast<int>(this->Materials.size())-1; i > a; --i)
  {
    this->setMaterial(i, this->getMaterial(i-1));
    double * ntat = getNormThick(i);
    double * ntatm1 = getNormThick(i-1);
    ntat[0] = ntatm1[0];
    ntat[1] = ntatm1[1];
  }
  double * r = this->getNormThick(a+1);
  double l[2] = {0.0,0.0};
  if(a != 0)
  {
    l[0] =this->getNormThick(a-1)[0];
    l[1] =this->getNormThick(a-1)[1];
  }
  this->getNormThick(a)[0] = (l[0]+r[0])*0.5;
  this->getNormThick(a)[1] = (l[1]+r[1])*0.5;
  this->setMaterial(a, cmbNucMaterialColors::instance()->getUnknownMaterial());
}

void Duct::removeLayer( int a )
{
  if(this->Materials.size() == 1) return; //Must have one
  if(static_cast<size_t>(a) >= this->Materials.size()) return; //outside range

  for(int i = a+1; i < static_cast<int>(this->Materials.size()); i++)
  {
    this->setMaterial(i-1, this->getMaterial(i));
    double * ntat = getNormThick(i-1);
    double * ntatm1 = getNormThick(i);
    ntat[0] = ntatm1[0];
    ntat[1] = ntatm1[1];
  }
  this->SetNumberOfLayers(this->Materials.size()-1);
  this->getNormThick(this->Materials.size()-1)[0] = 1.0;
  this->getNormThick(this->Materials.size()-1)[1] = 1.0;
}

bool Duct::operator==(const Duct& obj)
{
  return this->x==obj.x && this->y==obj.y &&
         this->z1==obj.z1 && this->z2==obj.z2 &&
         this->Materials==obj.Materials &&
         this->thickness[0]==obj.thickness[0] &&
         this->thickness[1]==obj.thickness[1];
}

void Duct::SetNumberOfLayers(int i)
{
  if(this->Materials.size() == static_cast<size_t>(i)) return;
  for(size_t at = 0; at < this->Materials.size(); at++)
  {
    QObject::disconnect(this->Materials[at].GetConnection(), SIGNAL(materialChanged()),
                        this->Connection, SIGNAL(Changed()));
  }
  Materials.resize(i);
  for(size_t at = 0; at < this->Materials.size(); at++)
  {
    QObject::connect( this->Materials[at].GetConnection(), SIGNAL(materialChanged()),
                      this->Connection, SIGNAL(Changed()) );
  }
}

size_t Duct::NumberOfLayers() const
{
  return Materials.size();
}

QPointer<cmbNucMaterial> Duct::getMaterial(int i)
{
  if(static_cast<size_t>(i) >= Materials.size()) return NULL;
  return Materials[i].getMaterial();
}

double* Duct::getNormThick(int i)
{
  if(static_cast<size_t>(i) >= this->Materials.size()) return NULL;
  return this->Materials[i].getThickness();
}

void Duct::setMaterial( int i, QPointer<cmbNucMaterial> mat )
{
  if(static_cast<size_t>(i) >= this->Materials.size()) return;
  this->Materials[i].changeMaterial(mat);
}

QSet< cmbNucMaterial* > Duct::getMaterials()
{
  QSet< cmbNucMaterial* > result;
  for( unsigned int i = 0; i < this->Materials.size(); ++i )
  {
    result.insert(this->Materials[i].getMaterial());
  }
  return result;
}

cmbNucMaterialLayer const& Duct::getMaterialLayer(int i) const
{
  return this->Materials[i];
}

void Duct::setMaterialLayer(int i, cmbNucMaterialLayer * ml)
{
  if(i >= this->Materials.size()) this->SetNumberOfLayers(i+1);
  Materials[i] = *ml;
  delete ml;
}

void Duct::splitMaterialLayer( std::vector<double> const& lx, std::vector<double> const& ly )
{
  assert(lx.size() == ly.size());
  if(lx.size() == this->Materials.size()) return; //nothing to do
  assert(lx.size() > this->Materials.size());
  std::vector< QPointer<cmbNucMaterial> > materials;
  int at = 0;
  for(unsigned int i = 0; i < lx.size(); ++i)
  {
    assert(at < this->Materials.size());
    assert(lx[i] <= getNormThick(at)[0]);
    if(lx[i] < getNormThick(at)[0])
    {
      materials.push_back(getMaterial(at));
    }
    else if(lx[i] == getNormThick(at)[0])
    {
      materials.push_back(getMaterial(at));
      at++;
    }
  }
  assert( materials.size() == lx.size());
  this->SetNumberOfLayers(lx.size());
  for(unsigned int i = 0; i < lx.size(); ++i)
  {
    this->setMaterial(i, materials[i]);
    double * ntat = getNormThick(i);
    ntat[0] = lx[i];
    ntat[1] = ly[i];
  }
}

/*******************************************************************************/

DuctCell::DuctCell()
{
  Connection = new DuctConnection();
  this->Label = "D1";
  this->Name = "Duct1";
  this->useCount = 0;
}

DuctCell::~DuctCell()
{
  Connection->sendDelete();
  this->deleteObjs(this->Ducts);
  delete Connection;
}

DuctConnection * DuctCell::GetConnection()
{
  return this->Connection;
}

enumNucPartsType DuctCell::GetType() const
{ return CMBNUC_ASSY_DUCTCELL;}

void DuctCell::RemoveDuct(Duct* duct, bool merge_prev)
{
  if(duct != NULL && this->Ducts.size() > 1)
  {
    unsigned int at = this->Ducts.size();
    for(unsigned int i = 0; i < this->Ducts.size(); ++i)
    {
      if(this->Ducts[i] == duct)
      { at = i; break; }
    }
    if( at == 0 )
    {
      this->Ducts[at+1]->z1 = duct->z1;
    }
    else if(at < this->Ducts.size())
    {
      if(merge_prev || at+1 == this->Ducts.size())
      {
        this->Ducts[at-1]->z2 = duct->z2;
      }
      else
      {
        this->Ducts[at+1]->z1 = duct->z1;
      }
    }
  }
  this->removeObj(duct, this->Ducts);
}

void DuctCell::AddDuct(Duct* duct)
{
  QObject::connect( duct->GetConnection(), SIGNAL(Changed()),
                    this->Connection, SIGNAL(Changed()) );
  this->Ducts.push_back(duct);
  this->sort();
  this->Connection->sendChange();
}

size_t DuctCell::numberOfDucts() const
{
  return this->Ducts.size();
}

Duct * DuctCell::getDuct(int i)
{
  if (this->Ducts.empty() || static_cast<size_t>(i) > this->Ducts.size()) return NULL;
  return this->Ducts[i];
}

void DuctCell::fill(DuctCell* other)
{
  this->Label = other->Label;
  this->Name = other->Name;
  for(unsigned int i = 0; i < this->Ducts.size(); ++i)
  {
    delete(this->Ducts[i]);
  }
  this->Ducts.resize(0);
  for(unsigned int i = 0; i < other->Ducts.size(); ++i)
  {
    Duct * tmp = new Duct(other->Ducts[i], false);
    this->AddDuct(tmp);
  }
}

QSet< cmbNucMaterial* > DuctCell::getMaterials()
{
  QSet< cmbNucMaterial* > result;
  for(unsigned int i = 0; i < Ducts.size(); ++i)
  {
    result.unite(Ducts[i]->getMaterials());
  }
  return result;
}

bool DuctCell::GetInnerDuctSize(double & x, double & y)
{
  x = -1, y = -1;
  for (unsigned int i = 0; i < this->Ducts.size(); ++i)
  {
    Duct * duct = Ducts[i];
    double tx = duct->GetLayerThick(0, 0), ty = duct->GetLayerThick(0, 1);
    if(x == -1 || x > tx) x = tx;
    if(y == -1 || y > ty) y = ty;
  }
  return x != -1 && y != -1;
}

Duct * DuctCell::getPrevious()
{
  if(Ducts.empty()) return NULL;
  return *Ducts.rbegin();
}

double DuctCell::getLength()
{
  if(this->Ducts.size() == 0) return 10;
  double z1 = this->Ducts[0]->z1;
  double z2 = this->Ducts[0]->z2;
  for (unsigned int i = 0; i < this->Ducts.size(); ++i)
  {
    Duct * duct = Ducts[i];
    if(duct->z1<z1) z1 = duct->z1;
    if(duct->z2>z2) z2 = duct->z2;
  }
  return z2 - z1;
}

void DuctCell::setLength(double l)
{
  if(this->Ducts.size() == 0) return;
  double z1 = this->Ducts[0]->z1;
  double z2 = this->Ducts[0]->z2;
  for (unsigned int i = 1; i < this->Ducts.size(); ++i)
  {
    Duct * duct = Ducts[i];
    if(duct->z1<z1) z1 = duct->z1;
    if(duct->z2>z2) z2 = duct->z2;
  }
  double prevL = z2 - z1;
  for (unsigned int i = 0; i < this->Ducts.size(); ++i)
  {
    Duct * duct = Ducts[i];
    if(duct->z1==z1) duct->z1 = 0;
    else duct->z1 = (duct->z1  - z1) / prevL * l;
    if(duct->z2 == z2) duct->z2 = l;
    else duct->z2 = (duct->z2 - z1) / prevL * l;
  }
}

void DuctCell::getZRange(double & z1, double & z2)
{
  Duct * d = *(this->Ducts.begin());
  z1 = d->z1;
  z2 = z1 + getLength();
}

vtkBoundingBox DuctCell::computeBounds(bool hex)
{
  Duct * d = *(this->Ducts.begin());
  double z1 = d->z1;
  double * thickness = d->thickness;
  if(!hex)
  {
    double t[] = { thickness[0]*0.5 , thickness[1]*0.5};
    return vtkBoundingBox(-t[0], t[0], -t[1], t[1], z1, z1 + getLength());
  }
  double t = thickness[0]*0.5 / 0.86602540378443864676372317075294;
  return vtkBoundingBox(-t, t, -t, t, z1, z1 + getLength());
}

bool ductComp(Duct* i,Duct* j) { return (i->getZ1() < j->getZ1()); }

void DuctCell::sort()
{
  std::sort(this->Ducts.begin(), this->Ducts.end(), ductComp);
}

bool DuctCell::operator==(const DuctCell& obj)
{
  if(this->Ducts.size() != obj.Ducts.size()) return false;
  for (unsigned int i = 0; i < this->Ducts.size(); ++i)
  {
    Duct * d1 = this->Ducts[i];
    Duct * d2 = obj.Ducts[i];
    if(! (*d1 == *d2) ) return false;
  }
  return this->Name == obj.Name;
}

bool DuctCell::setDuctThickness(double t1, double t2)
{
  bool change = false;
  for(unsigned int i = 0; i < this->numberOfDucts(); ++i)
  {
    Duct * duct = this->getDuct(i);
    change |= t1 != duct->thickness[0];
    duct->thickness[0] = t1;
    change |= t2 != duct->thickness[1];
    duct->thickness[1] = t2;
  }
  return change;
}

bool DuctCell::isUsed()
{
  return this->useCount != 0;
}

void DuctCell::used()
{
  this->useCount++;
}

void DuctCell::freed()
{
  if(this->useCount > 0)
    this->useCount--;
}

std::vector<double> DuctCell::getDuctLayers() const
{
  std::vector<double> result;
  result.push_back(this->Ducts[0]->z1);
  for(unsigned int i = 0; i < this->Ducts.size(); ++i)
  {
    result.push_back(this->Ducts[i]->z2);
  }
  return result;
}

void DuctCell::uniformizeMaterialLayers()
{
  std::set<double> slayersX, slayersY;
  for(unsigned int i = 0; i < this->Ducts.size(); ++i)
  {
    Duct * d = this->Ducts[i];
    for(unsigned int l = 0; l < d->NumberOfLayers(); ++l)
    {
      slayersX.insert(d->getNormThick(l)[0]);
      slayersY.insert(d->getNormThick(l)[1]);
    }
  }
  std::vector<double> layersX, layersY;
  std::set<double>::const_iterator iX = slayersX.begin();
  std::set<double>::const_iterator iY = slayersY.begin();

  for(; iX != slayersX.end(); ++iX, ++iY)
  {
    layersX.push_back(*iX);
    layersY.push_back(*iY);
  }
  for(unsigned int i = 0; i < this->Ducts.size(); ++i)
  {
    this->Ducts[i]->splitMaterialLayer(layersX, layersY);
  }
}

void DuctCell::splitDucts( std::vector<double> const& layers )
{
  std::vector<Duct*> addedDucts;
  int ductLoc = 0;
  int layersStart = 0;
  //find the the bottom
  double z1 = this->Ducts[0]->z1;
  for(;layersStart < layers.size(); ++layersStart)
  {
    if(layers[layersStart] == z1) break;
  }
  assert(layersStart != layers.size());
  for(int i = 0; i < layersStart; ++i)
  {
    Duct * d = new Duct(this->Ducts[0], false);
    d->setZ1(layers[i]);
    d->setZ2(layers[i+1]);
    for( unsigned int j = 0; j < d->NumberOfLayers(); ++j)
    {
      d->setMaterial(j, cmbNucMaterialColors::instance()->getMaterialByName("gap"));
    }
    addedDucts.push_back(d);
  }
  for(unsigned int i = 0; i < this->Ducts.size(); ++i)
  {
    double z2 = this->Ducts[i]->z2;
    int layerEnd = layersStart + 1;
    for(;layerEnd < layers.size(); ++layerEnd)
    {
      if(layers[layerEnd] == z2) break;
    }
    assert(layerEnd != layers.size());
    if( layerEnd != layersStart + 1)
    {
      this->Ducts[i]->setZ2(layers[layersStart + 1]);
      for(unsigned int j = layersStart + 1; j < layerEnd; ++j)
      {
        Duct * d = new Duct(this->Ducts[i], false);
        d->setZ1(layers[j]);
        d->setZ2(layers[j+1]);
        addedDucts.push_back(d);
      }
    }
    layersStart = layerEnd;
  }
  for(int i = layersStart; i < layers.size()-1; ++i)
  {
    Duct * d = new Duct(this->Ducts[0], false);
    d->setZ1(layers[i]);
    d->setZ2(layers[i+1]);
    for( unsigned int j = 0; j < d->NumberOfLayers(); ++j)
    {
      d->setMaterial(j, cmbNucMaterialColors::instance()->getMaterialByName("gap"));
    }
    addedDucts.push_back(d);
  }
  for(unsigned int i = 0; i < addedDucts.size(); ++i)
  {
    this->AddDuct(addedDucts[i]);
  }
}
