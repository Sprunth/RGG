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

#define ASSY_NOT_SET_VALUE -100001
#define ASSY_NOT_SET_KEY "NotSet"

class cmbAssyParameters
{
public:
  cmbAssyParameters()
  {
  this->Geometry =  this->GeometryType = this->GeomEngine = this->MeshType
    = this->RotateXYZ = this->CenterXYZ = this->CreateSideSetYesNo
    = this->InfoOnOff = this->SectionXYZ = this->MoveXYZ = ASSY_NOT_SET_KEY;
  this->RotateAngle = this->RadialMeshSize = this->AxialMeshSize
    = this->TetMeshSize = this->NeumannSet_StartId = this->MaterialSet_StartId
    = this->EdgeInterval = this->MergeTolerance = this->CreateFiles
    = this->SectionOffset = ASSY_NOT_SET_VALUE;
  this->SectionReverse = false;
  }

  static bool isKeySet(const std::string& key)
  { return key != ASSY_NOT_SET_KEY; }
  template<typename T> static bool isValueSet(const T& val)
  { return val != ASSY_NOT_SET_VALUE; }

  // Geometry     {Volume | Surface}
  std::string Geometry;
  // GeometryType {Hexagonal | Rectangular}
  std::string GeometryType;
  // GeomEngine   {ACIS | OCC}
  std::string GeomEngine;
  // MeshType     {Hex | Tet}
  std::string MeshType;

  // [TetMeshSize <size>]
  double TetMeshSize;
  // [RadialMeshSize <size>]
  double RadialMeshSize;
  // [AxialMeshSize <size>]
  double AxialMeshSize;
  // [Rotate {x | y | z} <angle>]
  std::string RotateXYZ;
  double RotateAngle;
  // [Center] {x | y | z}
  std::string CenterXYZ;
  // [NeumannSet_StartId <value>]
  int NeumannSet_StartId;
  // [MaterialSet_StartId <value>]
  int MaterialSet_StartId;
  // [EdgeInterval <value>]
  int EdgeInterval;
  // [MergeTolerance <value>]
  double MergeTolerance;
  // [CreateSideSet {Yes | No}]
  std::string CreateSideSetYesNo;
  // [Info {On | off}]
  std::string InfoOnOff;
  // [CreateFiles <block-number-shifted>]
  int CreateFiles;
  // [Section {x | y | z} <offset> [reverse] ]
  std::string SectionXYZ;
  int SectionOffset;
  bool SectionReverse;
  // [Move <x> <y> <z> ]
  std::string MoveXYZ;
};

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

  cmbAssyParameters* GetParameters() {return this->Parameters;}

  // Expose assembly parts for UI access
  std::vector<PinCell*> PinCells;
  DuctCell AssyDuct;
  Lattice AssyLattice;
  std::string label;

  std::string FileName;

protected :

  cmbAssyParameters *Parameters;

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
