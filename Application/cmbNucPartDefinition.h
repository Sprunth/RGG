#ifndef __cmbNucPartDefinition_h
#define __cmbNucPartDefinition_h

#include "vtkSmartPointer.h"
#include "vtkPolyData.h"

  struct Cylinder
  {
    double x;
    double y;
    double z1;
    double z2;
    double r;
    std::string material;
  };

  struct Frustum
  {
    double x;
    double y;
    double z1;
    double z2;
    double r1;
    double r2;
    std::string material;
  };

  struct PinCell
  {
    std::string name;
    std::string label;
    double pitchX;
    double pitchY;
    double pitchZ;
    std::vector<Cylinder> cylinders;
    std::vector<Frustum> frustums;

    vtkSmartPointer<vtkPolyData> polyData;
  };

  struct Duct
  {
    double x;
    double y;
    double z1;
    double z2;
    std::vector<std::string> materials;
    std::vector<double> thicknesses;
  };

  struct Material
  {
    std::string name;
    std::string label;
  };

enum enumNucParts
{
  ASSY_DUCT=0,
  ASSY_RECT_DUCT,
  ASSY_HEX_DUCT,
  ASSY_PINCELL,
  ASSY_CYLINDER_PIN,
  ASSY_FRUSTUM_PIN,
  ASSY_MATERIAL
};

#endif
