
#include "vtkCmbLayeredConeSource.h"

#include <vtkObjectFactory.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>

#include <vtkCellArray.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include "vtkPolyDataNormals.h"
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkNew.h>
#include <vtkMath.h>
#include <vtkAppendPolyData.h>
#include <vtkDoubleArray.h>

#include <cassert>

vtkStandardNewMacro(vtkCmbLayeredConeSource);

vtkCmbLayeredConeSource::vtkCmbLayeredConeSource()
{
  this->SetNumberOfInputPorts(0);

  this->Height = 1.0;
  this->Resolution = 6;

  this->BaseCenter[0] = 0.0;
  this->BaseCenter[1] = 0.0;
  this->BaseCenter[2] = 0.0;

  this->Direction[0] = 0.0;
  this->Direction[1] = 0.0;
  this->Direction[2] = 1.0;
  this->GenerateNormals = 1;
}

vtkCmbLayeredConeSource::~vtkCmbLayeredConeSource()
{
}

void vtkCmbLayeredConeSource::SetNumberOfLayers(int layers)
{
  this->LayerRadii.resize(layers);
  this->Modified();
}

int vtkCmbLayeredConeSource::GetNumberOfLayers()
{
  return this->LayerRadii.size();
}

void vtkCmbLayeredConeSource::SetTopRadius(int layer, double r1, double r2)
{
  this->LayerRadii[layer].TopRadii[0] = r1;
  this->LayerRadii[layer].TopRadii[1] = r2;
  this->Modified();
}

void vtkCmbLayeredConeSource::SetBaseRadius(int layer, double r1, double r2)
{
  this->LayerRadii[layer].BaseRadii[0] = r1;
  this->LayerRadii[layer].BaseRadii[1] = r2;
  this->Modified();
}

void vtkCmbLayeredConeSource::SetTopRadius(int layer, double radius)
{
  this->LayerRadii[layer].TopRadii[0] = this->LayerRadii[layer].TopRadii[1] = radius;
  this->Modified();
}

double vtkCmbLayeredConeSource::GetTopRadius(int layer)
{
  return this->LayerRadii[layer].TopRadii[0];
}

void vtkCmbLayeredConeSource::SetBaseRadius(int layer, double radius)
{
  this->LayerRadii[layer].BaseRadii[0] = this->LayerRadii[layer].BaseRadii[1] = radius;
  this->Modified();
}

#define ADD_DATA(IN)                         \
if (this->GenerateNormals)                   \
{                                            \
  vtkNew<vtkPolyDataNormals> normals;        \
  normals->SetInputDataObject(IN);           \
  normals->ComputePointNormalsOn();          \
  normals->Update();                         \
  output->SetBlock(i, normals->GetOutput()); \
}                                            \
else                                         \
{                                            \
  output->SetBlock(i, IN);                   \
}

double vtkCmbLayeredConeSource::GetBaseRadius(int layer)
{
  return this->LayerRadii[layer].BaseRadii[0];
}

int vtkCmbLayeredConeSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get multi-block data set
  vtkMultiBlockDataSet *output =
    vtkMultiBlockDataSet::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT())
    );

  // create and add each layer to the output
  output->SetNumberOfBlocks(this->GetNumberOfLayers());
  int innerRes = 0;
  int outerRes = this->Resolution;
  double * innerBottomR = NULL;
  double * innerTopR = NULL;
  double * outerBottomR = NULL;
  double * outerTopR = NULL;

  vtkTransform *t = vtkTransform::New();
  t->Translate(this->BaseCenter[0], this->BaseCenter[1], this->BaseCenter[2]);
  if( this->Direction[0] != 0.0 || this->Direction[1] != 0.0 || this->Direction[2] != 1.0 )
    {
    assert(!"Change in direction is currently not supported");
    }
  vtkNew<vtkTransformPolyDataFilter> filter;
  filter->SetTransform(t);
  t->Delete();

  for(int i = 0; i < this->GetNumberOfLayers(); i++)
    {
    outerBottomR = this->LayerRadii[i].BaseRadii;
    outerTopR = this->LayerRadii[i].TopRadii;
    vtkPolyData * tmpLayer = CreateLayer( this->Height,
                                          innerBottomR, outerBottomR,
                                          innerTopR,    outerTopR,
                                          innerRes,     outerRes );
    innerBottomR = outerBottomR;
    innerTopR    = outerTopR;
    innerRes     = outerRes;
    if(tmpLayer == NULL)
    {
      output->SetBlock(i, tmpLayer);
      continue;
    }

    filter->SetInputDataObject(tmpLayer);
    if(tmpLayer != NULL) tmpLayer->Delete();
    filter->Update();
    if (this->GenerateNormals)
      {
      vtkNew<vtkPolyDataNormals> normals;
      normals->SetInputConnection(filter->GetOutputPort());
      normals->ComputePointNormalsOn();
      normals->Update();
      output->SetBlock(i, normals->GetOutput());
      }
    else
      {
      output->SetBlock(i, filter->GetOutput());
      }
    }

  return 1;
}

#define CREATE_END(OFFSET)                       \
if(innerRes == 0)                                \
{                                                \
  for(int i = 0; i < outerRes; ++i)              \
  {                                              \
    pts[i] = i+OFFSET;                           \
  }                                              \
  cells->InsertNextCell(outerRes, pts);          \
}                                                \
else if(innerRes == outerRes)                    \
{                                                \
  for(int i = 0; i < innerRes; ++i)              \
  {                                              \
    pts[0] = i + OFFSET;                         \
    pts[1] = (i+1)%innerRes + OFFSET;            \
    pts[2] = (i+1)%innerRes + innerRes + OFFSET; \
    pts[3] = i+innerRes + OFFSET;                \
    cells->InsertNextCell(4, pts);               \
  }                                              \
}                                                \

vtkPolyData *
vtkCmbLayeredConeSource
::CreateLayer( double h,
               double * innerBottomR, double * outerBottomR,
               double * innerTopR,    double * outerTopR,
               int innerRes, int outerRes )
{
  if(outerTopR == NULL || outerBottomR == NULL) return NULL;
  vtkPolyData *polyData = vtkPolyData::New();
  polyData->Allocate();

  vtkPoints *points = vtkPoints::New();
  vtkCellArray *cells = vtkCellArray::New();

  vtkIdType pts[VTK_CELL_SIZE];

  points->SetDataTypeToDouble(); //used later during transformation
  points->Allocate((innerRes + outerRes)*2);

  //Points are order (OuterBottom, InnerBottom, OuterTop, InnerTop)
  AddPoints(points, 0, outerBottomR, outerRes);
  AddPoints(points, 0, innerBottomR, innerRes, 0.0005);
  AddPoints(points, h, outerTopR,    outerRes);
  AddPoints(points, h, innerTopR,    innerRes, 0.0005);



  //Add bottom calls
  if(1) { CREATE_END(0) }
  //Create top
  if(1) { const int offset = outerRes+innerRes; CREATE_END(offset) }
  //Outer Wall;
  if(1)
  {
    int offset = outerRes+innerRes;
    for(unsigned int i = 0; i < outerRes; ++i)
    {
      pts[0] = i;
      pts[1] = (i+1) % outerRes;
      pts[2] = (i+1) % outerRes + offset;
      pts[3] = i + offset;
      cells->InsertNextCell(4, pts);
    }
  }
  //Inner Wall
  if(1)
  {
    int offset = outerRes+innerRes;
    for(unsigned int i = 0; i < innerRes; ++i)
    {
      pts[0] = i + outerRes;
      pts[1] = (i+1) % innerRes + outerRes;
      pts[2] = (i+1) % innerRes + outerRes+ offset;
      pts[3] = i + outerRes + offset;
      cells->InsertNextCell(4, pts);
    }
  }
  polyData->SetPoints(points);
  polyData->SetPolys(cells);

  points->Delete();
  cells->Delete();

  return polyData;
}

void
vtkCmbLayeredConeSource
::AddPoints(vtkPoints *points, double h, double * r, int res, double shift)
{
  double point[3];
  point[2] = h;
  if(res == 4)
  {
    point[0] = -r[0];
    point[1] = -r[1];
    points->InsertNextPoint(point);
    point[0] =  r[0];
    point[1] = -r[1];
    points->InsertNextPoint(point);
    point[0] =  r[0];
    point[1] =  r[1];
    points->InsertNextPoint(point);
    point[0] = -r[0];
    point[1] =  r[1];
    points->InsertNextPoint(point);
    return;
  }
  double angle = 2.0*3.141592654/res;
  for(int j = 0; j < res; j++)
  {
    point[0] = (r[0] + shift) * cos(j * angle);
    point[1] = (r[1] + shift) * sin(j * angle);
    points->InsertNextPoint(point);
  }
}

void vtkCmbLayeredConeSource::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
