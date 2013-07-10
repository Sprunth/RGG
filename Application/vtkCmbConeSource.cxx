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
#include "vtkCmbConeSource.h"

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

vtkStandardNewMacro(vtkCmbConeSource);

//----------------------------------------------------------------------------
// Construct with default resolution 6, height 1.0, radius 0.5, and capping
// on.
vtkCmbConeSource::vtkCmbConeSource(int res)
{
  res = (res < 3 ? 3 : res);
  this->Resolution = res;
  this->Height = 1.0;
  this->BaseRadius = 0.5;
  this->TopRadius = 0.0;
  this->Capping = 1;

  this->BaseCenter[0] = 0.0;
  this->BaseCenter[1] = 0.0;
  this->BaseCenter[2] = 0.0;
  
  this->Direction[0] = 1.0;
  this->Direction[1] = 0.0;
  this->Direction[2] = 0.0;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
int vtkCmbConeSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  double angle;
  int numLines, numPolys, numPts;
  double x[3], xbot;
  int i;
  vtkIdType pts[VTK_CELL_SIZE];
  vtkPoints *newPoints; 
  vtkCellArray *newLines=0;
  vtkCellArray *newPolys=0;
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  // for streaming
  int piece;
  int numPieces;
  int maxPieces;
  int start, end;
  int createBottom;
  int createTop;
  int numCaps = (BaseRadius > 0.0);
  numCaps += (TopRadius > 0.0);
  int res = (this->Resolution < 3) ? 3 : this->Resolution;
  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  if (piece >= res && !(piece == 0 && res == 0))
    {
    return 1;
    }
  numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  maxPieces = res;
  if (numPieces > maxPieces)
    {
    numPieces = maxPieces;
    }
  if (piece >= maxPieces)
    {
    // Super class should do this for us, 
    // but I put this condition in any way.
    return 1;
    }
  start = maxPieces * piece / numPieces;
  end = (maxPieces * (piece+1) / numPieces) - 1;
  createBottom = (this->Capping && (start == 0) && (BaseRadius > 0.0));
  createTop = (this->Capping && (start == 0) && (TopRadius > 0.0));
  
  vtkDebugMacro("CmbConeSource Executing");
  
  angle = 2.0*3.141592654/res;

  int createCaps = createBottom + createTop;

  // Are we dealing with the degenerate case of both
  // radii  == 0
  if (!numCaps)
    {
    if (!piece)
      {
      }
    return 1;
    }

  // Set things up; allocate memory
  //

  // Are we dealing with a trucated cone?
  if (numCaps == 2)
    {
    // Are we creating endcaps
    if (createCaps)
      {
      // piece 0 has  2caps.
      numPts = (res * 2);
      numPolys =  2 * (end - start + 2);
      }
    else
      {
      numPts = 2 *(end - start + 2);
      numPolys = 2 *(end - start + 1);
      }
    }
  else  if (createCaps == 1)
    {
    // piece 0 has 1 cap.
    numPts = res + 1;
    numPolys = end - start + 2;
    }
  else
    {
    numPts = end - start + 3;
    numPolys = end - start + 1;
    }
  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(numPolys,res));
  newPoints = vtkPoints::New();
  newPoints->SetDataTypeToDouble(); //used later during transformation
  newPoints->Allocate(numPts);
  
  // Create cone
  //
  // Are we dealing with a tuncated cone?
  if (numCaps == 2)
    {
    if (createCaps)
      {
      for (i=0; i < res; i++) 
        {
        x[0] = 0.0;
        x[1] = this->BaseRadius * cos (i*angle);
        x[2] = this->BaseRadius * sin (i*angle);
        // Reverse the order
        pts[res - i - 1] = newPoints->InsertNextPoint(x);
        }
      newPolys->InsertNextCell(res,pts);

      for (i=0; i < res; i++) 
        {
        x[0] = this->Height;
        x[1] = this->TopRadius * cos (i*angle);
        x[2] = this->TopRadius * sin (i*angle);
        // Reverse the order
        pts[i] = newPoints->InsertNextPoint(x);
        }
      newPolys->InsertNextCell(res,pts);
      }
      
    if ( ! createCaps)
      {
      // we need to create the points also
      x[0] = this->Height;
      x[1] = this->TopRadius * cos (start*angle);
      x[2] = this->TopRadius * sin (start*angle);
      pts[0] = newPoints->InsertNextPoint(x);
      x[0] = 0.0;
      x[1] = this->BaseRadius * cos (start*angle);
      x[2] = this->BaseRadius * sin (start*angle);
      pts[1] = newPoints->InsertNextPoint(x);

      for (i = start; i <= end; ++i)
        {
        x[1] = this->BaseRadius * cos ((i+1)*angle);
        x[2] = this->BaseRadius * sin ((i+1)*angle);
        pts[2] = newPoints->InsertNextPoint(x);
        newPolys->InsertNextCell(3,pts);
        pts[1] = pts[2];
        x[0] = this->Height;
        x[1] = this->TopRadius * cos ((i+1)*angle);
        x[2] = this->TopRadius * sin ((i+1)*angle);
        pts[2] = newPoints->InsertNextPoint(x);
        newPolys->InsertNextCell(3,pts);
        pts[0] = pts[2];
        }
      }
    else
      {
      // bottom and points have already been created.
      for (i=start; i <= end; i++) 
        {
        pts[0] = res+i;
        pts[1] = i;
        pts[2] = i+1;
        if (pts[2] >= res)
          {
          pts[2] = 0;
          pts[3] = res;
          }
        else
          {
          pts[3] = res + i + 1;
          }
        newPolys->InsertNextCell(3,pts);
        pts[1] = pts[2];
        pts[2] = pts[3];
        newPolys->InsertNextCell(3,pts);
        }
      } // createCaps
    }
  else if (this->BaseRadius > 0.0)
    {
    x[0] = this->Height; // zero-centered
    x[1] = 0.0;
    x[2] = 0.0;
    pts[0] = newPoints->InsertNextPoint(x);
    
    xbot = 0.0;


  // General case: create Resolution triangles and single cap
    // create the bottom.
    if ( createBottom )
      {
      for (i=0; i < res; i++) 
        {
        x[0] = 0.0;
        x[1] = this->BaseRadius * cos (i*angle);
        x[2] = this->BaseRadius * sin (i*angle);
        // Reverse the order
        pts[res - i - 1] = newPoints->InsertNextPoint(x);
        }
      newPolys->InsertNextCell(res,pts);
      }
    
    pts[0] = 0;
    if ( ! createBottom)
      {
      // we need to create the points also
      x[0] = xbot;
      x[1] = this->BaseRadius * cos (start*angle);
      x[2] = this->BaseRadius * sin (start*angle);
      pts[1] = newPoints->InsertNextPoint(x);
      for (i = start; i <= end; ++i)
        {
        x[1] = this->BaseRadius * cos ((i+1)*angle);
        x[2] = this->BaseRadius * sin ((i+1)*angle);
        pts[2] = newPoints->InsertNextPoint(x);
        newPolys->InsertNextCell(3,pts);
        pts[1] = pts[2];
        }
      }
    else
      {
      // bottom and points have already been created.
      for (i=start; i <= end; i++) 
        {
        pts[1] = i+1;
        pts[2] = i+2;
        if (pts[2] > res)
          {
          pts[2] = 1;
          }
        newPolys->InsertNextCell(3,pts);
        }
      } // createBottom
    }

  // A non-default origin and/or direction requires transformation
  //
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
    double *ipts=
      static_cast<vtkDoubleArray *>(newPoints->GetData())->GetPointer(0);
    for (i=0; i<numPts; i++, ipts+=3)
      {
      t->TransformPoint(ipts,ipts);
      }
    
    t->Delete();
    }
  
  // Update ourselves
  //
  output->SetPoints(newPoints);
  newPoints->Delete();

  if ( newPolys )
    {
    newPolys->Squeeze(); // we may have estimated size; reclaim some space
    output->SetPolys(newPolys);
    newPolys->Delete();
    }
  else
    {
    output->SetLines(newLines);
    newLines->Delete();
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkCmbConeSource::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
               -1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkCmbConeSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Height: " << this->Height << "\n";
  os << indent << "BaseRadius: " << this->BaseRadius << "\n";
  os << indent << "TopRadius: " << this->TopRadius << "\n";
  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "BaseCenter: (" << this->BaseCenter[0] << ", " 
     << this->BaseCenter[1] << ", " << this->BaseCenter[2] << ")\n";
  os << indent << "Direction: (" << this->Direction[0] << ", " 
     << this->Direction[1] << ", " << this->Direction[2] << ")\n";
}
