
#include "vtkCmbLayeredConeSource.h"

#include <vtkObjectFactory.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>

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
    cone->SetCapping(i == 0);
    cone->SetBaseRadius(this->GetBaseRadius(i));
    cone->SetTopRadius(this->GetTopRadius(i));
    cone->SetHeight(this->Height);
    cone->SetBaseCenter(this->BaseCenter);
    cone->SetResolution(this->Resolution);
    cone->SetDirection(this->Direction);
    cone->Update();
    output->SetBlock(i, cone->GetOutput());
    cone->Delete();
    }

  return 1;
}

void vtkCmbLayeredConeSource::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
