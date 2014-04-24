#include "cmbNucPinCell.h"

PinSubPart::PinSubPart() : Materials(1)
{
  x=0.0; y=0.0; z1=0.0; z2=4.0;
  Connection = new PinConnection();
  this->setConnection(Materials[0]);
}

PinSubPart::~PinSubPart()
{
  delete Connection;
}

PinConnection* PinSubPart::GetConnection() const
{
  return Connection;
}

QPointer<cmbNucMaterial> PinSubPart::GetMaterial(int i)
{
  if(i>=0 && i<this->Materials.size())
  {
    return this->Materials[i].getMaterial();
  }
  return NULL;
}

void
PinSubPart::SetMaterial(int i,
                        QPointer<cmbNucMaterial> material)
{
  if(i>=0 && i<this->Materials.size())
  {
    this->Materials[i].changeMaterial( material );
  }
}

void PinSubPart::SetNumberOfLayers(int numLayers)
{
  if(this->Materials.size() == numLayers) return;
  //Disconnect because copy constructors could be called
  for(size_t at = 0; at < this->Materials.size(); at++)
  {
    QObject::disconnect(this->Materials[at].GetConnection(), SIGNAL(materialChanged()),
                        this->Connection, SIGNAL(Changed()));
  }
  this->Materials.resize(numLayers);
  for(size_t at = 0; at < this->Materials.size(); at++)
  {
    this->setConnection(this->Materials[at]);
  }
}

std::size_t PinSubPart::GetNumberOfLayers() const
{
  return Materials.size();
}

void PinSubPart::setConnection(cmbNucMaterialLayer & layer)
{
  QObject::connect(layer.GetConnection(), SIGNAL(materialChanged()),
                   this->Connection, SIGNAL(Changed()));
}

QSet< cmbNucMaterial* > PinSubPart::getMaterials()
{
  QSet< cmbNucMaterial* > result;
  for(size_t at = 0; at < this->Materials.size(); at++)
  {
    result.insert(Materials[at].getMaterial());
  }
  return result;
}

//*********************************************************//

Cylinder::Cylinder() : PinSubPart()
{
  r=1.6;
}

enumNucPartsType Cylinder::GetType()
{ return CMBNUC_ASSY_CYLINDER_PIN;}

bool Cylinder::operator==(const Cylinder& obj)
{
  return this->x==obj.x && this->y==obj.y &&
  this->z1==obj.z1 && this->z2==obj.z2 &&
  this->r==obj.r;
}

double Cylinder::getNormalizedThickness(int layer)
{
  return this->Materials[layer].getThickness()[0];
}

void Cylinder::Cylinder::setNormalizedThickness(int layer, double t)
{
  double * thick = this->Materials[layer].getThickness();
  thick[0] = thick[1] = t;
}

double Cylinder::getRadius(int layer)
{
  return this->Materials[layer].getThickness()[0]*this->r;
}

//*********************************************************//

Frustum::Frustum() : PinSubPart()
{
  r[TOP]=1.6; r[BOTTOM] = 1.4;
}

enumNucPartsType Frustum::GetType()
{ return CMBNUC_ASSY_FRUSTUM_PIN;}

bool Frustum::operator==(const Frustum& obj)
{
  return this->x==obj.x && this->y==obj.y &&
  this->z1==obj.z1 && this->z2==obj.z2 &&
  this->r[TOP]==obj.r[TOP] && this->r[BOTTOM]==obj.r[BOTTOM];
}

double Frustum::getNormalizedThickness(int layer, Frustum::End end)
{
  return this->Materials[layer].getThickness()[end];
}

void Frustum::setNormalizedThickness(int layer, Frustum::End end, double t)
{
  double * thick = this->Materials[layer].getThickness();
  thick[end] = t;
}

double Frustum::getRadius(int layer, Frustum::End end)
{
  return this->Materials[layer].getThickness()[end]*this->r[end];
}

//*********************************************************//

PinCell::PinCell()
{
  pitchX=0.0;
  pitchY=0.0;
  pitchZ=0.0;
  name=label="p1";
  legendColor = Qt::white;
  cutaway = false;
  Connection = new PinConnection();
}

PinCell::~PinCell()
{
  this->deleteObjs(this->Cylinders);
  this->deleteObjs(this->Frustums);
  delete Connection;
}

enumNucPartsType PinCell::GetType()
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
  if(i < this->Cylinders.size()) return Cylinders[i];
  return NULL;
}

Frustum * PinCell::GetFrustum(int i) const
{
  if(i < this->Frustums.size()) return Frustums[i];
  return NULL;
}

PinSubPart* PinCell::GetPart(int i) const
{
  if(i < this->Cylinders.size()) return Cylinders[i];
  i = i - this->Cylinders.size();
  if(i < this->Frustums.size()) return Frustums[i];
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
  return result;
}
