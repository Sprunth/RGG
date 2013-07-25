#ifndef cmbNucAssembly_H
#define cmbNucAssembly_H

#include <string>
#include <vector>

#include "vtkMultiBlockDataSet.h"
#include "cmbNucPartDefinition.h"
#include "vtkSmartPointer.h"
#include "vtkPolyData.h"

class cmbNucAssembly : public AssyPartObj
{
public:

  // Creates an empty assembly.
  cmbNucAssembly();

  // Destroys the assembly.
  ~cmbNucAssembly();

  virtual enumNucPartsType GetType() {return CMBNUC_ASSEMBLY;}

  // Adds a new pincell to the assebly. After adding the pincell it
  // can be placed in the assembly with the SetCell() method.
  void AddPinCell(PinCell *pincell);

  // Remove the pincell with label from the assembly.
  void RemovePinCell(const std::string &label);

  // Returns the pincell with label. Returns 0 if no pincell with
  // label exists.
  PinCell* GetPinCell(const std::string &label);

  // Reads an assembly from a ".inp" file.
  void ReadFile(const std::string &FileName);

  // Writes the assembly to a ".inp" file.
  void WriteFile(const std::string &FileName);

  // Returns a multi-block data set containing the geometry for
  // the assembly. This is used to render the assembly in 3D.
  vtkSmartPointer<vtkMultiBlockDataSet> GetData();

  // Adds/Remove a material by name
  void AddMaterial(Material *material);
  void RemoveMaterial(const std::string &name);

  // Expose assembly parts for UI access
  std::vector<PinCell*> PinCells;
  DuctCell AssyDuct;
  std::vector<Material*> Materials;
  Lattice AssyLattice;
  std::string label;
  double MeshSize;

private:
  // creates the polydata used to render the pincell
  vtkPolyData* CreatePinCellPolyData(PinCell *pincell);

private:
  std::string GeometryType;
  vtkSmartPointer<vtkMultiBlockDataSet> Data;
};

#endif // cmbNucAssembly_H
