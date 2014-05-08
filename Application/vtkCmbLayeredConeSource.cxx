
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

#include "vtkCmbConeSource.h"

vtkStandardNewMacro(vtkCmbLayeredConeSource);

vtkCmbLayeredConeSource::vtkCmbLayeredConeSource()
{
  this->SetNumberOfInputPorts(0);

  this->Height = 1.0;
  this->Resolution = 6;

  this->BaseCenter[0] = 0.0;
  this->BaseCenter[1] = 0.0;
  this->BaseCenter[2] = 0.0;

  this->Direction[0] = 1.0;
  this->Direction[1] = 0.0;
  this->Direction[2] = 0.0;
  this->GenerateNormals = 1;
}

vtkCmbLayeredConeSource::~vtkCmbLayeredConeSource()
{
}

void vtkCmbLayeredConeSource::SetNumberOfLayers(int layers)
{
  this->TopRadii.resize(layers);
  this->BaseRadii.resize(layers);
  this->Modified();
}

int vtkCmbLayeredConeSource::GetNumberOfLayers()
{
  return this->TopRadii.size();
}

void vtkCmbLayeredConeSource::SetTopRadius(int layer, double radius)
{
  this->TopRadii[layer] = radius;
  this->Modified();
}

double vtkCmbLayeredConeSource::GetTopRadius(int layer)
{
  return this->TopRadii[layer];
}

void vtkCmbLayeredConeSource::SetBaseRadius(int layer, double radius)
{
  this->BaseRadii[layer] = radius;
  this->Modified();
}

double vtkCmbLayeredConeSource::GetBaseRadius(int layer)
{
  return this->BaseRadii[layer];
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

  for(int i = 0; i < this->GetNumberOfLayers(); i++)
    {
    vtkCmbConeSource *cone = vtkCmbConeSource::New();
    cone->SetBaseRadius(this->GetBaseRadius(i));
    cone->SetTopRadius(this->GetTopRadius(i));
    cone->SetHeight(this->Height);
    cone->SetBaseCenter(this->BaseCenter);
    cone->SetResolution(this->Resolution);
    cone->SetDirection(this->Direction);

    if(i == 0)
      {
      cone->SetCapping(true);
      if (this->GenerateNormals)
        {
        vtkNew<vtkPolyDataNormals> normals;
        normals->SetInputConnection(cone->GetOutputPort());
        normals->ComputePointNormalsOn();
        normals->Update();
        output->SetBlock(i, normals->GetOutput());
        }
      else
        {
        cone->Update();
        output->SetBlock(i, cone->GetOutput());
        }
      cone->Delete();
      }
    else
      {
      // create cap for layer
      double angle = 2.0*3.141592654/this->Resolution;

      vtkAppendPolyData *merger = vtkAppendPolyData::New();
      cone->SetCapping(false);
      cone->Update();
      merger->AddInputData(cone->GetOutput());
      cone->Delete();
      vtkTransform *rotate = vtkTransform::New();
      rotate->RotateZ(30);

      double heights[] = { 0, this->Height };
      for(int height = 0; height < 2; height++)
        {
        double h = heights[height];
        double r1 = height == 0 ? this->GetBaseRadius(i-1) : this->GetTopRadius(i-1);
        double r2 = height == 0 ? this->GetBaseRadius(i) : this->GetTopRadius(i);

        vtkPolyData *cap = vtkPolyData::New();
        cap->Allocate();
        vtkPoints *points = vtkPoints::New();
        vtkCellArray *cells = vtkCellArray::New();

        // add base cap points
        double point[3];
        for(int j = 0; j < this->Resolution; j++)
          {
          point[0] = h;
          point[1] = r1 * cos(j * angle);
          point[2] = r1 * sin(j * angle);
          points->InsertNextPoint(point);
          }
        for(int j = 0; j < this->Resolution; j++)
          {
          point[0] = h;
          point[1] = r2 * cos(j * angle);
          point[2] = r2 * sin(j * angle);
          points->InsertNextPoint(point);
          }

        // add cells
        for(int j = 0; j < this->Resolution; j++)
          {
          int res = this->Resolution;
          vtkIdType cell[] = { j, (j + 1) % res, (j + 1) % res + res, j + res };
          cells->InsertNextCell(4, cell);
          }

        cap->SetPoints(points);
        cap->SetPolys(cells);
        points->Delete();
        cells->Delete();

        // A non-default origin and/or direction requires transformation
        if ( this->BaseCenter[0] != 0.0 || this->BaseCenter[1] != 0.0 ||
             this->BaseCenter[2] != 0.0 || this->Direction[0] != 1.0 ||
             this->Direction[1] != 0.0 || this->Direction[2] != 0.0 )
          {
          vtkTransform *t = vtkTransform::New();
          t->Translate(this->BaseCenter[0], this->BaseCenter[1], this->BaseCenter[2]);
          double vMag = vtkMath::Norm(this->Direction);
          if ( this->Direction[0] < 0.0 )
            {
            // flip x -> -x to avoid instability
            t->RotateWXYZ(180.0, (this->Direction[0]-vMag)/2.0,
                          this->Direction[1]/2.0, this->Direction[2]/2.0);
            t->RotateWXYZ(180.0, 0, 1, 0);
            }
          else
            {
            t->RotateWXYZ(180.0, (this->Direction[0]+vMag)/2.0,
                          this->Direction[1]/2.0, this->Direction[2]/2.0);
            }
          vtkNew<vtkTransformPolyDataFilter> filter, filter2;
          filter->SetTransform(t);
          t->Delete();
          filter->SetInputDataObject(cap);
          cap->Delete();
          filter->Update();
          if(Resolution == 6)
            {
            t = vtkTransform::New();
            t->RotateZ(30);
            filter2->SetTransform(t);
            filter2->SetInputDataObject(filter->GetOutput());
            t->Delete();
            filter2->Update();
            merger->AddInputData(filter2->GetOutput());
            }
          else
            {
            merger->AddInputData(filter->GetOutput());
            }
          }
        else
          {
          merger->AddInputData(cap);
          cap->Delete();
          }
        }
      rotate->Delete();

      // merge all polydata
      if (this->GenerateNormals)
        {
        vtkNew<vtkPolyDataNormals> normals;
        normals->SetInputConnection(merger->GetOutputPort());
        normals->ComputePointNormalsOn();
        normals->Update();
        output->SetBlock(i, normals->GetOutput());
        }
      else
        {
        merger->Update();
        output->SetBlock(i, merger->GetOutput());
        }
      merger->Delete();
      }
    }

  return 1;
}

void vtkCmbLayeredConeSource::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
