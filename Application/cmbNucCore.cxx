
#include "cmbNucCore.h"
#include "inpFileIO.h"

#include <iostream>
#include <algorithm>
#include <fstream>
#include <limits>
#include <set>
#include <cmath>

#include "cmbNucPinLibrary.h"
#include "cmbNucDuctLibrary.h"
#include "cmbNucAssembly.h"
#include "cmbNucAssemblyLink.h"

#include "vtkTransform.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkTransformFilter.h"
#include "vtkMath.h"
#include "vtkPolyData.h"
#include "cmbNucDefaults.h"
#include "cmbNucMaterialColors.h"
#include "vtkBoundingBox.h"

#include "vtkCmbLayeredConeSource.h"

#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include <QDebug>

void cmbNucCoreConnection::dataChanged()
{
  v->setAndTestDiffFromFiles(true);
  emit dataChangedSig();
}

void cmbNucCoreConnection::justFileChanged()
{
  v->fileChanged();
  emit dataChangedSig();
}

void cmbNucCoreConnection::assemblyChanged()
{
  v->setAndTestDiffFromFiles(true);
  emit dataChangedSig();
}

const double cmbNucCore::CosSinAngles[6][2] =
{ { cos( 2*(vtkMath::Pi() / 6.0) * (5 + 3) ),
    sin( 2*(vtkMath::Pi() / 6.0) * (5 + 3) ) },
  { cos( 2*(vtkMath::Pi() / 6.0) * (4 + 3) ),
    sin( 2*(vtkMath::Pi() / 6.0) * (4 + 3) ) },
  { cos( 2*(vtkMath::Pi() / 6.0) * (3 + 3) ),
    sin( 2*(vtkMath::Pi() / 6.0) * (3 + 3) ) },
  { cos( 2*(vtkMath::Pi() / 6.0) * (2 + 3) ),
    sin( 2*(vtkMath::Pi() / 6.0) * (2 + 3) ) },
  { cos( 2*(vtkMath::Pi() / 6.0) * (1 + 3) ),
    sin( 2*(vtkMath::Pi() / 6.0) * (1 + 3) ) },
  { cos( 2*(vtkMath::Pi() / 6.0) * (0 + 3) ),
    sin( 2*(vtkMath::Pi() / 6.0) * (0 + 3) ) } };

cmbNucCore::cmbNucCore(bool needSaved)
{
  this->PinLibrary = new cmbNucPinLibrary;
  this->DuctLibrary = new cmbNucDuctLibrary;
  this->AssemblyPitchX = this->AssemblyPitchY = 23.5;
  this->HexSymmetry = 1;
  DifferentFromFile = needSaved;
  DifferentFromH5M = true;
  DifferentFromGenBoundaryLayer = true;
  this->Connection = new cmbNucCoreConnection();
  this->Connection->v = this;
  hasCylinder = false;
  cylinderRadius = 0;
  cylinderOuterSpacing = 0;

  QObject::connect(this->PinLibrary->GetConnection(), SIGNAL(libraryChanged()),
                   this->Connection, SLOT(justFileChanged()));
  QObject::connect(this->DuctLibrary->GetConnection(), SIGNAL(libraryChanged()),
                   this->Connection, SLOT(justFileChanged()));
}

cmbNucCore::~cmbNucCore()
{
  for(std::vector< cmbNucAssemblyLink* >::iterator fit = this->AssemblyLinks.begin();
      fit != this->AssemblyLinks.end(); ++fit)
  {
    delete *fit;
  }
  this->AssemblyLinks.clear();

  for(std::vector<cmbNucAssembly*>::iterator fit=this->Assemblies.begin();
      fit!=this->Assemblies.end(); ++fit)
  {
    if(*fit)
    {
      delete *fit;
    }
  }
  this->Assemblies.clear();
  this->clearBoundaryLayer();
  delete this->Defaults;
  delete this->PinLibrary;
  delete this->DuctLibrary;
  delete this->Connection;
}

vtkBoundingBox cmbNucCore::computeBounds()
{
  if(this->Assemblies.size()==0) return vtkBoundingBox();
  double z1, z2;
  this->Assemblies[0]->getZRange(z1,z2);
  double wh[2];
  this->Assemblies[0]->GetDuctWidthHeight(wh);
  std::pair<int, int> dim = lattice.GetDimensions();
  if(IsHexType())
  {
    double diameter = wh[0]*(dim.first*2-1);
    double radius = diameter*0.5;
    double pointDist = wh[0]*0.5/0.86602540378443864676372317075294;
    double tmpH = (dim.first + std::floor((dim.first-1)*0.5))*pointDist;

    int subType = lattice.GetGeometrySubType();
    double tx = 0, ty = 0;
    double min[2], max[2];
    if((subType & ANGLE_360) && dim.first>=1)
    {
      tx = wh[0]*dim.first;
      double tmp = tx - wh[0];
      double t2 = tmp*0.5;
      ty = -std::sqrt(tmp*tmp-t2*t2);
      double r = std::max(tmpH,radius);
      min[0] = min[1] = -r;
      max[0] = max[1] = r;
    }
    else if((subType & ANGLE_60) && (subType & VERTEX))
    {
      min[0] = 0;
      min[1] = 0;
      max[0] = tmpH;
      max[1] = 0.86602540378443864676372317075294*tmpH;
    }
    else if((subType & ANGLE_60))
    {
      min[0] = 0;
      min[1] = 0;
      max[0] = radius;
      max[1] = 0.86602540378443864676372317075294*radius;
    }
    else
    {
      min[0] = 0;
      min[1] = 0;
      max[0] = radius;
      max[1] = radius;
    }
    if(getHasCylinder())
    {
      min[0] = std::min(min[0], -this->cylinderRadius);
      min[1] = std::min(min[1], -this->cylinderRadius);
      max[0] = std::max(max[0], this->cylinderRadius);
      max[1] = std::max(max[1], this->cylinderRadius);
    }
    return vtkBoundingBox(tx+min[0], tx+max[0],
                          ty+min[1], ty+max[1],
                          z1, z2);
  }
  else
  {
    vtkBoundingBox b;
    double transX = this->Assemblies[0]->getAssyDuct().getDuct(0)->getX();
    double transY = this->Assemblies[0]->getAssyDuct().getDuct(0)->getY();
    double pt[4];
    calculateRectPt( 0, 0, pt );
    calculateRectPt(dim.first-1, dim.second-1, pt+2);
    pt[0] -= wh[0]*0.5;
    pt[1] -= wh[1]*0.5;
    pt[2] += wh[0]*0.5;
    pt[3] += wh[1]*0.5;
    double cp[] = {(pt[2] + pt[0])*0.5,(pt[3] + pt[1])*0.5};
    double w = pt[2] - pt[0];
    double h = pt[3] - pt[1];
    if(getHasCylinder())
    {
      w = std::max(w, 2*this->cylinderRadius);
      h = std::max(h, 2*this->cylinderRadius);
    }
    return vtkBoundingBox(transX+cp[0]-w*0.5, transX+cp[0]+w*0.5,
                          transY+cp[1]-h*0.5, transY+cp[1]+h*0.5,
                          z1, z2);
  }
}

void cmbNucCore::SetDimensions(int i, int j)
{
  this->lattice.SetDimensions(i, j);
}

void cmbNucCore::clearExceptAssembliesAndGeom()
{
  this->lattice.SetDimensions(1, 1, true);
  this->setAndTestDiffFromFiles(true);
  CurrentFileName = "";
  ExportFileName = "";
  meshFilePrefix = "";
  meshFileExtention = "";
  Params.clear();
}

bool cmbNucCore::AddAssemblyLink(cmbNucAssemblyLink * assemblyLink)
{
  if(!assemblyLink) return false;
  if(this->Assemblies.empty()) return false;
  if(!assemblyLink->isValid()) return false;
  cmbNucAssembly *a1 = assemblyLink->getLink();
  cmbNucAssembly *a2 = this->GetAssembly(a1->getLabel());
  if(a2 == NULL) return false;
  if(a2 != a1)
  {
    assemblyLink->setLink(a2);
  }
  AssemblyLinks.push_back(assemblyLink);
  return true;
}

void cmbNucCore::AddAssembly(cmbNucAssembly *assembly)
{
  if(!assembly) return;
  if(this->Assemblies.size()==0)
  {
    if(assembly)
    {
      this->lattice.SetGeometryType(
        assembly->getLattice().GetGeometryType());
    }
    this->SetDimensions(1, 1);
  }
  this->Assemblies.push_back(assembly);
  assembly->setPinLibrary(this->PinLibrary);
  assembly->setDuctLibrary(this->DuctLibrary);
  if( this->getLattice().GetGeometrySubType() & ANGLE_60 &&
      this->getLattice().GetGeometrySubType() & VERTEX )
  {
    assembly->getLattice().setFullCellMode(Lattice::HEX_FULL);
  }
  else
  {
    assembly->getLattice().setFullCellMode(Lattice::HEX_FULL_30);
  }
  QObject::connect(assembly->GetConnection(), SIGNAL(dataChangedSig()),
                   this->Connection, SIGNAL(dataChangedSig()));
  QObject::connect(assembly->GetConnection(), SIGNAL(colorChanged()),
                   this->Connection, SIGNAL(colorChanged()));
  QObject::connect(assembly->GetConnection(), SIGNAL(dataChangedSig()),
                   this->Connection, SLOT(assemblyChanged()));
  if(this->Assemblies.size() == 1)
  {
    this->SetAssemblyLabel(0, 0, assembly->getLabel(), assembly->GetLegendColor());
  }
  // the new assembly need to be in the grid
}

void cmbNucCore::RemoveAssembly(const std::string &label)
{
  //remove links
  for(size_t i = 0; i < this->AssemblyLinks.size();)
  {
    if(this->AssemblyLinks[i]->getLink()->getLabel() == label)
    {
      delete this->AssemblyLinks[i];
      this->AssemblyLinks.erase(this->AssemblyLinks.begin() + i);
      this->fileChanged();
    }
    else
    {
      ++i;
    }
  }
  //remove assy
  for(size_t i = 0; i < this->Assemblies.size(); i++)
  {
    if(this->Assemblies[i]->getLabel() == label)
    {
      delete this->Assemblies[i];
      this->Assemblies.erase(this->Assemblies.begin() + i);
      this->fileChanged();
      break;
    }
  }
  // update the Grid
  if(this->lattice.ClearCell(label))
  {
    this->setAndTestDiffFromFiles(true);
  }
}

void cmbNucCore::RemoveAssemblyLink(const std::string &label)
{
  for(size_t i = 0; i < this->AssemblyLinks.size(); i++)
  {
    if(this->AssemblyLinks[i]->getLabel() == label)
    {
      delete this->AssemblyLinks[i];
      this->AssemblyLinks.erase(this->AssemblyLinks.begin() + i);
      this->fileChanged();
      break;
    }
  }
  // update the Grid
  if(this->lattice.ClearCell(label))
  {
    this->setAndTestDiffFromFiles(true);
  }
}

cmbNucAssembly* cmbNucCore::GetAssembly(const std::string &label)
{
  for(size_t i = 0; i < this->Assemblies.size(); i++)
  {
    if(this->Assemblies[i]->getLabel() == label)
    {
      return this->Assemblies[i];
    }
  }

  return NULL;
}

cmbNucAssembly* cmbNucCore::GetAssembly(int idx)
{
  return idx< static_cast<int>(this->Assemblies.size()) ? this->Assemblies[idx] : NULL;
}

cmbNucAssemblyLink* cmbNucCore::GetAssemblyLink(const std::string &label)
{
  for(size_t i = 0; i < this->AssemblyLinks.size(); i++)
  {
    if(this->AssemblyLinks[i]->getLabel() == label)
    {
      return this->AssemblyLinks[i];
    }
  }

  return NULL;
}

cmbNucAssemblyLink* cmbNucCore::GetAssemblyLink(int idx)
{
  return idx< static_cast<int>(this->AssemblyLinks.size()) ? this->AssemblyLinks[idx] : NULL;
}

std::vector< cmbNucAssembly* > cmbNucCore::GetUsedAssemblies()
{
  std::set<std::string> usedDict;
  for(size_t i = 0; i < this->lattice.getSize(); i++)
  {
    for(size_t j = 0; j < this->lattice.getSize(i); j++)
    {
      usedDict.insert(this->lattice.GetCell(i, j).label);
    }
  }
  std::vector< cmbNucAssembly* > result;
  for (unsigned int i = 0; i < this->Assemblies.size(); ++i)
  {
    if(this->Assemblies[i]!=NULL &&
       usedDict.find(this->Assemblies[i]->getLabel()) != usedDict.end())
    {
      result.push_back(Assemblies[i]);
    }
  }
  return result;
}

std::vector< cmbNucAssemblyLink* > cmbNucCore::GetUsedLinks()
{
  std::set<std::string> usedDict;
  for(size_t i = 0; i < this->lattice.getSize(); i++)
  {
    for(size_t j = 0; j < this->lattice.getSize(i); j++)
    {
      usedDict.insert(this->lattice.GetCell(i, j).label);
    }
  }
  std::vector< cmbNucAssemblyLink* > result;
  for (unsigned int i = 0; i < this->AssemblyLinks.size(); ++i)
  {
    if(this->AssemblyLinks[i]!=NULL &&
       usedDict.find(this->AssemblyLinks[i]->getLabel()) != usedDict.end())
    {
      result.push_back(this->AssemblyLinks[i]);
    }
  }
  return result;
}


void cmbNucCore::calculateRectPt(unsigned int i, unsigned int j, double pt[2])
{
  double outerDuctWidth;
  double outerDuctHeight;
  Defaults->getDuctThickness(outerDuctWidth, outerDuctHeight);
  pt[1] = i * (outerDuctHeight)-outerDuctHeight*(this->lattice.getSize()-1);
  pt[0] = j * (outerDuctWidth);
}

void cmbNucCore::calculateRectTranslation(double /*lastPt*/[2], double & transX, double & transY)
{
  cmbNucAssembly * assy = this->GetAssembly(0);
  transX = assy->getAssyDuct().getDuct(0)->getX();
  transY = assy->getAssyDuct().getDuct(0)->getY();
}

void cmbNucCore::setGeometryLabel(std::string geomType)
{
  enumGeometryType type = lattice.GetGeometryType();
  int subType = lattice.GetGeometrySubType() & JUST_ANGLE;
  std::transform(geomType.begin(), geomType.end(), geomType.begin(), ::tolower);
  type = HEXAGONAL;
  if( geomType == "hexflat" )
  {
    subType |= FLAT;
  }
  else if(geomType == "hexvertex")
  {
    subType |= VERTEX;
  }
  else
  {
    type = RECTILINEAR;
    subType = ANGLE_360;
  }
  lattice.SetGeometryType(type);
  lattice.SetGeometrySubType(subType);
}

void cmbNucCore::setHexSymmetry(int sym)
{
  int type = lattice.GetGeometrySubType() & ~JUST_ANGLE; // clear angle
  if(sym == 6)
  {
    type |= ANGLE_60;
  }
  else if(sym == 12)
  {
    type |= ANGLE_30;
  }
  else //all others treated as 360
  {
    type |= ANGLE_360;
  }
  lattice.SetGeometrySubType(type);
}

bool cmbNucCore::IsHexType()
{
  return HEXAGONAL == lattice.GetGeometryType();
}

void cmbNucCore::SetLegendColorToAssemblies(int numDefaultColors, int defaultColors[][3])
{
  for(unsigned int i = 0; i < this->Assemblies.size(); ++i)
  {
      cmbNucAssembly * subAssembly = this->Assemblies[i];
      if (subAssembly)
      {
        int acolorIndex = i  % numDefaultColors;
        QColor acolor(defaultColors[acolorIndex][0],
                      defaultColors[acolorIndex][1],
                      defaultColors[acolorIndex][2]);
        subAssembly->SetLegendColor(acolor);
      }
  }
  this->RebuildGrid();
}

void cmbNucCore::RebuildGrid()
{
  for(unsigned int i = 0; i < Assemblies.size(); ++i)
  {
    this->lattice.SetCellColor(this->Assemblies[i]->getLabel(),
                               this->Assemblies[i]->GetLegendColor());
  }
}

void cmbNucCore::computePitch()
{
  std::vector< cmbNucAssembly* > assemblies = this->GetUsedAssemblies();
  AssemblyPitchX = 0;
  AssemblyPitchY = 0;
  double tmp[2];
  for (unsigned int i = 0; i < assemblies.size(); ++i)
  {
    if(assemblies[i]==NULL) continue;
    assemblies[i]->GetDuctWidthHeight(tmp);
    if(tmp[0]>AssemblyPitchX) AssemblyPitchX = tmp[0];
    if(tmp[1]>AssemblyPitchY) AssemblyPitchY = tmp[1];
  }
}

int cmbNucCore::GetNumberOfAssemblies() const
{
  return static_cast<int>(this->Assemblies.size());
}

int cmbNucCore::GetNumberOfAssemblyLinks() const
{
  return static_cast<int>(this->AssemblyLinks.size());
}

void cmbNucCore::fileChanged()
{
  this->DifferentFromFile = true;
}

void cmbNucCore::boundaryLayerChanged()
{
  std::vector< cmbNucAssembly* > assy = this->GetUsedAssemblies();
  for(size_t i = 0; i < assy.size(); ++i )
  {
    assy[i]->GetConnection()->geometryChanged();
  }
  this->DifferentFromGenBoundaryLayer = true;
  this->DifferentFromFile = true;
}

void cmbNucCore::setAndTestDiffFromFiles(bool diffFromFile)
{
  if(diffFromFile)
  {
    this->fileChanged();
    this->DifferentFromH5M = true;
    this->DifferentFromGenBoundaryLayer = true;
    return;
  }
  //make sure file exits
  //check to see if a h5m file has been generate and is older than this file
  QFileInfo inpInfo(this->CurrentFileName.c_str());
  if(!inpInfo.exists())
  {
    this->fileChanged();
    this->DifferentFromH5M = true;
    this->DifferentFromGenBoundaryLayer = true;
    return;
  }
  this->DifferentFromFile = false;
  if(this->ExportFileName.empty())
  {
    this->DifferentFromH5M = true;
    this->DifferentFromGenBoundaryLayer = true;
    return;
  }
  //QFileInfo h5mFI();
  QDateTime inpLM = inpInfo.lastModified();
  QFileInfo exportInfo(this->ExportFileName.c_str());
  QFileInfo h5mInfo(exportInfo.dir(), this->getMeshOutputFilename().c_str());
  if(!h5mInfo.exists())
  {
    this->DifferentFromH5M = true;
    return;
  }
  QDateTime h5mLM = h5mInfo.lastModified();
  this->DifferentFromH5M = h5mLM < inpLM;
  if(getNumberOfBoundaryLayers() != 0)
  {
    h5mInfo = QFileInfo(exportInfo.dir(), this->getMeshOutputFilename().c_str());
    if(!h5mInfo.exists())
    {
      this->DifferentFromGenBoundaryLayer = true;
      return;
    }
    h5mLM = h5mInfo.lastModified();
    this->DifferentFromGenBoundaryLayer = h5mLM < inpLM;
  }
  else
  {
    this->DifferentFromGenBoundaryLayer = this->DifferentFromH5M;
  }
  this->checkUsedAssembliesForGen();
}

void cmbNucCore::checkUsedAssembliesForGen()
{
  if(this->DifferentFromH5M) return;
  QFileInfo corefi(this->ExportFileName.c_str());
  QFileInfo h5mInfo(corefi.dir(),
                    this->getMeshOutputFilename().c_str());
  std::vector< cmbNucAssembly* > assy = this->GetUsedAssemblies();
  for(unsigned int i = 0; i < assy.size() && !this->DifferentFromH5M; ++i)
  {
    this->DifferentFromH5M |= assy[i]->changeSinceLastGenerate();
    QFileInfo inpInfo(assy[i]->getFileName().c_str());
    QFileInfo cubInfo(corefi.dir(), inpInfo.baseName() + ".cub");
    this->DifferentFromH5M |= !cubInfo.exists() || h5mInfo.lastModified() < cubInfo.lastModified();
  }
}

bool cmbNucCore::changeSinceLastSave() const
{
  return this->DifferentFromFile;
}

bool cmbNucCore::changeSinceLastGenerate() const
{
  return this->DifferentFromH5M;
}

bool cmbNucCore::boundaryLayerChangedSinceLastGenerate() const
{
  return this->DifferentFromH5M || this->DifferentFromGenBoundaryLayer;
}

QPointer<cmbNucDefaults> cmbNucCore::GetDefaults()
{
  return this->Defaults;
}

bool cmbNucCore::HasDefaults() const
{
  return this->Defaults != NULL;
}

void cmbNucCore::initDefaults()
{
  double DuctThickX = 10;
  double DuctThickY = 10;
  double length = 10;
  double z0 = 0;
  delete this->Defaults;
  this->Defaults = new cmbNucDefaults();
  this->Defaults->setHeight(length);
  this->Defaults->setZ0(z0);

  this->Defaults->setDuctThickness(DuctThickX, DuctThickY);
}

void cmbNucCore::calculateDefaults()
{
  delete this->Defaults;
  this->Defaults = new cmbNucDefaults();
  std::vector< cmbNucAssembly* > assys = this->GetUsedAssemblies();

  double AxialMeshSize = 1e23;
  int    EdgeInterval = 2147483647;

  double DuctThickX = -1;
  double DuctThickY = -1;
  double length = -1;
  double z0 = -1e23;
  QString MeshType;

  for(unsigned int i = 0; i < assys.size(); ++i)
  {
    cmbNucAssembly * assy = assys[i];
    DuctCell & dc = assy->getAssyDuct();
    cmbAssyParameters* params =  assy->GetParameters();
    if(params->isValueSet(params->AxialMeshSize) &&
       params->AxialMeshSize < AxialMeshSize)
      AxialMeshSize = params->AxialMeshSize;
    if(params->isValueSet(params->EdgeInterval) &&
       params->EdgeInterval < EdgeInterval)
      EdgeInterval = params->EdgeInterval;
    if(MeshType.isEmpty() && params->isValueSet(params->MeshType))
    {
      MeshType = params->MeshType.c_str();
    }
    if(length < dc.getLength()) length = dc.getLength();
    double z[2];
    dc.getZRange(z[0], z[1]);
    if(z0 < z[0]) z0 = z[0];

    double r[2];
    assy->GetDuctWidthHeight(r);
    if( r[0] > DuctThickX ) DuctThickX = r[0];
    if( r[1] > DuctThickY ) DuctThickY = r[1];
  }
  if(AxialMeshSize != 1e23)
    this->Defaults->setAxialMeshSize(AxialMeshSize);
  if(EdgeInterval != 2147483647)
    this->Defaults->setEdgeInterval(EdgeInterval);
  if(!MeshType.isEmpty()) this->Defaults->setMeshType(MeshType);
  if(length != -1) this->Defaults->setHeight(length);
  if(z0 != -1e23) this->Defaults->setZ0(z0);

  this->Defaults->setDuctThickness(DuctThickX, DuctThickY);
  this->sendDefaults();
}

void cmbNucCore::sendDefaults()
{
  std::vector< cmbNucAssembly* > assys = this->Assemblies;
  for(unsigned int i = 0; i < assys.size(); ++i)
  {
    assys[i]->setFromDefaults(this->Defaults);
  }

  double dt1, dt2, l, z0;
  this->Defaults->getDuctThickness(dt1,dt2);
  this->Defaults->getHeight(l);
  this->Defaults->getZ0(z0);

  for(size_t i = 0; i < this->DuctLibrary->GetNumberOfDuctCells(); ++i)
  {
    DuctCell * dc = this->DuctLibrary->GetDuctCell(i);
    dc->setDuctThickness(dt1,dt2);
    dc->setLength(l);
    dc->setZ0(z0);
  }
}

bool cmbNucCore::label_unique(std::string n)
{
  std::transform(n.begin(), n.end(), n.begin(), ::tolower);
  {
    std::vector< cmbNucAssembly* > assys = this->Assemblies;
    for(unsigned int i = 0; i < assys.size(); ++i)
    {
      std::string aname = assys[i]->getLabel();
      std::transform(aname.begin(), aname.end(), aname.begin(), ::tolower);
      if(aname == n) return false;
    }
  }

  {
    std::vector< cmbNucAssemblyLink* > assys = this->AssemblyLinks;
    for(unsigned int i = 0; i < assys.size(); ++i)
    {
      std::string aname = assys[i]->getLabel();
      std::transform(aname.begin(), aname.end(), aname.begin(), ::tolower);
      if(aname == n) return false;
    }
  }

  return true;
}

void cmbNucCore::fillList(QStringList & l)
{
  l.append("xx");
  for(int i = 0; i < this->GetNumberOfAssemblies(); i++)
  {
    cmbNucAssembly *t = this->GetAssembly(i);
    l.append(t->getLabel().c_str());
  }
  for(int i = 0; i < this->GetNumberOfAssemblyLinks(); i++)
  {
    cmbNucAssemblyLink *t = this->GetAssemblyLink(i);
    l.append(t->getLabel().c_str());
  }
}

AssyPartObj * cmbNucCore::getFromLabel(const std::string & s)
{
  cmbNucAssembly * assy = this->GetAssembly(s);
  if(assy == NULL) return this->GetAssemblyLink(s);
  return assy;
}

void cmbNucCore::drawCylinder(double r, int i)
{
  if(r <= 0 || i <= 0)
  {
    return;
  }
  if(!hasCylinder)
  {
    cmbNucMaterialColors::instance()->getUnknownMaterial()->inc();
  }
  hasCylinder = true;
  cylinderRadius = r;
  cylinderOuterSpacing = i;
}

void cmbNucCore::clearCylinder()
{
  if(hasCylinder)
  {
    cmbNucMaterialColors::instance()->getUnknownMaterial()->dec();
  }
  hasCylinder = false;
}

std::map< std::string, std::set< Lattice::CellDrawMode> >
cmbNucCore::getDrawModesForAssemblies()
{
  std::map< std::string, std::set< Lattice::CellDrawMode> > result;
  for(size_t i = 0; i < lattice.getSize(); i++) //layer
  {
    for(size_t j = 0; j < lattice.getSize(i); j++) //index on ring
    {
      if(!lattice.GetCell(i,j).isBlank())
      {
        result[lattice.GetCell(i,j).label].insert(lattice.getDrawMode(/*index*/j, /*Layer*/ i) );
      }
    }
  }
  return result;
}

bool cmbNucCore::okToDelete(std::string const& label)
{
  //NOTE: We might want to make this more effient
  if(this->Assemblies.size() == 1) return false;
  for(unsigned int i = 0; i < this->AssemblyLinks.size(); ++i)
  {
    if(this->AssemblyLinks[i]->getLink()->getLabel() == label) return false;
  }
  return true;
}

void cmbNucCore::addBoundaryLayer(boundaryLayer * bl)
{
  if(bl == NULL) return;
  this->BoundaryLayers.push_back(bl);
}

int cmbNucCore::getNumberOfBoundaryLayers() const
{
  return static_cast<int>(this->BoundaryLayers.size());
}

void cmbNucCore::removeBoundaryLayer(size_t bl)
{
  if(static_cast<size_t>(bl)>= this->BoundaryLayers.size())
  {
    return;
  }
  this->BoundaryLayers.erase(this->BoundaryLayers.begin() + bl);
}

void cmbNucCore::clearBoundaryLayer()
{
  for(size_t i = 0; i < this->BoundaryLayers.size(); ++i)
  {
    delete this->BoundaryLayers[i];
  }
  BoundaryLayers.clear();
}

cmbNucCore::boundaryLayer * cmbNucCore::getBoundaryLayer(int bl) const
{
  if(static_cast<size_t>(bl)>= this->BoundaryLayers.size())
  {
    return NULL;
  }
  return BoundaryLayers[bl];
}

std::string cmbNucCore::getMeshOutputFilename() const
{
  if(meshFilePrefix.empty()) return "";
  return meshFilePrefix + "." + meshFileExtention;
}

void cmbNucCore::setMeshOutputFilename(std::string const& fname)
{
  QFileInfo qf(fname.c_str());
  this->meshFilePrefix = qf.completeBaseName().toStdString();
  this->meshFileExtention = qf.suffix().toStdString();
}

std::string const& cmbNucCore::getExportFileName() const
{
  return this->ExportFileName;
}

void cmbNucCore::setExportFileName(std::string const& fname)
{
  this->ExportFileName = fname;
  if(meshFilePrefix.empty())
  {
    QFileInfo qf(fname.c_str());
    this->meshFilePrefix = qf.completeBaseName().toStdString();
    this->meshFileExtention = "h5m";
  }
}

void cmbNucCore::setFileName( std::string const& fname )
{
  this->CurrentFileName = fname;
}

void cmbNucCore::setGenerateDirectory(std::string const& dir)
{
  this->GenerateDirectory = dir;
}

std::string const& cmbNucCore::getGenerateDirectory() const
{
  return this->GenerateDirectory;
}
