#ifndef cmbNucCore_H
#define cmbNucCore_H

#include <string>
#include <vector>

#include "vtkMultiBlockDataSet.h"
#include "cmbNucPartDefinition.h"
#include "vtkSmartPointer.h"

class cmbNucAssembly;

class cmbNucCore : public AssyPartObj
{
public:

  // Creates an empty Core.
  cmbNucCore();

  // Destroys the Core.
  ~cmbNucCore();

  virtual enumNucPartsType GetType() {return CMBNUC_CORE;}

  // Adds a new Assembly to the core. After adding the Assembly it
  // can be placed in the Core with the SetAssembly() method.
  void AddAssembly(cmbNucAssembly *assembly);

  // Remove the Assembly with label from the Core.
  void RemoveAssembly(const std::string &label);

  // Returns the Assembly with label or index.
  // Returns 0 if no Assembly with label or index exists.
  cmbNucAssembly* GetAssembly(const std::string &label);
  cmbNucAssembly* GetAssembly(int idx);

  // Return the number of assemblies in the core
  int GetNumberOfAssemblies()
    {return (int)this->Assemblies.size();}

  // Sets the dimensions of the Assembly Core.
  void SetDimensions(int i, int j)
    {
    this->Grid.resize(i);
    for(int k = 0; k < i; k++)
      {
      this->Grid[k].resize(j);
      }
    }
  // Returns the dimensions of the Assembly Core.
  std::pair<int, int> GetDimensions() const
    {
    return std::make_pair((int)this->Grid.size(), (int)this->Grid[0].size());
    }
  // Sets the contents of the Assembly (i, j) to name.
  void SetAssemblyLabel(int i, int j, const std::string &name)
    {
    this->Grid[i][j] = name;
    }
  // Returns the contents of the Assembly (i, j).
  std::string GetAssemblyLabel(int i, int j) const
    {
    return this->Grid[i][j];
    }
  // Clears the contents of the Assembly (i, j). This is equivalent
  // to calling SetAssembly(i, j, "xx").
  void ClearAssemblyLabel(int i, int j)
    {
    this->SetAssemblyLabel(i, j, "xx");
    }

  // Returns a multi-block data set containing the geometry for
  // the core with assemblies. This is used to render the core in 3D.
  vtkSmartPointer<vtkMultiBlockDataSet> GetData();

  // Get/Set Assembly pitch
  double AssyemblyPitchX;
  double AssyemblyPitchY;

private:
  std::vector<std::vector<std::string> > Grid;
  vtkSmartPointer<vtkMultiBlockDataSet> Data;
  std::vector<cmbNucAssembly*> Assemblies;

};

#endif // cmbNucCore_H
