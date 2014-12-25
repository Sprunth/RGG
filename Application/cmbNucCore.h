#ifndef cmbNucCore_H
#define cmbNucCore_H

#include <string>
#include <vector>
#include <QColor>
#include <QObject>
#include <QPointer>
#include <QStringList>

#include "vtkMultiBlockDataSet.h"
#include "cmbNucPartDefinition.h"
#include "cmbNucLattice.h"
#include "vtkSmartPointer.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkTransform.h"
#include "vtkBoundingBox.h"

class cmbNucAssembly;
class cmbNucPinLibrary;
class inpFileReader;
class inpFileHelper;
class inpFileWriter;
class cmbNucDefaults;
class vtkPolyData;

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

  enum {None=0,External=1,Generate=2} BackgroundMode;

#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, DK) \
  TYPE Var; \
  bool Var##IsSet() \
  { return Var != DEFAULT; }

#define FUN_STRUCT(TYPE,X,Var,Key,DEFAULT, DK) FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, DK)

  EXTRA_VARABLE_MACRO()

#undef FUN_SIMPLE
#undef FUN_STRUCT

  bool BackgroundIsSet() {return !Background.empty();}

  cmbNucCoreParams()
  {
    Background = "";
    BackgroundFullPath = "";
    BackgroundMode = None;
#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, DK) Var = DEFAULT;
#define FUN_STRUCT(TYPE,X,Var,Key,DEFAULT, DK) Var = TYPE();
    EXTRA_VARABLE_MACRO()
#undef FUN_SIMPLE
#undef FUN_STRUCT
  }

  void clear()
  {
#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, DK) Var = DEFAULT;
#define FUN_STRUCT(TYPE,X,Var,Key,DEFAULT, DK) Var = TYPE();
    EXTRA_VARABLE_MACRO()
#undef FUN_SIMPLE
#undef FUN_STRUCT

  }

  std::vector<std::string> UnknownKeyWords;
  std::string Background;
  std::string BackgroundFullPath;
};

class cmbNucCore;

class cmbNucCoreConnection: public QObject
{
  Q_OBJECT

  friend class cmbNucCore;
private:
  cmbNucCore * v;
public slots:
  void dataChanged();
  void assemblyChanged();
  void clearData();
signals:
  void dataChangedSig();
  void colorChanged();
};

// Represents the core which is composed of multiple assemblies
// (cmbNucAssembly). Assemblies are layed out on a lattice.
class cmbNucCore : public LatticeContainer
{
public:

  friend class inpFileReader;
  friend class inpFileHelper;
  friend class inpFileWriter;
  friend class cmbNucCoreConnection;

  // Creates an empty Core.
  cmbNucCore(bool needSaved = true);

  const static double CosSinAngles[6][2];

  // Destroys the Core.
  ~cmbNucCore();

  vtkBoundingBox computeBounds();

  cmbNucCoreConnection* GetConnection(){return this->Connection;}

  virtual enumNucPartsType GetType() const {return CMBNUC_CORE;}

  void clearExceptAssembliesAndGeom();

  // Adds a new Assembly to the core. After adding the Assembly it
  // can be placed in the Core with the SetAssembly() method.
  void AddAssembly(cmbNucAssembly *assembly);

  // Remove the Assembly with label from the Core.
  void RemoveAssembly(const std::string &label);

  // Returns the Assembly with label or index.
  // Returns 0 if no Assembly with label or index exists.
  cmbNucAssembly* GetAssembly(const std::string &label);
  cmbNucAssembly* GetAssembly(int idx);

  bool label_unique(std::string & n);

  std::vector< cmbNucAssembly* > GetUsedAssemblies();

  // Return the number of assemblies in the core
  int GetNumberOfAssemblies() const;

  // Sets the dimensions of the Assembly Core.
  void SetDimensions(int i, int j);
  // Returns the dimensions of the Assembly Core.

  std::pair<int, int> GetDimensions() const
    {
    return this->lattice.GetDimensions();
    }
  // Sets the contents of the Assembly (i, j) to name.
  void SetAssemblyLabel(int i, int j, const std::string &name, const QColor& color)
    {
    this->lattice.SetCell(i, j, name, color);
    }
  // Returns the contents of the Assembly (i, j).
  Lattice::LatticeCell GetAssemblyLabel(int i, int j) const
    {
    return this->lattice.GetCell(i, j);
    }

  // Rebuild the grid (which for now just updates the colors at each cell)
  void RebuildGrid();

  void SetLegendColorToAssemblies(int numDefaultColors, int defaultColors[][3]);

  // Check if GeometryType is Hexagonal
  bool IsHexType();

  void computePitch();

  virtual void calculateRectPt(unsigned int i, unsigned int j, double pt[2]);

  virtual void calculateRectTranslation(double /*lastPt*/[2], double & transX, double & transY);

  //Set the different from file and tests the h5m file;
  void setAndTestDiffFromFiles(bool diffFromFile);
  bool changeSinceLastSave() const;
  bool changeSinceLastGenerate() const;

  std::string getLabel(){return "Core";}
  std::string getFileName(){return FileName;}
  virtual std::string getTitle(){ return "Core"; }

  // Get/Set Assembly pitch
  double AssyemblyPitchX;
  double AssyemblyPitchY;

  void setGeometryLabel(std::string geomType);

  std::string FileName;
  std::string h5mFile;
  void setHexSymmetry(int sym);

  bool DifferentFromFile;
  bool DifferentFromH5M;

  cmbNucCoreParams Params;

  QPointer<cmbNucDefaults> GetDefaults();
  bool HasDefaults() const;
  void calculateDefaults();
  void sendDefaults();
  void initDefaults();

  void drawCylinder(double r, int i);
  void clearCylinder();


  void checkUsedAssembliesForGen();

  void fillList(QStringList & l);

  virtual AssyPartObj * getFromLabel(const std::string & s);

  double getCylinderRadius()
  { return cylinderRadius; }

  int getCylinderOuterSpacing()
  { return cylinderOuterSpacing; }

  void setCylinderRadius(double r)
  {
    this->cylinderRadius = r;
  }

  void setCylinderOuterSpacing(int v)
  {
    this->cylinderOuterSpacing = v;
  }

  bool getHasCylinder()
  {
    return hasCylinder;
  }

  cmbNucPinLibrary * getPinLibrary()
  {
    return this->PinLibrary;
  }

private:
  bool hasCylinder;
  double cylinderRadius;
  int cylinderOuterSpacing;

  std::vector<cmbNucAssembly*> Assemblies;
  cmbNucPinLibrary * PinLibrary;
  cmbNucCoreConnection * Connection;

  QPointer<cmbNucDefaults> Defaults;

  int HexSymmetry;

};

#endif // cmbNucCore_H
