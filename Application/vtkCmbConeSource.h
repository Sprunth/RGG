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
// .NAME vtkCmbConeSource - generate polygonal conical shell
// .SECTION Description
// vtkCmbConeSource creates a cone centered at a specified point and pointing in
// a specified direction. The BaseCenter defines the center of cone's Base and
// the BaseRadius defines the radius of base.  The Height and Direction defines
// the position of the "top" of the conical shell.  If the TopRadius is set to be 
// 0, the shell will come to a point at the top, else it will have a circular area.

#ifndef __vtkCmbConeSource_h
#define __vtkCmbConeSource_h

#include "vtkPolyDataAlgorithm.h"

#include "vtkCell.h" // Needed for VTK_CELL_SIZE

class vtkCmbConeSource : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkCmbConeSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with default resolution 6, height 1.0, radius 0.5, and
  // capping on. The base cone is centered at the origin and points down
  // the x-axis.
  static vtkCmbConeSource *New();

  // Description:
  // Set the height of the cone. This is the height along the cone in
  // its specified direction.
  vtkSetClampMacro(Height,double,0.0,VTK_DOUBLE_MAX)
  vtkGetMacro(Height,double);

  // Description:
  // Set the base radius of the cone.
  vtkSetClampMacro(BaseRadius,double,0.0,VTK_DOUBLE_MAX)
  vtkGetMacro(BaseRadius,double);

  // Description:
  // Set the top radius of the cone.
  vtkSetClampMacro(TopRadius,double,0.0,VTK_DOUBLE_MAX)
  vtkGetMacro(TopRadius,double);

  // Description:
  // Set the number of facets used to represent the cone.
  vtkSetClampMacro(Resolution,int,0,VTK_CELL_SIZE)
  vtkGetMacro(Resolution,int);

  // Description:
  // Set the base center of the cone.
  // The default is 0,0,0.
  vtkSetVector3Macro(BaseCenter,double);
  vtkGetVectorMacro(BaseCenter,double,3);

  // Description:
  // Set the orientation vector of the cone. The vector does not have
  // to be normalized. The direction goes from the center of the base toward
  // the apex. The default is (1,0,0).
  vtkSetVector3Macro(Direction,double);
  vtkGetVectorMacro(Direction,double,3);

  // Description:
  // Turn on/off whether to cap the cone with a polygon.
  vtkSetMacro(Capping,int);
  vtkGetMacro(Capping,int);
  vtkBooleanMacro(Capping,int);

protected:
  vtkCmbConeSource(int res=6);
  ~vtkCmbConeSource() {}

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  double Height;
  double BaseRadius;
  double TopRadius;
  int Resolution;
  int Capping;
  double BaseCenter[3];
  double Direction[3];
  
private:
  vtkCmbConeSource(const vtkCmbConeSource&);  // Not implemented.
  void operator=(const vtkCmbConeSource&);  // Not implemented.
};

#endif


