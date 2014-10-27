
#include "cmbNucAssembly.h"
#include "cmbNucCore.h"
#include "inpFileIO.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <limits>
#include <cctype>

#include "cmbNucMaterialColors.h"
#include "cmbNucDefaults.h"
#include "vtkCmbLayeredConeSource.h"
#include "cmbNucRender.h"

#include <vtkClipClosedSurface.h>
#include <vtkPlaneCollection.h>

#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkAppendPolyData.h"
#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkPolyDataNormals.h"
#include "vtkXMLMultiBlockDataWriter.h"
#include "vtkNew.h"
#include "vtkMath.h"

#include "vtkXMLMultiBlockDataWriter.h"
#include <QMap>
#include <QDebug>
#include <QFileInfo>
#include <QDateTime>
#include <QDir>

std::string TO_AXIS_STRING[] = {"X", "Y", "Z"};

//transformation helper classes

const double cmbNucAssembly::CosSinAngles[6][2] =
{ { cos( 2*(vtkMath::Pi() / 6.0) * (1 + 3) ),
    sin( 2*(vtkMath::Pi() / 6.0) * (1 + 3) ) },
  { cos( 2*(vtkMath::Pi() / 6.0) * (2 + 3) ),
    sin( 2*(vtkMath::Pi() / 6.0) * (2 + 3) ) },
  { cos( 2*(vtkMath::Pi() / 6.0) * (3 + 3) ),
    sin( 2*(vtkMath::Pi() / 6.0) * (3 + 3) ) },
  { cos( 2*(vtkMath::Pi() / 6.0) * (4 + 3) ),
    sin( 2*(vtkMath::Pi() / 6.0) * (4 + 3) ) },
  { cos( 2*(vtkMath::Pi() / 6.0) * (5 + 3) ),
    sin( 2*(vtkMath::Pi() / 6.0) * (5 + 3) ) },
  { cos( 2*(vtkMath::Pi() / 6.0) * (0 + 3) ),
    sin( 2*(vtkMath::Pi() / 6.0) * (0 + 3) ) } };

void cmbNucAssembly::Transform::setAxis(std::string a)
{
  Valid = true;
  if(a == "X" || a == "x") axis = X;
  else if(a == "Y" || a == "y") axis = Y;
  else if(a == "Z" || a == "z") axis = Z;
  else Valid = false;
}

cmbNucAssembly::Rotate::Rotate( std::string a, double delta )
{
  this->setAxis(a);
  this->angle = delta;
}

std::ostream&
cmbNucAssembly::Rotate::write(std::ostream& os) const
{
  os << "Rotate " << TO_AXIS_STRING[this->axis] << " " << this->angle;
  return os;
}

cmbNucAssembly::Section::Section( std::string a, double v, std::string d )
:dir(1), value(v)
{
  setAxis(a);
  std::transform(d.begin(), d.end(), d.begin(), ::tolower);
  d.erase(d.begin(), std::find_if(d.begin(), d.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
  d.erase(std::find_if(d.rbegin(), d.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), d.end());
  if(d == "reverse") dir = -1;
}

std::ostream&
cmbNucAssembly::Section::write(std::ostream& os) const
{
  os << "Section " << TO_AXIS_STRING[this->axis] << " " << this->value;
  if(dir == -1) os << " reverse";
  return os;
}

/*************************************************************************/

void cmbNucAssemblyConnection::dataChanged()
{
  v->setAndTestDiffFromFiles(true);
  emit dataChangedSig();
}

void cmbNucAssemblyConnection::calculatePitch()
{
  double x, y;
  v->calculatePitch(x, y);
  emit pitchResult(x, y);
}

void cmbNucAssemblyConnection::geometryChanged()
{
  v->geometryChanged();
  emit dataChangedSig();
  emit colorChanged();
}

cmbNucAssembly::cmbNucAssembly()
{
  KeepPinsCentered = false;
  this->LegendColor = Qt::white;
  this->Parameters = new cmbAssyParameters;
  this->DifferentFromFile = true;
  this->DifferentFromCub = true;
  this->Connection = new cmbNucAssemblyConnection();
  this->Connection->v = this;
  this->Defaults = new cmbNucDefaults();

  QObject::connect(AssyDuct.GetConnection(), SIGNAL(Changed()),
                   this->Connection, SLOT(dataChanged()));

  QObject::connect(this->Defaults,   SIGNAL(calculatePitch()),
                   this->Connection, SLOT(calculatePitch()));
  QObject::connect(this->Connection, SIGNAL(pitchResult(double, double)),
                   this->Defaults,   SIGNAL(recieveCalculatedPitch(double, double)));
}

cmbNucAssembly::~cmbNucAssembly()
{
  AssyPartObj::deleteObjs(this->PinCells);
  for(unsigned int i = 0; i < this->Transforms.size(); ++i)
  {
    delete this->Transforms[i];
  }
  this->Transforms.clear();
  delete this->Parameters;
  delete this->Connection;
  delete this->Defaults;
}

void cmbNucAssembly::geometryChanged()
{
  setAndTestDiffFromFiles(true);
}

QColor cmbNucAssembly::GetLegendColor() const
{
  return this->LegendColor;
}

void cmbNucAssembly::SetLegendColor(const QColor& color)
{
  this->LegendColor = color;
}

vtkBoundingBox cmbNucAssembly::computeBounds()
{
  return this->AssyDuct.computeBounds(this->IsHexType());
}

void cmbNucAssembly::getZRange(double & z1, double & z2)
{
  this->AssyDuct.getZRange(z1, z2);
}

void cmbNucAssembly::AddPinCell(PinCell *pincell)
{
  QObject::connect(pincell->GetConnection(), SIGNAL(Changed()),
                   this->Connection, SLOT(dataChanged()));
  QObject::connect(pincell->GetConnection(), SIGNAL(CellMaterialChanged()),
                   this->Connection, SLOT(geometryChanged()));
  this->PinCells.push_back(pincell);
}

void cmbNucAssembly::RemovePinCell(const std::string label_in)
{
  for(size_t i = 0; i < this->PinCells.size(); i++)
    {
    if(this->PinCells[i]->label == label_in)
      {
      delete this->PinCells[i];
      this->PinCells.erase(this->PinCells.begin() + i);
      break;
      }
    }
  // update the Grid
  if(this->lattice.ClearCell(label_in))
  {
    setAndTestDiffFromFiles(true);
  }
}

void cmbNucAssembly::fillList(QStringList & l)
{
  for(size_t i = 0; i < this->GetNumberOfPinCells(); i++)
  {
    PinCell *pincell = this->GetPinCell(i);
    l.append(pincell->label.c_str());
  }
}

PinCell* cmbNucAssembly::GetPinCell(const std::string &label_in)
{
  for(size_t i = 0; i < this->PinCells.size(); i++)
    {
    if(this->PinCells[i]->label == label_in)
      {
      return this->PinCells[i];
      }
    }

  return 0;
}

PinCell* cmbNucAssembly::GetPinCell(int pc) const
{
  if(static_cast<size_t>(pc) < this->PinCells.size())
  {
    return this->PinCells[pc];
  }
  return NULL;
}

std::size_t cmbNucAssembly::GetNumberOfPinCells() const
{
  return this->PinCells.size();
}

std::string cmbNucAssembly::getGeometryLabel() const
{
  return this->GeometryType;
}

void cmbNucAssembly::setGeometryLabel(std::string geomType)
{
  this->GeometryType = geomType;
  std::transform(geomType.begin(), geomType.end(),
                 geomType.begin(), ::tolower);
  if(geomType == "hexagonal")
  {
    this->lattice.SetGeometryType(HEXAGONAL);
  }
  else
  {
    this->lattice.SetGeometryType(RECTILINEAR);
  }
}

bool cmbNucAssembly::IsHexType()
{
  return this->lattice.GetGeometryType() == HEXAGONAL;
}

void cmbNucAssembly::ReadFile(const std::string &fname)
{
  inpFileReader freader;
  if(!freader.open(fname))
  {
    return;
  }
  freader.read(*this);
}

void cmbNucAssembly::WriteFile(const std::string &fname)
{
  inpFileWriter::write(fname, *this, true);
}

void cmbNucAssembly::calculateRectPt(unsigned int i, unsigned j,
                                     double pt[2])
{
  std::string const& l = this->lattice.Grid[i][j].label;
  double pitch_ij[2] = {0,0};
  PinCell* pincell = NULL;
  if(!l.empty() && l != "xx" && l != "XX" && (pincell = this->GetPinCell(l)) != NULL )
  {
    pitch_ij[0] = pincell->pitchX;
    pitch_ij[1] = pincell->pitchY;
  }
  else
  {
    for(unsigned int at = 0; at < PinCells.size(); ++at)
    {
      if(PinCells[at] != NULL)
      {
        pitch_ij[0] = PinCells[at]->pitchX;
        pitch_ij[1] = PinCells[at]->pitchY;
        break;
      }
    }
  }

  if(i==0)
  {
    pt[1] = 0;
  }
  else if(j == 0)
  {
    std::string const& l2 = this->lattice.Grid[i-1][j].label;
    if(!l2.empty() && l2 != "xx" && l2 != "XX" && (pincell = this->GetPinCell(l2)) != NULL)
    {
      pt[1] += (pitch_ij[1] + pincell->pitchY) * 0.5;
    }
    else
    {
      pt[1] += pitch_ij[1];
    }
  }

  if(j==0)
  {
    pt[0] = 0;
  }
  else
  {
    std::string const& l2 = this->lattice.Grid[i][j-1].label;
    if(!l2.empty() && l2 != "xx" && l2 != "XX" && (pincell = this->GetPinCell(l2)) != NULL)
    {
      pt[0] += (pitch_ij[0] + pincell->pitchX) * 0.5;
    }
    else
    {
      pt[0] += pitch_ij[0];
    }
  }
}

void cmbNucAssembly::setPitch(double x, double y)
{
  bool changed = false;
  for(unsigned int i = 0; i < PinCells.size(); ++i)
  {
    changed |= PinCells[i]->pitchX != x || PinCells[i]->pitchY != y;
    PinCells[i]->pitchX = x;
    PinCells[i]->pitchY = y;
  }
  if(changed) this->Connection->dataChanged();
}

void cmbNucAssembly::GetDuctWidthHeight(double r[2])
{
  r[0] = 0;
  r[1] = 0;
  for(unsigned int i = 0; i < this->AssyDuct.numberOfDucts(); ++i)
    {
    Duct * tmpd = this->AssyDuct.getDuct(i);
    double t =tmpd->thickness[0];
    if(t > r[0]) r[0] = t;
    t = tmpd->thickness[1];
    if(t > r[1]) r[1] = t;
    }
}

void cmbNucAssembly::computeDefaults()
{
  double x, y, l = AssyDuct.getLength();
  if(l>0) Defaults->setHeight(l);
  this->calculatePitch(x, y);
  if(x>=0 && y >= 0) Defaults->setPitch(x,y);
}

void cmbNucAssembly::calculatePitch(double & x, double & y)
{
  double inDuctThick[2];
  if(!this->AssyDuct.GetInnerDuctSize(inDuctThick[0],inDuctThick[1]) &&
     !this->Defaults->getDuctThickness(inDuctThick[0],inDuctThick[1]))
  {
    inDuctThick[0] = inDuctThick[1] = 10;
  }
  if(this->IsHexType())
  {
    const double d = inDuctThick[0]-inDuctThick[0]*0.035; // make it slightly smaller to make exporting happy
    const double l = this->lattice.Grid.size();
    const double cost=0.86602540378443864676372317075294;
    const double sint=0.5;
    x = y = (cost*d)/(l+sint*(l-1));
  }
  else
  {
    double w = lattice.Grid[0].size();
    double h = lattice.Grid.size();
    x = (inDuctThick[0])/(w+0.5);
    y = (inDuctThick[1])/(h+0.5);
  }
  if(x<0) x = -1;
  if(y<0) y = -1;
}

void cmbNucAssembly::calculateRadius(double & r)
{
  double minWidth;
  double maxNumber;
  double inDuctThick[2];
  if(!this->AssyDuct.GetInnerDuctSize(inDuctThick[0],inDuctThick[1]) &&
     !this->Defaults->getDuctThickness(inDuctThick[0],inDuctThick[1]))
  {
    inDuctThick[0] = inDuctThick[1] = 10;
  }
  if(this->IsHexType())
  {
    minWidth = inDuctThick[0]/2.0;
    maxNumber = lattice.Grid.size()-0.5;
  }
  else
  {
    minWidth = std::min(inDuctThick[0],inDuctThick[1]);
    maxNumber = std::max(lattice.Grid[0].size(),
                         lattice.Grid.size());
  }
  r = (minWidth/maxNumber)*0.5;
  r = r - r*0.25;
  if(r<0) r = -1;
}

void cmbNucAssembly::setAndTestDiffFromFiles(bool diffFromFile)
{
  if(diffFromFile)
  {
    this->DifferentFromFile = true;
    this->DifferentFromCub = true;
    return;
  }
  //make sure file exits
  //check to see if a cub file has been generate and is older than this file
  QFileInfo inpInfo(this->FileName.c_str());
  if(!inpInfo.exists())
  {
    this->DifferentFromFile = true;
    this->DifferentFromCub = true;
    return;
  }
  this->DifferentFromFile = false;
  QDateTime inpLM = inpInfo.lastModified();
  QFileInfo cubInfo(inpInfo.dir(), inpInfo.baseName().toLower() + ".cub");
  if(!cubInfo.exists())
  {
    this->DifferentFromCub = true;
    return;
  }
  QDateTime cubLM = cubInfo.lastModified();
  this->DifferentFromCub = cubLM < inpLM;
}

bool cmbNucAssembly::changeSinceLastSave() const
{
  return this->DifferentFromFile;
}

bool cmbNucAssembly::changeSinceLastGenerate() const
{
  return this->DifferentFromCub;
}

void cmbNucAssembly::clear()
{
  AssyPartObj::deleteObjs(this->PinCells);
  this->PinCells.clear();
  delete this->Parameters;
  this->Parameters = new cmbAssyParameters;
}

QSet< cmbNucMaterial* > cmbNucAssembly::getMaterials()
{
  QSet< cmbNucMaterial* > result = AssyDuct.getMaterials();
  for(unsigned int i = 0; i < PinCells.size(); ++i)
  {
    result.unite(PinCells[i]->getMaterials());
  }
  return result;
}

void cmbNucAssembly::setFromDefaults(QPointer<cmbNucDefaults> d)
{
  if(d == NULL) return;
  bool change = false;
  double tmpD;
  int tmpI;
  QString tmpS;
  if(d->getAxialMeshSize(tmpD))
  {
    change |= Parameters->AxialMeshSize != tmpD;
    Parameters->AxialMeshSize = tmpD;
  }
  if(d->getEdgeInterval(tmpI))
  {
    change |= Parameters->EdgeInterval != tmpI;
    Parameters->EdgeInterval = tmpI;
  }
  if(d->getMeshType(tmpS))
  {
    std::string tmp = tmpS.toStdString();
    change |= Parameters->MeshType != tmp;
    Parameters->MeshType = tmp;
  }

  double tmpd2;
  if(d->getDuctThickness(tmpD,tmpd2))
  {
    for(unsigned int i = 0; i < this->AssyDuct.numberOfDucts(); ++i)
    {
      Duct * duct = this->AssyDuct.getDuct(i);
      change |= tmpD != duct->thickness[0];
      duct->thickness[0] = tmpD;
      change |= tmpd2 != duct->thickness[1];
      duct->thickness[1] = tmpd2;
    }
    this->Defaults->setDuctThickness(tmpD,tmpd2);
    if(KeepPinsCentered) this->centerPins();
  }
  if(d->getHeight(tmpD))
  {
    if(tmpD != AssyDuct.getLength())
    {
      change = true;
      AssyDuct.setLength(tmpD);
    }
    this->Defaults->setHeight(tmpD);
  }
  if(change) this->Connection->dataChanged();
}

void cmbNucAssembly::setCenterPins(bool t)
{
  KeepPinsCentered = t;
  if(KeepPinsCentered)
  {
    this->centerPins();
  }
}

void cmbNucAssembly::centerPins()
{
  bool change = false;
  double px, py;
  calculatePitch(px,py);
  for(unsigned int i = 0; i < PinCells.size(); ++i)
  {
    bool regen = false;
    PinCell * pc = PinCells[i];
    regen |= pc->pitchX != px;
    pc->pitchX = px;
    regen |= pc->pitchY != py;
    pc->pitchY = py;
    if(regen)
    {
      change = true;
      pc->CachedData = NULL; //will be filled in create data
    }
  }
  if(change)
  {
    this->Connection->dataChanged();
  }
}

bool cmbNucAssembly::addTransform(cmbNucAssembly::Transform * in)
{
  if(in != NULL && in->isValid())
  {
    this->Transforms.push_back(in);
    return true;
  }
  return false;
}

void cmbNucAssembly::setLabel(std::string & n)
{
  if(this->label != n)
  {
    this->label = n;
  }
}

bool cmbNucAssembly::updateTransform(int at, Transform * in)
{
  if(in != NULL && in->isValid() &&
     static_cast<size_t>(at) <= this->Transforms.size())
  {
    Transform * tat = NULL;
    if(static_cast<size_t>(at) == this->Transforms.size() && addTransform(in))
    {
      return true;
    }
    else if( ( tat = getTransform(at) ) != NULL &&
             ( tat->getAxis() != in->getAxis() ||
               tat->getValue() != in->getValue() ||
               tat->reverse() != in->reverse() ||
               tat->getLabel() != in->getLabel() ) )
    {
      this->Transforms[at] =in;
      delete tat;
      return true;
    }
  }
  delete in;
  return false;
}

bool cmbNucAssembly::removeOldTransforms(int i)
{
  if(static_cast<size_t>(i) < this->Transforms.size())
  {
    this->Transforms.resize(i);
    return true;
  }
  return false;
}

cmbNucAssembly::Transform* cmbNucAssembly::getTransform(int i) const
{
  if(static_cast<size_t>(i) < this->Transforms.size())
  {
    return this->Transforms[i];
  }
  return NULL;
}

size_t cmbNucAssembly::getNumberOfTransforms() const
{
  return this->Transforms.size();
}
