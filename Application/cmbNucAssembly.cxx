
#include "cmbNucAssembly.h"
#include "cmbNucCore.h"
#include "inpFileIO.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <limits>
#include <cctype>

#include "vtkCmbDuctSource.h"
#include "cmbNucMaterialColors.h"
#include "cmbNucDefaults.h"
#include "vtkCmbLayeredConeSource.h"
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

void
cmbNucAssembly::Rotate::apply( vtkMultiBlockDataSet * input,
                               vtkMultiBlockDataSet * output ) const
{
  vtkSmartPointer<vtkTransform> xform = vtkSmartPointer<vtkTransform>::New();
  switch(this->axis)
  {
    case X:
      xform->RotateX(this->angle);
      break;
    case Y:
      xform->RotateY(this->angle);
      break;
    case Z:
      xform->RotateZ(this->angle);
      break;
  }
  cmbNucCore::transformData(input, output, xform);
}

std::ostream&
cmbNucAssembly::Rotate::write(std::ostream& os) const
{
  os << "Rotate " << TO_AXIS_STRING[this->axis] << " " << this->angle;
  return os;
}

cmbNucAssembly::Section::Section( std::string a, double v, std::string d )
:value(v), dir(1)
{
  setAxis(a);
  std::transform(d.begin(), d.end(), d.begin(), ::tolower);
  d.erase(d.begin(), std::find_if(d.begin(), d.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
  d.erase(std::find_if(d.rbegin(), d.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), d.end());
  if(d == "reverse") dir = -1;
}

void
cmbNucAssembly::Section::apply( vtkMultiBlockDataSet * input,
                                vtkMultiBlockDataSet * output ) const
{
  double normal[3];
  double tmp[] ={0,0,0};
  tmp[this->axis] = dir;
  vtkSmartPointer<vtkTransform> xform = vtkSmartPointer<vtkTransform>::New();
  xform->RotateZ(60);
  xform->TransformPoint(tmp,normal);
  double sum = std::sqrt(normal[0]*normal[0]+ normal[1]*normal[1] + normal[2]*normal[2]);
  normal[0] /= sum;
  normal[1] /= sum;
  normal[2] /= sum;
  cmbNucAssembly::clip(input, output, normal);
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
  this->Data = vtkSmartPointer<vtkMultiBlockDataSet>::New();
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
  CreateData();
}

QColor cmbNucAssembly::GetLegendColor() const
{
  return this->LegendColor;
}

void cmbNucAssembly::SetLegendColor(const QColor& color)
{
  this->LegendColor = color;
}

void cmbNucAssembly::UpdateGrid()
{
  std::pair<int, int> dim = this->AssyLattice.GetDimensions();
  for(size_t i = 0; i < dim.first; i++)
    {
    size_t layerCells = this->AssyLattice.GetGeometryType() == HEXAGONAL ?
      6*i : dim.second;
    for(size_t j = 0; j < layerCells; j++)
      {
      std::string label = this->AssyLattice.GetCell(i, j).label;
      PinCell* pc = this->GetPinCell(label);
      if(pc)
        {
        this->AssyLattice.SetCell(i, j, label, pc->GetLegendColor());
        }
      else
        {
        this->AssyLattice.ClearCell(i, j);
        }
      }
    }
}

void cmbNucAssembly::AddPinCell(PinCell *pincell)
{
  QObject::connect(pincell->GetConnection(), SIGNAL(Changed()),
                   this->Connection, SLOT(dataChanged()));
  QObject::connect(pincell->GetConnection(), SIGNAL(CellMaterialChanged()),
                   this->Connection, SLOT(geometryChanged()));
  this->PinCells.push_back(pincell);
}

void cmbNucAssembly::RemovePinCell(const std::string label)
{
  for(size_t i = 0; i < this->PinCells.size(); i++)
    {
    if(this->PinCells[i]->label == label)
      {
      delete this->PinCells[i];
      this->PinCells.erase(this->PinCells.begin() + i);
      break;
      }
    }
  // update the Grid
  if(this->AssyLattice.ClearCell(label))
  {
    setAndTestDiffFromFiles(true);
    CreateData();
  }
}

PinCell* cmbNucAssembly::GetPinCell(const std::string &label)
{
  for(size_t i = 0; i < this->PinCells.size(); i++)
    {
    if(this->PinCells[i]->label == label)
      {
      return this->PinCells[i];
      }
    }

  return 0;
}

PinCell* cmbNucAssembly::GetPinCell(int pc) const
{
  if(pc < this->PinCells.size())
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
    this->AssyLattice.SetGeometryType(HEXAGONAL);
  }
  else
  {
    this->AssyLattice.SetGeometryType(RECTILINEAR);
  }
}

bool cmbNucAssembly::IsHexType()
{
  return this->AssyLattice.GetGeometryType() == HEXAGONAL;
}

void cmbNucAssembly::ReadFile(const std::string &FileName)
{
  inpFileReader freader;
  if(!freader.open(FileName))
  {
    return;
  }
  freader.read(*this);
}

void cmbNucAssembly::WriteFile(const std::string &fname)
{
  inpFileWriter::write(fname, *this, true);
}

void cmbNucAssembly::updateMaterialColors(
  unsigned int& realflatidx,
  vtkCompositeDataDisplayAttributes *attributes)
{

  // count number of pin blocks in the data set
  int pins = this->AssyLattice.GetNumberOfCells();
  int pin_count = 0;
  int ducts_count = 0;
  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();

  int numAssyBlocks = this->AssyLattice.GetNumberOfCells() +
                      this->AssyDuct.numberOfDucts();
  for(unsigned int idx = 0; idx < this->AssyLattice.GetNumberOfCells(); ++idx)
  {
    realflatidx++;
    std::string label = this->AssyLattice.GetCell(idx).label;
    PinCell* pinCell = this->GetPinCell(label);

    if(pinCell)
    {
      for(unsigned int cidx = 0; cidx < pinCell->GetNumberOfParts(); cidx++)
      {
        realflatidx++; // increase one for this Part
        for(int k = 0; k < pinCell->GetNumberOfLayers(); k++)
        {
          matColorMap->SetBlockMaterialColor(attributes, ++realflatidx,
                                             pinCell->GetPart(cidx)->GetMaterial(k));
        }
        if(pinCell->cellMaterialSet())
        {
          matColorMap->SetBlockMaterialColor(attributes, ++realflatidx,
                                             pinCell->getCellMaterial());
        }
      }
    }
  }
  for(unsigned int idx = 0; idx < this->AssyDuct.numberOfDucts(); ++idx)
  {
    realflatidx++;
    if(vtkMultiBlockDataSet* ductBlock =
       vtkMultiBlockDataSet::SafeDownCast(this->Data->GetBlock(this->AssyLattice.GetNumberOfCells()+idx)))
    {
      Duct* duct = this->AssyDuct.getDuct(idx);
      unsigned int numBlocks = ductBlock->GetNumberOfBlocks();
      for(unsigned int b = 0; b < numBlocks; b++)
      {
        matColorMap->SetBlockMaterialColor(attributes, ++realflatidx,
                                           duct->getMaterial(b));
      }
    }
  }
}

vtkSmartPointer<vtkMultiBlockDataSet> cmbNucAssembly::GetData()
{
  if(this->Data->GetNumberOfBlocks() > 0)
    {
    return this->Data;
    }
  return this->CreateData();
}

void cmbNucAssembly::computeRecOffset(unsigned int i, unsigned j,
                                      double &tx, double &ty)
{
  std::string const& l = this->AssyLattice.Grid[i][j].label;
  double pitch_ij[2] = {0,0};
  PinCell* pincell = NULL;
  if(!l.empty() && l != "xx" && l != "XX" && (pincell = this->GetPinCell(l)) != NULL )
  {
    pitch_ij[0] = pincell->pitchX;
    pitch_ij[1] = pincell->pitchY;
  }
  else
  {
    for(unsigned int i = 0; i < PinCells.size(); ++i)
    {
      if(PinCells[i] != NULL)
      {
        pitch_ij[0] = PinCells[i]->pitchX;
        pitch_ij[1] = PinCells[i]->pitchY;
        break;
      }
    }
  }

  if(i==0)
  {
    ty = 0;
  }
  else if(j == 0)
  {
    std::string const& l2 = this->AssyLattice.Grid[i-1][j].label;
    if(!l2.empty() && l2 != "xx" && l2 != "XX" && (pincell = this->GetPinCell(l2)) != NULL)
    {
      ty += (pitch_ij[1] + pincell->pitchY) * 0.5;
    }
    else
    {
      ty += pitch_ij[1];
    }
  }

  if(j==0)
  {
    tx = 0;
  }
  else
  {
    std::string const& l2 = this->AssyLattice.Grid[i][j-1].label;
    if(!l2.empty() && l2 != "xx" && l2 != "XX" && (pincell = this->GetPinCell(l2)) != NULL)
    {
      tx += (pitch_ij[0] + pincell->pitchX) * 0.5;
    }
    else
    {
      tx += pitch_ij[0];
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

vtkSmartPointer<vtkMultiBlockDataSet> cmbNucAssembly::CreateData()
{
  if(this->AssyDuct.numberOfDucts()==0 || this->AssyLattice.Grid.size() == 0)
    {
    return NULL;
    }
  double currentLaticePoint[] = {0,0};
  double latticeOffset[2];

  std::vector< std::vector< double > > offX, offY;

  if(this->AssyLattice.GetGeometryType() == RECTILINEAR)
    {
    offX.resize(this->AssyLattice.Grid.size(), std::vector< double >(this->AssyLattice.Grid[0].size(), 0));
    offY.resize(this->AssyLattice.Grid.size(), std::vector< double >(this->AssyLattice.Grid[0].size(), 0));
    for(size_t i = 0; i < this->AssyLattice.Grid.size(); i++)
      {
      const std::vector<LatticeCell> &row = this->AssyLattice.Grid[i];
      for(size_t j = 0; j < row.size(); j++)
        {
        this->computeRecOffset(i, j, currentLaticePoint[0], currentLaticePoint[1]);
        offX[i][j] = currentLaticePoint[0];
        offY[i][j] = currentLaticePoint[1];
        }
      }
    size_t r = this->AssyLattice.Grid.size()-1;
    size_t c = this->AssyLattice.Grid[0].size()-1;
    double maxLatPt[2] = {offX[r][c], offY[r][c]};
    latticeOffset[0] = -maxLatPt[0]*0.5;
    latticeOffset[1] = -maxLatPt[1]*0.5;
    }

  // setup data
  this->Data->SetNumberOfBlocks(this->AssyLattice.GetNumberOfCells() +
                                this->AssyDuct.numberOfDucts());

  // For Hex type
  Duct *hexDuct = this->AssyDuct.getDuct(0);
  double layerCorners[8][2], layerRadius;

  double overallDx = 0;
  for(size_t i = 0; i < this->AssyLattice.Grid.size(); i++)
    {
    double overallDy = 0;
    // For hex geometry type, figure out the six corners first
    if(this->AssyLattice.GetGeometryType() == HEXAGONAL && i>0)
      {
      for(int c = 0; c < 6; c++)
        {
        double angle = 2 * (vtkMath::Pi() / 6.0) * (c + 3.5);
        layerCorners[c][0] = cos(angle);
        layerCorners[c][1] = sin(angle);
        }
      }

    size_t startBlock = this->AssyLattice.GetGeometryType() == HEXAGONAL ?
      (i==0 ? 0 : (1 + 3*i*(i-1))) : (i*this->AssyLattice.Grid[0].size());
    const std::vector<LatticeCell> &row = this->AssyLattice.Grid[i];
    for(size_t j = 0; j < row.size(); j++)
      {
      const std::string &type = row[j].label;

      if(!type.empty() && type != "xx" && type != "XX")
        {
        PinCell* pincell = this->GetPinCell(type);
        if(pincell && (pincell->GetNumberOfParts())>0)
          {
          // create polydata for the pincell
          if(!pincell->CachedData)
            {
            pincell->CachedData.TakeReference(
              cmbNucAssembly::CreatePinCellMultiBlock(pincell, this->IsHexType()));
            }
          vtkMultiBlockDataSet *dataSet = pincell->CachedData;
          vtkNew<vtkMultiBlockDataSet> pinDataSet;
          pinDataSet->SetNumberOfBlocks(dataSet->GetNumberOfBlocks());
          for(int block=0; block<dataSet->GetNumberOfBlocks(); block++)
            {
            vtkMultiBlockDataSet* sectionBlock =
              vtkMultiBlockDataSet::SafeDownCast(dataSet->GetBlock(block));
            if(!sectionBlock)
              {
              continue;
              }
            vtkNew<vtkMultiBlockDataSet> transdataSet;
            transdataSet->SetNumberOfBlocks(sectionBlock->GetNumberOfBlocks());
            for(int layer=0; layer<sectionBlock->GetNumberOfBlocks(); layer++)
              {
              vtkPolyData* polyBlock =
                vtkPolyData::SafeDownCast(sectionBlock->GetBlock(layer));
              if(!polyBlock)
                {
                continue;
                }

              // move the polydata to the correct position
              vtkTransform *transform = vtkTransform::New();

              if(this->AssyLattice.GetGeometryType() == HEXAGONAL)
                {
                double pinDistFromCenter = pincell->pitchX * (i);
                double tX=hexDuct->x, tY=hexDuct->y, tZ=0.0;
                int cornerIdx;
                if(i == 1)
                  {
                  cornerIdx = j%6;
                  tX += pinDistFromCenter*layerCorners[cornerIdx][0];
                  tY += pinDistFromCenter*layerCorners[cornerIdx][1];
                  }
                else if( i > 1)
                  {
                  cornerIdx = j / i;
                  int idxOnEdge = j%i;
                  if(idxOnEdge == 0) // one of the corners
                    {
                    tX += pinDistFromCenter*layerCorners[cornerIdx][0];
                    tY += pinDistFromCenter*layerCorners[cornerIdx][1];
                    }
                  else
                    {
                    // for each layer, we should have (numLayers-2) middle hexes
                    // between the corners
                    double deltx, delty, numSegs = i, centerPos[2];
                    int idxNext = cornerIdx==5 ? 0 : cornerIdx+1;
                    deltx = pinDistFromCenter*(layerCorners[idxNext][0] - layerCorners[cornerIdx][0]) / numSegs;
                    delty = pinDistFromCenter*(layerCorners[idxNext][1] - layerCorners[cornerIdx][1]) / numSegs;
                    centerPos[0] = pinDistFromCenter*layerCorners[cornerIdx][0] + deltx * (idxOnEdge);
                    centerPos[1] = pinDistFromCenter*layerCorners[cornerIdx][1] + delty * (idxOnEdge);
                    tX += centerPos[0];
                    tY += centerPos[1];
                    }
                  }

                transform->RotateZ(30);
                transform->Translate(tX, -tY, tZ);
                }
              else
                {
                double tx = offX[i][j]+latticeOffset[0];
                double ty = offY[i][j]+latticeOffset[1];
                transform->Translate(tx,ty, 0);
                }

              vtkNew<vtkTransformFilter> filter;
              filter->SetTransform(transform);
              transform->Delete();
              filter->SetInputDataObject(polyBlock);
              filter->Update();
              transdataSet->SetBlock(layer, filter->GetOutput());
              }
            pinDataSet->SetBlock(block, transdataSet.GetPointer());
            }
          this->Data->SetBlock(startBlock+j, pinDataSet.GetPointer());
          }
        else
          {
          this->Data->SetBlock(startBlock+j, NULL);
          }
        }
      else
        {
        this->Data->SetBlock(startBlock+j, NULL);
        }
      }
    }

  // setup ducts
  double z, deltaZ, height;
  size_t numDucts = this->AssyDuct.numberOfDucts();
  for(size_t i = 0; i < numDucts; i++)
    {
    Duct *duct = this->AssyDuct.getDuct(i);

    vtkCmbDuctSource *ductSource = vtkCmbDuctSource::New();
    z = duct->z1;
    height = duct->z2 - duct->z1;
    double deltaZ = height * 0.0005;
    // For first duct, move the Origin up in z by 0.05 % of the the Height so
    // that the bottoms of the pins are not covered by duct's bottom
    // For last duct, Reduce the height by 0.1 % of the Height so
    // that the tops of the pins are not covered by duct's top
    if(i == 0) // first duct
      {
      z = duct->z1 + deltaZ;
      // if more than one duct, first duct height need to be reduced by deltaZ
      height = numDucts > 1 ? height - deltaZ : height - 2*deltaZ;
      }
    else if (i == numDucts - 1) // last duct
      {
      height -= 2*deltaZ;
      }

    ductSource->SetOrigin(duct->x, duct->y, z);
    ductSource->SetHeight(height);
    ductSource->SetGeometryType(this->AssyLattice.GetGeometryType());

    for(size_t j = 0; j < duct->NumberOfLayers(); j++)
      {
      ductSource->AddLayer(duct->GetLayerThick(j,0) - duct->GetLayerThick(j,0)*0.0005,
                           duct->GetLayerThick(j,1) - duct->GetLayerThick(j,0)*0.0005);
      }

    ductSource->Update();

    this->Data->SetBlock(this->AssyLattice.GetNumberOfCells() + i, ductSource->GetOutput());
    ductSource->Delete();
    }

  for(unsigned int i = 0; i < this->Transforms.size(); ++i)
    {
    this->Transforms[i]->apply(this->Data, this->Data);
    }

  return this->Data;
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
  double x, y, r, l = AssyDuct.getLength();
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
    const double l = this->AssyLattice.Grid.size();
    const double cost=0.86602540378443864676372317075294;
    const double sint=0.5;
    x = y = (cost*d)/(l+sint*(l-1));
  }
  else
  {
    double w = AssyLattice.Grid[0].size();
    double h = AssyLattice.Grid.size();
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
    maxNumber = AssyLattice.Grid.size()-0.5;
  }
  else
  {
    minWidth = std::min(inDuctThick[0],inDuctThick[1]);
    maxNumber = std::max(AssyLattice.Grid[0].size(),
                         AssyLattice.Grid.size());
  }
  r = (minWidth/maxNumber)*0.5;
  r = r - r*0.25;
  if(r<0) r = -1;
}

void cmbNucAssembly::removeDuct(Duct* d)
{
  AssyDuct.RemoveDuct(d);
  this->CreateData();
}

vtkMultiBlockDataSet* cmbNucAssembly::CreatePinCellMultiBlock(PinCell* pincell, bool isHex, bool cutaway)
{
  if(pincell->GetNumberOfParts() == 0)
    {
    return vtkMultiBlockDataSet::New();
    }

  // There are two child multibock, one for cylinders, one for frustums
  vtkMultiBlockDataSet *dataSet = vtkMultiBlockDataSet::New();
  dataSet->SetNumberOfBlocks(pincell->GetNumberOfParts());

  // build all cylinders and frustums
  const int PinCellResolution = 18;
  size_t numParts = pincell->GetNumberOfParts();
  for(size_t j = 0; j < numParts; ++j)
    {
    PinSubPart* part = pincell->GetPart(j);

    vtkSmartPointer<vtkCmbLayeredConeSource> coneSource =
      vtkSmartPointer<vtkCmbLayeredConeSource>::New();
    coneSource->SetNumberOfLayers(pincell->GetNumberOfLayers() + (pincell->cellMaterialSet()?1:0));
    coneSource->SetBaseCenter(0, 0, part->z1);
    coneSource->SetHeight(part->z2 - part->z1);
    double lastR[2];

    for(int k = 0; k < pincell->GetNumberOfLayers(); k++)
      {
      lastR[0] = part->getRadius(k,Frustum::BOTTOM);
      lastR[1] = part->getRadius(k,Frustum::TOP);
      coneSource->SetBaseRadius(k, part->getRadius(k,Frustum::BOTTOM));
      coneSource->SetTopRadius(k, part->getRadius(k,Frustum::TOP));
      coneSource->SetResolution(k, PinCellResolution);
      }
    if(pincell->cellMaterialSet())
      {
      double r[] = {pincell->pitchX*0.5, pincell->pitchY*0.5};
      int res = 4;
      if(isHex)
        {
        res = 6;
        r[0] = r[1] = r[0]/0.86602540378443864676372317075294;
        }
      coneSource->SetBaseRadius(pincell->GetNumberOfLayers(), r[0], r[1]);
      coneSource->SetTopRadius(pincell->GetNumberOfLayers(), r[0], r[1]);
      coneSource->SetResolution(pincell->GetNumberOfLayers(), res);
      }
    double direction[] = { 0, 0, 1 };
    coneSource->SetDirection(direction);
    coneSource->Update();

    if(cutaway)
      {
      vtkMultiBlockDataSet *coneData = coneSource->GetOutput();
      double normal[] = {0, 1, 0};
      clip(coneData,coneData,normal);
      dataSet->SetBlock(j, coneData);
      }
    else
      {
      dataSet->SetBlock(j, coneSource->GetOutput());
      }
    }

  return dataSet;
}

void cmbNucAssembly::clip(vtkMultiBlockDataSet * input, vtkMultiBlockDataSet * output,
                          double * normal, int offset)
{
  vtkNew<vtkPlane> plane;
  plane->SetOrigin(-normal[0]*0.005*offset, -normal[1]*0.005*offset, -normal[2]*0.005*offset);
  plane->SetNormal(normal[0], normal[1], normal[2]);
  for(int block = 0; block < input->GetNumberOfBlocks(); block++)
  {
    if(vtkDataObject* objBlock = input->GetBlock(block))
    {
      if(vtkMultiBlockDataSet* part =
         vtkMultiBlockDataSet::SafeDownCast(objBlock))
      {
        vtkSmartPointer<vtkMultiBlockDataSet> clipPart = vtkSmartPointer<vtkMultiBlockDataSet>::New();
        clipPart->SetNumberOfBlocks(part->GetNumberOfBlocks());
        clip(part, clipPart, normal, offset+1);
        output->SetBlock(block, clipPart);
      }
      else if(vtkPolyData *pd =
              vtkPolyData::SafeDownCast(objBlock))
      {
        vtkNew<vtkClipClosedSurface> clipper;
        vtkNew<vtkPlaneCollection> clipPlanes;
        vtkNew<vtkPolyDataNormals> normals;
        clipPlanes->AddItem(plane.GetPointer());
        clipper->SetClippingPlanes(clipPlanes.GetPointer());
        clipper->SetActivePlaneId(0);
        clipper->SetClipColor(1.0,1.0,1.0);
        clipper->SetActivePlaneColor(1.0,1.0,0.8);
        clipper->GenerateOutlineOff();
        clipper->SetInputData(pd);
        clipper->GenerateFacesOn();
        normals->SetInputConnection(clipper->GetOutputPort());
        normals->Update();
        vtkPolyData * result = normals->GetOutput();
        if(result->GetNumberOfPolys()!=0)
          output->SetBlock(block, result);
        else
          output->SetBlock(block, NULL);
      }
      else
      {
        output->SetBlock(block, NULL);
      }
    }
    else
    {
      output->SetBlock(block, NULL);
    }
  }
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
  QFileInfo cubInfo(inpInfo.dir(), inpInfo.baseName() + ".cub");
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
  bool generate_data = false;
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
    generate_data = true;
    if(KeepPinsCentered) this->centerPins();
  }
  if(d->getHeight(tmpD))
  {
    if(tmpD != AssyDuct.getLength())
    {
      change = true;
      generate_data = true;
      AssyDuct.setLength(tmpD);
    }
    this->Defaults->setHeight(tmpD);
  }
  if(generate_data) this->CreateData();
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
    this->CreateData();
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

bool cmbNucAssembly::updateTransform(int at, Transform * in)
{
  if(in != NULL && in->isValid() && at <= this->Transforms.size())
  {
    Transform * tat = NULL;
    if(at == this->Transforms.size() && addTransform(in))
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
  if(i < this->Transforms.size())
  {
    this->Transforms.resize(i);
    return true;
  }
  return false;
}

cmbNucAssembly::Transform* cmbNucAssembly::getTransform(int i) const
{
  if(i < this->Transforms.size()) return this->Transforms[i];
  return NULL;
}

size_t cmbNucAssembly::getNumberOfTransforms() const
{
  return this->Transforms.size();
}
