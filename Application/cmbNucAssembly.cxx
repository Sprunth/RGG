
#include "cmbNucAssembly.h"
#include "cmbNucCore.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <limits>
#include <cctype>
#include <functional>
#include <cassert>

#include "cmbNucMaterialColors.h"
#include "cmbNucDefaults.h"
#include "vtkCmbLayeredConeSource.h"
#include "cmbNucRender.h"
#include "cmbNucPinLibrary.h"
#include "cmbNucDuctLibrary.h"

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

void cmbAssyParameters::fill(cmbAssyParameters * other)
{
  UnknownParams = other->UnknownParams;
  this->Geometry = other->Geometry;
  this->MeshType = other->MeshType;
  this->CenterXYZ = other->CenterXYZ;
  this->HBlock = other->HBlock;
  this->Info = other->Info;
  this->RadialMeshSize = other->RadialMeshSize;
  this->AxialMeshSize = other->AxialMeshSize;
  this->TetMeshSize = other->TetMeshSize;
  this->CreateFiles = other->CreateFiles;
  this->MoveXYZ[0] = other->MoveXYZ[0];
  this->MoveXYZ[1] = other->MoveXYZ[1];
  this->MoveXYZ[2] = other->MoveXYZ[2];
  //this->SectionReverse = false;
#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, DK) this->Var = other->Var;
  ASSYGEN_EXTRA_VARABLE_MACRO()
#undef FUN_SIMPLE
}

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

cmbNucAssembly::Rotate::Rotate(AXIS a, double delta)
{
  Valid = true;
  axis = a;
  this->angle = delta;
}

std::ostream&
cmbNucAssembly::Rotate::write(std::ostream& os) const
{
  os << "Rotate " << TO_AXIS_STRING[this->axis] << " " << this->angle;
  return os;
}

cmbNucAssembly::Section::Section(AXIS a, double v, int d)
{
  Valid = true;
  axis = a;
  value = v;
  dir = d;
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
  if(v->KeepPinsCentered) v->centerPins();
  emit dataChangedSig();
}

void cmbNucAssemblyConnection::geometryChanged()
{
  v->geometryChanged();
  emit dataChangedSig();
  emit colorChanged();
}

void cmbNucAssemblyConnection::pinDeleted(PinCell* pc)
{
  v->RemovePinCell(pc->getLabel());
}

/*************************************************************************/

cmbNucAssembly::cmbNucAssembly()
{
  this->Pins = NULL;
  KeepPinsCentered = true; //on by default
  this->LegendColor = Qt::white;
  this->Parameters = new cmbAssyParameters;
  this->DifferentFromJournel = true;
  this->DifferentFromCub = true;
  this->Connection = new cmbNucAssemblyConnection();
  this->Connection->v = this;
  this->Defaults = new cmbNucDefaults();
  this->AssyDuct = NULL;
  this->pinPitchX = 0;
  this->pinPitchY = 0;

  QObject::connect(this->Connection, SIGNAL(pitchResult(double, double)),
                   this->Defaults,   SIGNAL(recieveCalculatedPitch(double, double)));
}

cmbNucAssembly::~cmbNucAssembly()
{
  for(unsigned int i = 0; i < this->Transforms.size(); ++i)
  {
    delete this->Transforms[i];
    this->Transforms[i] = NULL;
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
  return this->AssyDuct->computeBounds(this->IsHexType());
}

void cmbNucAssembly::getZRange(double & z1, double & z2)
{
  this->AssyDuct->getZRange(z1, z2);
}

void cmbNucAssembly::AddPinCell(PinCell *pincell)
{
  if(pincell == NULL) return;
  QObject::connect(pincell->GetConnection(), SIGNAL(Changed()),
                   this->Connection, SLOT(dataChanged()));
  QObject::connect(pincell->GetConnection(), SIGNAL(CellMaterialChanged()),
                   this->Connection, SLOT(geometryChanged()));
  QObject::connect(pincell->GetConnection(), SIGNAL(Deleted(PinCell*)),
                   this->Connection, SLOT(pinDeleted(PinCell*)));
  this->PinCells.push_back(pincell);
}

void cmbNucAssembly::SetPinCell(int i, PinCell *pc)
{
  if(static_cast<std::size_t>(i) > this->PinCells.size()) return;
  if(pc == NULL) return;
  PinCell * old = this->PinCells[i];
  if(old == pc) return;
  QObject::disconnect(old->GetConnection(), SIGNAL(Changed()),
                      this->Connection, SLOT(dataChanged()));
  QObject::disconnect(old->GetConnection(), SIGNAL(CellMaterialChanged()),
                      this->Connection, SLOT(geometryChanged()));
  QObject::disconnect(old->GetConnection(), SIGNAL(Deleted(PinCell*)),
                      this->Connection, SLOT(pinDeleted(PinCell*)));
  QObject::connect(pc->GetConnection(), SIGNAL(Changed()),
                   this->Connection, SLOT(dataChanged()));
  QObject::connect(pc->GetConnection(), SIGNAL(CellMaterialChanged()),
                   this->Connection, SLOT(geometryChanged()));
  QObject::connect(pc->GetConnection(), SIGNAL(Deleted(PinCell*)),
                   this->Connection, SLOT(pinDeleted(PinCell*)));
  this->PinCells[i] = pc;
}

void cmbNucAssembly::RemovePinCell(const std::string label_in)
{
  for(size_t i = 0; i < this->PinCells.size(); i++)
  {
    if(this->PinCells[i]->getLabel() == label_in)
    {
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

QString
cmbNucAssembly::extractLabel(QString const& s)
{
  return this->Pins->extractLabel(s);
}

void cmbNucAssembly::setUsedLabels(std::map<QString, int> const& labels)
{
  for(size_t i = 0; i < this->PinCells.size(); ++i)
  {
    PinCell *pincell = this->PinCells[i];
    QObject::disconnect(pincell->GetConnection(), SIGNAL(Changed()),
                        this->Connection, SLOT(dataChanged()));
    QObject::disconnect(pincell->GetConnection(), SIGNAL(CellMaterialChanged()),
                        this->Connection, SLOT(geometryChanged()));
    QObject::disconnect(pincell->GetConnection(), SIGNAL(Deleted(PinCell*)),
                        this->Connection, SLOT(pinDeleted(PinCell*)));
  }
  this->PinCells.clear();
  for(std::map<QString, int>::const_iterator it = labels.begin(); it != labels.end(); ++it)
  {
    if(it->second != 0)
    {
      QString l = it->first;
      this->AddPinCell(this->Pins->GetPinCell(l.toStdString()));
    }
  }
}

void cmbNucAssembly::fillList(QStringList & l)
{
  this->Pins->fillList(l);
}

PinCell* cmbNucAssembly::GetPinCell(const std::string &label_in)
{
  return this->Pins->GetPinCell(label_in);
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

void cmbNucAssembly::calculateRectPt(unsigned int i, unsigned j,
                                     double pt[2])
{
  double pitch_ij[2] = {this->pinPitchX,this->pinPitchY};

  if(i==0)
  {
    pt[1] = 0;
  }
  else if(j == 0)
  {
    pt[1] += pitch_ij[1];
  }

  if(j==0)
  {
    pt[0] = 0;
  }
  else
  {
    pt[0] += pitch_ij[0];
  }
}

void cmbNucAssembly::setPitch(double x, double y, bool testDiff)
{
  bool changed = pinPitchX != x || pinPitchY != y;
  this->pinPitchX = x;
  this->pinPitchY = y;
  if(testDiff && changed) this->Connection->dataChanged();
}

void cmbNucAssembly::GetDuctWidthHeight(double r[2])
{
  r[0] = 0;
  r[1] = 0;
  for(unsigned int i = 0; i < this->AssyDuct->numberOfDucts(); ++i)
  {
    Duct * tmpd = this->AssyDuct->getDuct(i);
    double t =tmpd->getThickness(0);
    if(t > r[0]) r[0] = t;
    t = tmpd->getThickness(1);
    if(t > r[1]) r[1] = t;
  }
}

void cmbNucAssembly::computeDefaults()
{
  double x, y, l = AssyDuct->getLength();
  if(l>0) Defaults->setHeight(l);
  this->calculatePitch(x, y);
}

void cmbNucAssembly::calculatePitch(int width, int height, double & x, double & y)
{
  double inDuctThick[2];
  if(!this->AssyDuct->GetInnerDuctSize(inDuctThick[0],inDuctThick[1]) &&
     !this->Defaults->getDuctThickness(inDuctThick[0],inDuctThick[1]))
  {
    inDuctThick[0] = inDuctThick[1] = 10;
  }
  if(this->IsHexType())
  {
    const double d = inDuctThick[0]-inDuctThick[0]*0.035; // make it slightly smaller to make exporting happy
    const double cost=0.86602540378443864676372317075294;
    const double sint=0.5;
    x = y = (cost*d)/(width+sint*(width-1));
  }
  else
  {
    x = (inDuctThick[0])/(height+0.5);
    y = (inDuctThick[1])/(width+0.5);
  }
  if(x<0) x = -1;
  if(y<0) y = -1;
}

void cmbNucAssembly::calculatePitch(double & x, double & y)
{
  std::pair<int, int> dim = lattice.GetDimensions();
  this->calculatePitch(dim.first, dim.second, x, y);
}

void cmbNucAssembly::calculateRadius(double & r)
{
  double minWidth;
  double maxNumber;
  double inDuctThick[2];
  if(!this->AssyDuct->GetInnerDuctSize(inDuctThick[0],inDuctThick[1]) &&
     !this->Defaults->getDuctThickness(inDuctThick[0],inDuctThick[1]))
  {
    inDuctThick[0] = inDuctThick[1] = 10;
  }
  std::pair<int, int> dim = lattice.GetDimensions();
  if(this->IsHexType())
  {
    minWidth = inDuctThick[0]/2.0;
    maxNumber = dim.first-0.5;
  }
  else
  {
    minWidth = std::min(inDuctThick[0],inDuctThick[1]);
    maxNumber = std::max(dim.second, dim.first);
  }
  r = (minWidth/maxNumber)*0.5;
  r = r - r*0.25;
  if(r<0) r = -1;
}

void cmbNucAssembly::setAndTestDiffFromFiles(bool diffFromFile)
{
  if(diffFromFile)
  {
    this->DifferentFromCub = true;
    this->DifferentFromJournel = true;
  }
  else if(!QFileInfo(this->getFileName().c_str()).exists()) // make sure inp exists
  {
    this->DifferentFromCub = true;
    this->DifferentFromJournel = true;
  }
  else if(this->checkJournel(this->getFileName())) //check the assygen output
  {
    this->DifferentFromCub = true;
    this->DifferentFromJournel = true;
  }
  else if(this->checkCubitOutputFile(this->getFileName())) // check the cubit output
  {
    this->DifferentFromCub = true;
    this->DifferentFromJournel = false;
  }
  else
  {
    this->DifferentFromCub = false;
    this->DifferentFromJournel = false;
  }
}

std::string cmbNucAssembly::getOutputExtension()
{
  if(this->Parameters->Save_Exodus)
    return ".exo";
  return ".cub";
}

bool cmbNucAssembly::changeSinceLastGenerate() const
{
  return this->DifferentFromCub || this->DifferentFromJournel;
}

void cmbNucAssembly::clear()
{
  this->PinCells.clear();
  delete this->Parameters;
  this->Parameters = new cmbAssyParameters;
  for(unsigned int i = 0; i < this->Transforms.size(); ++i)
  {
    delete this->Transforms[i];
    this->Transforms[i] = NULL;
  }
  this->Transforms.clear();
}

QSet< cmbNucMaterial* > cmbNucAssembly::getMaterials()
{
  QSet< cmbNucMaterial* > result = AssyDuct->getMaterials();
  for(unsigned int i = 0; i < PinCells.size(); ++i)
  {
    result.unite(PinCells[i]->getMaterials());
  }
  return result;
}

QSet< cmbNucMaterial* > cmbNucAssembly::getInterfaceMaterials(QPointer<cmbNucMaterial> blmat)
{
  if(this->AssyDuct->isInnerDuctMaterial(blmat))
  {
    QSet< cmbNucMaterial* > result = AssyDuct->getInterfaceMaterials(blmat);
    for(unsigned int i = 0; i < PinCells.size(); ++i)
    {
      result.unite(PinCells[i]->getOuterMaterials(blmat));
      //TODO inner materials
    }
    return result;
  }
  else
  {
    //TODO all inner materials
    return QSet< cmbNucMaterial* >();
  }
}

QSet< cmbNucMaterial* > cmbNucAssembly
::getOtherFixed(QPointer<cmbNucMaterial> /*boundaryLayerMaterial*/,
                QPointer<cmbNucMaterial> /*fixedMaterial*/)
{
  //TODO
  return QSet< cmbNucMaterial* >();
}

bool cmbNucAssembly
::has_boundary_layer_interface(QPointer<cmbNucMaterial> in_material) const
{
  //TODO consider inside the pin
  return this->AssyDuct->isInnerDuctMaterial(in_material);
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
  else if( Parameters->AxialMeshSize != ASSY_NOT_SET_VALUE )
  {
    Parameters->AxialMeshSize = ASSY_NOT_SET_VALUE;
    change = true;
  }
  if(d->getEdgeInterval(tmpI))
  {
    change |= Parameters->EdgeInterval != tmpI;
    Parameters->EdgeInterval = tmpI;
  }
  else if(Parameters->EdgeInterval != ASSY_NOT_SET_VALUE)
  {
    Parameters->EdgeInterval = ASSY_NOT_SET_VALUE;
    change = true;
  }
  if(d->getMeshType(tmpS))
  {
    std::string tmp = tmpS.toStdString();
    change |= Parameters->MeshType != tmp;
    Parameters->MeshType = tmp;
  }
  else if(Parameters->MeshType != ASSY_NOT_SET_KEY)
  {
    Parameters->MeshType = ASSY_NOT_SET_KEY;
    change = false;
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
  double px, py;
  calculatePitch(px,py);
  if(px != pinPitchX || py != pinPitchY)
  {
    pinPitchX = px;
    pinPitchY = py;
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

bool cmbNucAssembly::removeOldTransforms(int i)
{
  for(unsigned int r = i; r < this->Transforms.size(); ++r)
  {
    if(this->Transforms[r] != NULL)
    {
      delete this->Transforms[r];
      this->Transforms[r] = NULL;
    }
  }
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

bool cmbNucAssembly::setDuctCell(DuctCell * ad, bool resetPitch)
{
  if(ad == this->AssyDuct)
  {
    double tmpx, tmpy;
    if(resetPitch)
    {
      this->calculatePitch(tmpx, tmpy);
      if( pinPitchX != tmpx || pinPitchY != tmpy )
      {
        pinPitchX = tmpx;
        pinPitchY = tmpy;
        return true;
      }
    }
    return false;
  }

  if(this->AssyDuct != NULL)
  {
    QObject::disconnect(AssyDuct->GetConnection(), SIGNAL(Changed()),
                        this->Connection, SLOT(dataChanged()));
    this->AssyDuct->freed();
  }
  this->AssyDuct = ad;
  if(ad != NULL)
  {
    if(resetPitch) this->calculatePitch(pinPitchX, pinPitchY);
    QObject::connect(ad->GetConnection(), SIGNAL(Changed()),
                     this->Connection, SLOT(dataChanged()));
    this->AssyDuct->used();

  }
  return true;
}

void cmbNucAssembly::setDuctLibrary(cmbNucDuctLibrary * d)
{
  this->Ducts = d;
  if(this->AssyDuct == NULL)
  {
    int at = 0;
    this->setDuctCell(d->GetDuctCell(at));
  }
}

//Used when importing inp file
void cmbNucAssembly::adjustRotation()
{
  if(this->Transforms.empty()) return;
  double r = 0;
  if(this->lattice.getFullCellMode() ==  Lattice::HEX_FULL_30)
  {
    r = -30;
  }
  for(unsigned int i = 0; i < this->Transforms.size(); ++i)
  {
    if(this->Transforms[i]->getLabel() == "Rotate" && this->Transforms[i]->getAxis() == Transform::Z)
    {
      double v = this->Transforms[i]->getValue();
      r += v;
    }
  }
  while(r < -180) r += 360;
  if(r != 0)
  {
    this->setZAxisRotation(static_cast<int>(r));
  }
  removeOldTransforms(0);
}

double cmbNucAssembly::getZAxisRotation() const
{
  if(zAxisRotation.isValid()) return zAxisRotation.getValue();
  return 0;
}

void cmbNucAssembly::setZAxisRotation(double d)
{
  zAxisRotation = Rotate(Transform::Z, d);
}

cmbNucAssembly * cmbNucAssembly::clone(cmbNucPinLibrary * pl,
                                       cmbNucDuctLibrary * dl)
{
  cmbNucAssembly * result = new cmbNucAssembly();
  result->Label = this->Label;
  result->setPinLibrary(pl);
  result->setDuctLibrary(dl);
  for(std::vector<PinCell*>::const_iterator iter = this->PinCells.begin(); iter != this->PinCells.end(); ++iter)
  {
    result->AddPinCell(pl->GetPinCell((*iter)->getLabel()));
  }
  DuctCell * dc = dl->GetDuctCell(this->AssyDuct->getName());
  assert(dc != NULL);
  result->setDuctCell(dc);
  result->zAxisRotation = this->zAxisRotation;
  result->GeometryType = this->GeometryType;
  result->DifferentFromCub = this->DifferentFromCub;
  result->DifferentFromJournel = this->DifferentFromJournel;

  result->Parameters->fill(this->Parameters);

  result->pinPitchX = this->pinPitchX;
  result->pinPitchY = this->pinPitchY;

  result->lattice = Lattice(this->lattice);

  result->ExportFilename = this->ExportFilename;
  result->Path = this->Path;

  return result;
}

void cmbNucAssembly::setPath(std::string const& path)
{
  this->Path = path;
}

void cmbNucAssembly::setFileName(std::string const& fname)
{
  //NOTE: we are stripping directory information from filename
  QFileInfo fi(fname.c_str());
  this->ExportFilename = fi.fileName().toStdString();
}

std::string cmbNucAssembly::getFileName()
{
  if(this->IsHexType())
  {
    return cmbNucAssembly::getFileName(Lattice::HEX_FULL);
  }
  else
  {
    return cmbNucAssembly::getFileName(Lattice::RECT);
  }
}

std::string cmbNucAssembly::getFileName(Lattice::CellDrawMode mode, size_t nom)
{
  if(mode == Lattice::RECT || mode == Lattice::HEX_FULL || mode == Lattice::HEX_FULL_30 || nom == 1)
  {
    if(this->ExportFilename.empty())
    {
      this->ExportFilename = (QString( "assembly_") +
                              QString(this->getLabel().c_str()).toLower() + ".inp").toStdString();
    }
    return this->Path +  "/" + this->ExportFilename;
  }
  return (QString((this->Path + "/assembly_").c_str()) +
          QString(Lattice::generate_string(this->getLabel(),
                                           mode).c_str()).toLower() +
          ".inp").toStdString();
}

bool cmbNucAssembly::needToRunMode(Lattice::CellDrawMode mode, std::string & fname, size_t nom )
{
  fname = this->getFileName(mode, nom);
  if(this->changeSinceLastGenerate()) return true;
  else if(!QFileInfo(fname.c_str()).exists()) return true;
  else if(this->checkJournel(fname)) return true;
  else if(this->checkCubitOutputFile(fname)) return true;
  //else
  return false;
}

bool cmbNucAssembly::checkJournel(std::string const& fname )
{
  QFileInfo inpInfo(fname.c_str());
  QDateTime inpLM = inpInfo.lastModified();
  QFileInfo jrlInfo(inpInfo.dir(), inpInfo.baseName().toLower() + ".jou");
  if(jrlInfo.exists())
  {
    QDateTime jrlLM = jrlInfo.lastModified();
    return jrlLM < inpLM;
  }
  return true;
}

bool cmbNucAssembly::checkCubitOutputFile(std::string const& fname )
{
  QFileInfo inpInfo(fname.c_str());
  QDateTime inpLM = inpInfo.lastModified();
  QFileInfo jrlInfo(inpInfo.dir(), inpInfo.baseName().toLower() + ".jou");
  QFileInfo outInfo(inpInfo.dir(), inpInfo.baseName().toLower() + getOutputExtension().c_str());
  if(!outInfo.exists())
  {
    return true;
  }
  QDateTime cubLM = outInfo.lastModified();
  return cubLM < jrlInfo.lastModified();
}
