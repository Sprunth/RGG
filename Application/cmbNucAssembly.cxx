
#include "cmbNucAssembly.h"
#include "inpFileIO.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <limits>

#include "vtkCmbDuctSource.h"
#include "cmbNucMaterialColors.h"
#include "cmbNucDefaults.h"
#include "vtkCmbConeSource.h"
#include "vtkCmbLayeredConeSource.h"
#include <vtkClipClosedSurface.h>
#include <vtkPlaneCollection.h>

#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkAppendPolyData.h"
#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkPolyDataNormals.h"
#include "vtkNew.h"
#include "vtkMath.h"

#include "vtkXMLMultiBlockDataWriter.h"
#include <QMap>
#include <QDebug>
#include <QFileInfo>
#include <QDateTime>
#include <QDir>

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

void cmbNucAssemblyConnection::calculateRadius()
{
  double r;
  v->calculateRadius(r);
  emit radiusResult(r);
}

cmbNucAssembly::cmbNucAssembly()
{
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
  QObject::connect(this->Defaults,   SIGNAL(calculatePinRadius()),
                   this->Connection, SLOT(calculateRadius()));
  QObject::connect(this->Connection, SIGNAL(pitchResult(double, double)),
                   this->Defaults,   SIGNAL(recieveCalculatedPitch(double, double)));
  QObject::connect(this->Connection, SIGNAL(radiusResult(double)),
                   this->Defaults,   SIGNAL(recieveRadius(double)));
}

cmbNucAssembly::~cmbNucAssembly()
{
  AssyPartObj::deleteObjs(this->PinCells);
  delete this->Parameters;
  delete this->Connection;
  delete this->Defaults;
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
  this->PinCells.push_back(pincell);
}

void cmbNucAssembly::RemovePinCell(const std::string &label)
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

bool cmbNucAssembly::IsHexType()
{
  std::string strGeoType = this->GeometryType;
  std::transform(this->GeometryType.begin(), this->GeometryType.end(),
    strGeoType.begin(), ::tolower);
  return strGeoType == "hexagonal";
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

  std::string pinMaterial = "pin";
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
    double maxLatPt[2] = {0,0};
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
    maxLatPt[0] = offX[r][c];
    maxLatPt[1] = offY[r][c];
    double m2[] = {0,0};
    for(unsigned int i = 0; i < this->AssyDuct.numberOfDucts(); ++i)
    {
      Duct * tmpd = this->AssyDuct.getDuct(i);
      for(unsigned int j = 0; j < tmpd->NumberOfLayers(); ++j)
        {
        double t =tmpd->GetLayerThick(j, 0);
        if(t > m2[0]) m2[0] = t;
        t = tmpd->GetLayerThick(j, 1);
        if(t > m2[1]) m2[1] = t;
        }
    }
    latticeOffset[0] = (m2[0] - maxLatPt[0])*0.5;
    latticeOffset[1] = (m2[1] - maxLatPt[1])*0.5;
    }

  // setup data
  this->Data->SetNumberOfBlocks(this->AssyLattice.GetNumberOfCells() +
                                this->AssyDuct.numberOfDucts());

  // For Hex type
  Duct *hexDuct = this->AssyDuct.getDuct(0);
  double layerCorners[8][2], hexRadius, hexDiameter, layerRadius;
  hexDiameter = hexDuct->thickness[0];
  hexRadius = hexDiameter / (double)(2 * cos(30.0 * vtkMath::Pi() / 180.0));
  hexRadius = hexRadius / (double)(2*this->AssyLattice.Grid.size()-1);

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
              cmbNucAssembly::CreatePinCellMultiBlock(pincell));
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

                transform->Translate(tX, tY, tZ);
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
      ductSource->AddLayer(duct->GetLayerThick(j,0), duct->GetLayerThick(j,1));
      }

    ductSource->Update();

    this->Data->SetBlock(this->Data->GetNumberOfBlocks() - i - 1, ductSource->GetOutput());
    ductSource->Delete();
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
  this->calculateRadius(r);
  if(r>0) Defaults->setPinRadius(r);
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
    double l = AssyLattice.Grid.size();
    x = y = (inDuctThick[0]*0.5)/(l-0.5);
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

vtkMultiBlockDataSet* cmbNucAssembly::CreatePinCellMultiBlock(PinCell* pincell, bool cutaway)
{
  if(pincell->GetNumberOfParts() == 0)
    {
    return vtkMultiBlockDataSet::New();
    }

  // There are two child multibock, one for cylinders, one for frustums
  vtkMultiBlockDataSet *dataSet = vtkMultiBlockDataSet::New();
  dataSet->SetNumberOfBlocks(pincell->GetNumberOfParts());

  // build all cylinders and frustums
  const int PinCellResolution = 16;
  size_t numCyls = pincell->NumberOfCylinders();
  for(size_t j = 0; j < numCyls; j++)
    {
    Cylinder *cylinder = pincell->GetCylinder(j);

    vtkSmartPointer<vtkCmbLayeredConeSource> coneSource =
      vtkSmartPointer<vtkCmbLayeredConeSource>::New();
    coneSource->SetNumberOfLayers(pincell->GetNumberOfLayers());
    coneSource->SetBaseCenter(0, 0, cylinder->z1);
    coneSource->SetHeight(cylinder->z2 - cylinder->z1);

    for(int k = 0; k < pincell->GetNumberOfLayers(); k++)
      {
      coneSource->SetBaseRadius(k, cylinder->getRadius(k));
      coneSource->SetTopRadius(k, cylinder->getRadius(k));
      }
    coneSource->SetResolution(PinCellResolution);
    double direction[] = { 0, 0, 1 };
    coneSource->SetDirection(direction);
    coneSource->Update();

    if(cutaway)
      {
      vtkMultiBlockDataSet *coneData = coneSource->GetOutput();
      for(int block = 0; block < coneData->GetNumberOfBlocks(); block++)
        {
        vtkPolyData *coneLayerData =
          vtkPolyData::SafeDownCast(coneData->GetBlock(block));
        vtkNew<vtkClipClosedSurface> clipper;
        vtkNew<vtkPlaneCollection> clipPlanes;
        vtkNew<vtkPlane> plane;
        vtkNew<vtkPolyDataNormals> normals;
        plane->SetOrigin(0, 0 + 0.001 * block, 0);
        plane->SetNormal(0, 1, 0);
        clipPlanes->AddItem(plane.GetPointer());
        clipper->SetClippingPlanes(clipPlanes.GetPointer());
        clipper->SetActivePlaneId(0);
        clipper->SetClipColor(1.0,1.0,1.0);
        clipper->SetActivePlaneColor(1.0,1.0,0.8);
        clipper->GenerateOutlineOff();
        clipper->SetInputData(coneLayerData);
        clipper->GenerateFacesOn();
        normals->SetInputConnection(clipper->GetOutputPort());
        normals->Update();
        coneData->SetBlock(block, normals->GetOutput());
        }
      dataSet->SetBlock(j, coneData);
      }
    else
      {
      dataSet->SetBlock(j, coneSource->GetOutput());
      }
    }

  for(size_t j = 0; j < pincell->NumberOfFrustums(); j++)
    {
    Frustum* frustum = pincell->GetFrustum(j);

    vtkSmartPointer<vtkCmbLayeredConeSource> coneSource =
      vtkSmartPointer<vtkCmbLayeredConeSource>::New();
    coneSource->SetNumberOfLayers(pincell->GetNumberOfLayers());
    coneSource->SetBaseCenter(0, 0, frustum->z1);
    coneSource->SetHeight(frustum->z2 - frustum->z1);

    for(int k = 0; k < pincell->GetNumberOfLayers(); k++)
      {
      coneSource->SetBaseRadius(k, frustum->getRadius(k,Frustum::BOTTOM));
      coneSource->SetTopRadius(k, frustum->getRadius(k,Frustum::TOP));
      }
    coneSource->SetResolution(PinCellResolution);
    double direction[] = { 0, 0, 1 };
    coneSource->SetDirection(direction);
    coneSource->Update();

    if(cutaway)
      {
      vtkMultiBlockDataSet *coneData = coneSource->GetOutput();
      for(int block = 0; block < coneData->GetNumberOfBlocks(); block++)
        {
        vtkPolyData *coneLayerData =
          vtkPolyData::SafeDownCast(coneData->GetBlock(block));
        vtkNew<vtkClipClosedSurface> clipper;
        vtkNew<vtkPlaneCollection> clipPlanes;
        vtkNew<vtkPlane> plane;
        vtkNew<vtkPolyDataNormals> normals;
        plane->SetOrigin(0, 0 + 0.001 * block, 0);
        plane->SetNormal(0, 1, 0);
        clipPlanes->AddItem(plane.GetPointer());
        clipper->SetClippingPlanes(clipPlanes.GetPointer());
        clipper->SetActivePlaneId(0);
        clipper->SetClipColor(1.0,1.0,1.0);
        clipper->SetActivePlaneColor(1.0,1.0,0.8);
        clipper->GenerateOutlineOff();
        clipper->SetInputData(coneLayerData);
        clipper->GenerateFacesOn();
        normals->SetInputConnection(clipper->GetOutputPort());
        normals->Update();
        coneData->SetBlock(block, normals->GetOutput());
        }
      dataSet->SetBlock(numCyls+j, coneData);
      }
    else
      {
      dataSet->SetBlock(numCyls+j, coneSource->GetOutput());
      }
    }

  return dataSet;
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
  bool change = false;
  double tmpD;
  int tmpI;
  QString tmpS;
  if(d->getRadialMeshSize(tmpD))
  {
    change |= Parameters->RadialMeshSize != tmpD;
    Parameters->RadialMeshSize = tmpD;
  }
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
  if(d->getRotate(tmpS, tmpD))
  {
    change |= Parameters->RotateXYZ != tmpS.toStdString();
    change |= Parameters->RotateAngle != tmpD;
    Parameters->RotateXYZ = tmpS.toStdString();
    Parameters->RotateAngle = tmpD;
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
    this->CreateData();
  }
  if(change) this->Connection->dataChanged();
}
