#ifndef cmbNucCore_H
#define cmbNucCore_H

#include <string>
#include <vector>
#include <QColor>
#include <QObject>

#include "vtkMultiBlockDataSet.h"
#include "cmbNucPartDefinition.h"
#include "vtkSmartPointer.h"

class cmbNucAssembly;
class inpFileReader;
class inpFileHelper;
class inpFileWriter;

#define EXTRA_VARABLE_MACRO() \
   FUN_SIMPLE(std::string, QString, ProblemType, problemtype, "", "") \
   FUN_SIMPLE(std::string, QString, Geometry, geometry, "", "") \
   FUN_SIMPLE(double, QString, MergeTolerance, mergetolerance, -1e23, "") \
   FUN_SIMPLE(std::string, QString, SaveParallel, saveparallel, "", "") \
   FUN_SIMPLE(bool, bool, Info, info, false, "on") \
   FUN_SIMPLE(bool, bool, MeshInfo, meshinfo, false, "on") \
   FUN_STRUCT(std::vector<cmbNucCoreParams::NeumannSetStruct>, cmbNucCoreParams::NeumannSetStruct, NeumannSet, neumannset, false, "") \
   FUN_STRUCT(cmbNucCoreParams::ExtrudeStruct,                 cmbNucCoreParams::ExtrudeStruct,    Extrude,    extrude,    false, "") \
   FUN_SIMPLE(std::string, QString, OutputFileName, outputfilename, "", "")

template<class TYPE>
bool operator!=(std::vector<TYPE> const& vec, bool v)
{
  return vec.empty() == v;
}

class cmbNucCoreParams : public QObject
{
  Q_OBJECT
public:
  struct ExtrudeStruct
  {
    ExtrudeStruct()
    :Size(-1e23), Divisions(-100)
    {}

    double Size;
    int Divisions;
    bool operator!=(bool b)
    { return (Size == -1e23 && Divisions == -100) == b; }
  };

  struct NeumannSetStruct
  {
    NeumannSetStruct()
    :Side(""), Id(-100), Equation("")
    {}

    std::string Side;
    int Id;
    std::string Equation;
    bool operator!=(bool b)
    { return (Side == "" && Id == -100) == b; }
  };

#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, DK) \
  TYPE Var; \
  bool Var##IsSet() \
  { return Var != DEFAULT; }

#define FUN_STRUCT(TYPE,X,Var,Key,DEFAULT, DK) FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, DK)

  EXTRA_VARABLE_MACRO()

#undef FUN_SIMPLE
#undef FUN_STRUCT

  cmbNucCoreParams()
  {
#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, DK) Var = DEFAULT;
#define FUN_STRUCT(TYPE,X,Var,Key,DEFAULT, DK) Var = TYPE();
    EXTRA_VARABLE_MACRO()
#undef FUN_SIMPLE
#undef FUN_STRUCT
  }

  std::vector<std::string> UnknownKeyWords;

private:

  void convert(QString qw, std::string & result) const
  {
    result = qw.toStdString();
  }

  void convert(QString qw, double & result) const
  {
    bool ok;
    double previous = result;
    result = qw.toDouble(&ok);
    if(!ok)
    {
      result = previous;
    }
  }

  void convert(bool qw, bool & result) const
  {
    result = qw;
  }

public slots:

  void setProblemType(QString v)
  {  convert(v, ProblemType); }

  void setGeometry(QString v)
  {  convert(v, Geometry); }

  void setMergeTolerance(QString v)
  {  convert(v, MergeTolerance); }

  void setSaveParallel(QString v)
  {  convert(v, SaveParallel); }

  void setInfo(bool v)
  {  convert(v, Info); }

  void setMeshInfo(bool v)
  {  convert(v, MeshInfo); }

  void setOutputFileName(QString v)
  {  convert(v, OutputFileName); }
};

// Represents the core which is composed of multiple assemblies
// (cmbNucAssembly). Assemblies are layed out on a lattice.
class cmbNucCore : public AssyPartObj
{
public:

  friend class inpFileReader;
  friend class inpFileHelper;
  friend class inpFileWriter;

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
  int GetNumberOfAssemblies();

  // Sets the dimensions of the Assembly Core.
  void SetDimensions(int i, int j);
  // Returns the dimensions of the Assembly Core.

  std::pair<int, int> GetDimensions() const
    {
    return this->CoreLattice.GetDimensions();
    }
  // Sets the contents of the Assembly (i, j) to name.
  void SetAssemblyLabel(int i, int j, const std::string &name, const QColor& color)
    {
    this->CoreLattice.SetCell(i, j, name, color);
    }
  // Returns the contents of the Assembly (i, j).
  LatticeCell GetAssemblyLabel(int i, int j) const
    {
    return this->CoreLattice.GetCell(i, j);
    }
  // Clears the contents of the Assembly (i, j). This is equivalent
  // to calling SetAssembly(i, j, "xx", Qt::white).
  void ClearAssemblyLabel(int i, int j)
    {
    this->CoreLattice.ClearCell(i, j);
    }

  // Rebuild the grid (which for now just updates the colors at each cell)
  void RebuildGrid();

  // Returns a multi-block data set containing the geometry for
  // the core with assemblies. This is used to render the core in 3D.
  vtkSmartPointer<vtkMultiBlockDataSet> GetData();

  void SetLegendColorToAssemblies(int numDefaultColors, int defaultColors[][3]);

  // Check if GeometryType is Hexagonal
  bool IsHexType();

  // read file and return a new Assembly
  cmbNucAssembly* loadAssemblyFromFile(
     const std::string &fileName, const std::string &assyLabel);

  // Get/Set Assembly pitch
  double AssyemblyPitchX;
  double AssyemblyPitchY;

  Lattice CoreLattice;
  std::string BackgroudMeshFile;
  std::string GeometryType;
  std::string FileName;
  std::string h5mFile;
  int HexSymmetry;

  cmbNucCoreParams Params;

private:
  vtkSmartPointer<vtkMultiBlockDataSet> Data;
  std::vector<cmbNucAssembly*> Assemblies;

};

#endif // cmbNucCore_H
