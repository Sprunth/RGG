/*=========================================================================

Copyright (c) 1998-2012 Kitware Inc. 28 Corporate Drive,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced,
distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO
PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
=========================================================================*/
#include "cmbNucHexDuctSource.h"

#include "vtkMultiBlockDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPolyData.h"
#include "vtkTransform.h"
#include "vtkCellArray.h"

#include <math.h>

vtkStandardNewMacro(cmbNucHexDuctSource);

//----------------------------------------------------------------------------
cmbNucHexDuctSource::cmbNucHexDuctSource()
{
  std::fill(this->Origin, this->Origin + 3, 0.0);
  this->Height = 0.0;
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
cmbNucHexDuctSource::~cmbNucHexDuctSource()
{
}

//----------------------------------------------------------------------------
int cmbNucHexDuctSource::RequestData(
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
    vtkPolyData *polyData = this->CreateLayer(i);
    output->SetBlock(i, polyData);
    polyData->Delete();
    }

  return 1;
}

//----------------------------------------------------------------------------
int cmbNucHexDuctSource::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
//  vtkInformation *outInfo = outputVector->GetInformationObject(0);
//  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);
  return 1;
}

//----------------------------------------------------------------------------
vtkPolyData* cmbNucHexDuctSource::CreateLayer(int layer)
{
  // get parameters for the layer
  double x = this->Origin[0];
  double y = this->Origin[1];
  double z = this->Origin[2];

  // pitch (p), height (h)
  double p = this->Pitch;
  double h = this->Height;
  if (layer == 0)
      {
      // For the inner most layer lets reduce the h by 0.1% so
      // we don't completely cover the tops of the pins
      h *= 0.999;
      // Move the Origin up in z by 0.05 % of the the Height
      z += this->Height * 0.0005;
      }

  // adjust layer origin and size based on the sizes of the previous layers
  //x += this->Layers[this->Layers.size()-2] - w;
  //y += this->Layers[this->Layers.size()-1] - l;

  //w -= this->Layers[this->Layers.size()-2] - w;
  //l -= this->Layers[this->Layers.size()-1] - l;

  vtkPolyData *polyData = vtkPolyData::New();
  polyData->Allocate();

  vtkPoints *points = vtkPoints::New();
  vtkCellArray *cells = vtkCellArray::New();

  polyData->SetPoints(points);
  polyData->SetPolys(cells);

  points->Delete();
  cells->Delete();

  return polyData;
}

//----------------------------------------------------------------------------
void cmbNucHexDuctSource::AddLayer(double thickness)
{
  this->Layers.push_back(thickness);
}

//----------------------------------------------------------------------------
int cmbNucHexDuctSource::GetNumberOfLayers()
{
  return this->Layers.size();
}

//----------------------------------------------------------------------------
void cmbNucHexDuctSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
