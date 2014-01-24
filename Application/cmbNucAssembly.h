#ifndef cmbNucAssembly_H
#define cmbNucAssembly_H

#include <string>
#include <vector>
#include <QColor>

#include "vtkMultiBlockDataSet.h"
#include "cmbNucPartDefinition.h"
#include "vtkSmartPointer.h"
#include "vtkPolyData.h"

class vtkCompositeDataDisplayAttributes;

// Represents an assembly. Assemblies are composed of pin cells (cmbNucPinCell)
// and the surrounding ducting. Assemblies can be loaded and stored to files
// with the ReadFile() and Write() file methods. Assemblies are grouped together
// into cores (cmbNucCore).
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

  // Call this when a pin cell property changes, it will rebuild the lattice
  void UpdateGrid();

  // Returns the pincell with label. Returns 0 if no pincell with
  // label exists.
  PinCell* GetPinCell(const std::string &label);

  // Reads an assembly from a ".inp" file.
  void ReadFile(const std::string &FileName);

  // Writes the assembly to a ".inp" file.
  void WriteFile(const std::string &FileName);

  // The color to use to represent this assembly type in the lattice editor
  QColor GetLegendColor() const;
  void SetLegendColor(const QColor& color);

  // updates the block colors based on their materials
  void updateMaterialColors(unsigned int& realflatidx,
    vtkCompositeDataDisplayAttributes *attributes);

  // Returns a multi-block data set containing the geometry for
  // the assembly. This is used to render the assembly in 3D.
  vtkSmartPointer<vtkMultiBlockDataSet> GetData();

  // Remove a material by name, which will also update the material
  // assignment of the ducts and pincells inside this assembly
  void RemoveMaterial(const std::string &name);

  // creates the multiblock used to render the pincell. if cutaway is true the
  // pincell will be cut in half length-wise to show the interior layers.
  static vtkMultiBlockDataSet* CreatePinCellMultiBlock(PinCell *pincell, bool cutaway = false);

  // Expose assembly parts for UI access
  std::vector<PinCell*> PinCells;
  DuctCell AssyDuct;
  Lattice AssyLattice;
  std::string label;
  double MeshSize;
  std::string FileName;

protected :

  // For Hex geometry
  double RadialMeshSize;
  double AxialMeshSize;
  std::string  RotateDirection;
  double RotateAngle;

  friend class cmbNucMainWindow;

  vtkSmartPointer<vtkMultiBlockDataSet> CreateData();

private:
  std::string GeometryType;
  vtkSmartPointer<vtkMultiBlockDataSet> Data;
  QColor LegendColor;

  // Check if GeometryType is Hexagonal
  bool IsHexType();

};

#endif // cmbNucAssembly_H
