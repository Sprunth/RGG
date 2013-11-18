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
#ifndef __cmbNucHexDuctSource_h
#define __cmbNucHexDuctSource_h

#include <vector>

#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkPolyData;

class cmbNucHexDuctSource : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(cmbNucHexDuctSource, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static cmbNucHexDuctSource *New();

  // Description:
  // Set/get the pitch of the duct.
  vtkSetMacro(Pitch, double)
  vtkGetMacro(Pitch, double)

  // Description:
  // Set/get the height of the duct.
  vtkSetMacro(Height, double)
  vtkGetMacro(Height, double)

  // Description:
  // Set the origin of the duct. The default is 0,0,0.
  vtkSetVector3Macro(Origin, double)
  vtkGetVectorMacro(Origin, double, 3)

  void AddLayer(double thickness);
  int GetNumberOfLayers();

protected:
  cmbNucHexDuctSource();
  ~cmbNucHexDuctSource();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  vtkPolyData* CreateLayer(int layer);

  double Pitch;
  double Height;
  double Origin[3];

  std::vector<double> Layers;

private:
  cmbNucHexDuctSource(const cmbNucHexDuctSource&);  // Not implemented.
  void operator=(const cmbNucHexDuctSource&);  // Not implemented.
};

#endif
