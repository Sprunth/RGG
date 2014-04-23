#include "cmbNucDuctCell.h"

Duct::MaterialLayer::MaterialLayer()
: cmbNucMaterialLayer()
{
  this->normThickness[0] = 1;
  this->normThickness[1] = 1;
}

Duct::MaterialLayer::MaterialLayer(const MaterialLayer & ml)
: cmbNucMaterialLayer(ml)
{
  this->normThickness[0] = ml.normThickness[0];
  this->normThickness[1] = ml.normThickness[1];
}

bool Duct::MaterialLayer::operator==( const Duct::MaterialLayer & other ) const
{
  return Material == other.Material &&
  normThickness[0] == other.normThickness[0] &&
  normThickness[1] == other.normThickness[1];
}

/*******************************************************************************/

Duct::Duct(enumNucPartsType type)
{
  Connection = new DuctConnection();
  x=0.0;y=0.0;z1=0.0;z2=4.0;
  SetNumberOfLayers(1);
  thickness[0] = 18;
  thickness[1] = 18;
  enType = type;
}

Duct::~Duct()
{
  delete Connection;
}

DuctConnection * Duct::GetConnection()
{
  return Connection;
}

enumNucPartsType Duct::GetType()
{ return enType;}

void Duct::SetType(enumNucPartsType type)
{ enType = type;}

double Duct::GetLayerThick(size_t layer, size_t t) const
{
  return Materials[layer].normThickness[t] * this->thickness[t];
}

bool Duct::operator==(const Duct& obj)
{
  return this->x==obj.x && this->y==obj.y &&
  this->z1==obj.z1 && this->z2==obj.z2 &&
  this->Materials==obj.Materials &&
  this->thickness[0]==obj.thickness[0] &&
  this->thickness[1]==obj.thickness[1] &&
  this->enType == obj.enType;
}

void Duct::SetNumberOfLayers(int i)
{
  if(this->Materials.size() == i) return;
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
  if(i >= Materials.size()) return NULL;
  return Materials[i].getMaterial();
}

double* Duct::getNormThick(int i)
{
  if(i >= this->Materials.size()) return NULL;
  return this->Materials[i].normThickness;
}

void Duct::setMaterial( int i, QPointer<cmbNucMaterial> mat )
{
  if(i >= this->Materials.size()) return;
  this->Materials[i].changeMaterial(mat);
}

/*******************************************************************************/

DuctCell::DuctCell()
{
  Connection = new DuctConnection();
}

DuctCell::~DuctCell()
{
  this->deleteObjs(this->Ducts);
  delete Connection;
}

DuctConnection * DuctCell::GetConnection()
{
  return this->Connection;
}

enumNucPartsType DuctCell::GetType()
{ return CMBNUC_ASSY_DUCTCELL;}

void DuctCell::RemoveDuct(Duct* duct)
{
  this->removeObj(duct, this->Ducts);
}

void DuctCell::AddDuct(Duct* duct)
{
  QObject::connect( duct->GetConnection(), SIGNAL(Changed()),
                    this->Connection, SIGNAL(Changed()) );
  this->Ducts.push_back(duct);
}

size_t DuctCell::numberOfDucts() const
{
  return this->Ducts.size();
}

Duct * DuctCell::getDuct(int i)
{
  if (i > this->Ducts.size()) return NULL;
  return this->Ducts[i];
}
