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
#ifndef __vtkCmbDuctSource_h
#define __vtkCmbDuctSource_h

#include <vector>

#include "vtkMultiBlockDataSetAlgorithm.h"

#include "cmbNucPartDefinition.h"
class vtkPolyData;

// The vtkCmbDuctSource is a VTK algorithm which produces a multi-block data-set
// containing vtkPolyData for each of the layers in a rectangular duct. The height
// and origin of the whole duct can be modified. Layers can be added by specifying
// their thickness in the X and Y directions.
class vtkCmbDuctSource : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkCmbDuctSource, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkCmbDuctSource *New();

  // Description:
  // Set/get the height of the duct.
  vtkSetMacro(Height, double)
  vtkGetMacro(Height, double)

  // Description:
  // Set the origin of the duct. The default is 0,0,0.
  vtkSetVector3Macro(Origin, double)
  vtkGetVectorMacro(Origin, double, 3)

  // Add a new layer with thicknesses x and y. For Hexagonal duct,
  // only one parameter (distance) is needed.  Ducts should be
  // built up starting at the inner most layer and moving to the
  // outermost layer.
  void AddLayer(double x, double y);
  void AddLayer(double distance) // For Hex duct
  { this->AddLayer(distance, distance); }

  // Returns the number of layers in the duct.
  int GetNumberOfLayers();

  // Description:
  // Set the type of the duct.
  vtkSetClampMacro(GeometryType,int,RECTILINEAR,HEXAGONAL)
  vtkGetMacro(GeometryType,int);

protected:
  vtkCmbDuctSource();
  ~vtkCmbDuctSource();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  double Height;
  double Origin[3];
  std::vector<double> Layers;
  int GeometryType;

private:
  vtkCmbDuctSource(const vtkCmbDuctSource&);  // Not implemented.
  void operator=(const vtkCmbDuctSource&);  // Not implemented.
};

#endif
