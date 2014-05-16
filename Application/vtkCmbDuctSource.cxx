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
#include "vtkCmbDuctSource.h"

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
#include "vtkCmbLayeredConeSource.h"
#include "vtkSmartPointer.h"

#include <math.h>

vtkStandardNewMacro(vtkCmbDuctSource);

//----------------------------------------------------------------------------
vtkCmbDuctSource::vtkCmbDuctSource()
{
  std::fill(this->Origin, this->Origin + 3, 0.0);
  this->Height = 0.0;
  this->SetNumberOfInputPorts(0);
  this->GeometryType = RECTILINEAR;
}

//----------------------------------------------------------------------------
vtkCmbDuctSource::~vtkCmbDuctSource()
{
}

//----------------------------------------------------------------------------
int vtkCmbDuctSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get multi-block data set
  vtkMultiBlockDataSet *output =
    vtkMultiBlockDataSet::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int numLayers = this->GetNumberOfLayers();
  // get parameters for the layer
  double x = this->Origin[0];
  double y = this->Origin[1];
  double z = this->Origin[2];
  vtkSmartPointer<vtkCmbLayeredConeSource> coneSource =
  vtkSmartPointer<vtkCmbLayeredConeSource>::New();
  coneSource->SetNumberOfLayers(numLayers);
  coneSource->SetBaseCenter(x, y, z);
  double direction[] = { 0, 0, 1 };
  coneSource->SetDirection(direction);
  coneSource->SetHeight(this->Height);

  // create HEX duct
  if(this->GeometryType == HEXAGONAL)
    {
    double preDist = 0;
    for(int k = 0; k < numLayers; k++)
      {
      double distance = this->Layers[2*k]/2.0;
      double radius = distance / (double)(cos(30.0 * vtkMath::Pi() / 180.0));
      coneSource->SetBaseRadius(k, radius);
      coneSource->SetTopRadius(k, radius);
      coneSource->SetResolution(k, 6);
      preDist = distance;
      }
    }
  else // create Rect duct
    {
    for(int k = 0; k < numLayers; k++)
      {
      double w = this->Layers[k*2+0];
      double l = this->Layers[k*2+1];
      coneSource->SetBaseRadius(k, w*0.5, l*0.5);
      coneSource->SetTopRadius(k, w*0.5, l*0.5);
      coneSource->SetResolution(k, 4);
      }
    }
  coneSource->Update();
  output->ShallowCopy(coneSource->GetOutput());

  return 1;
}

//----------------------------------------------------------------------------
int vtkCmbDuctSource::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
//  vtkInformation *outInfo = outputVector->GetInformationObject(0);
//  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkCmbDuctSource::AddLayer(double x, double y)
{
  this->Layers.push_back(x);
  this->Layers.push_back(y);
}

//----------------------------------------------------------------------------
int vtkCmbDuctSource::GetNumberOfLayers()
{
  return this->Layers.size() / 2;
}

//----------------------------------------------------------------------------
void vtkCmbDuctSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
