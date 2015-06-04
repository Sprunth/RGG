#ifndef cmbNucCore_H
#define cmbNucCore_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <QColor>
#include <QString>
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

#include "cmbNucMaterialColors.h"

class cmbNucAssembly;
class cmbNucAssemblyLink;
class cmbNucPinLibrary;
class cmbNucDuctLibrary;
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
   FUN_STRUCT(cmbNucCoreParams::ExtrudeStruct,                 cmbNucCoreParams::ExtrudeStruct,    Extrude,    extrude,    false, "")

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
public:
  virtual ~cmbNucCoreConnection() {}
private:
  cmbNucCore * v;
public slots:
  void dataChanged();
  void justFileChanged();
  void assemblyChanged();
signals:
  void dataChangedSig();
  void colorChanged();
};

// Represents the core which is composed of multiple assemblies
// (cmbNucAssembly). Assemblies are layed out on a lattice.
class cmbNucCore : public LatticeContainer
{
public:

  friend class cmbNucCoreConnection;

  class  boundaryLayer
  {
  public:
     boundaryLayer()//TODO update this when there is better understanding
    {
      interface_material = cmbNucMaterialColors::instance()->getMaterialByName("water");
      NeumannSet = 14;
      Fixmat = 4;
      Thickness = 0.05;
      Intervals = 6;
      Bias = 0.7;
    }
    QPointer<cmbNucMaterial> interface_material;
    QPointer<cmbNucMaterial> fixed_material;
    int NeumannSet;
    int Fixmat;
    double Thickness;
    int Intervals;
    double Bias;
    //used for export purposes
    std::string jou_file_name;
  };

  // Creates an empty Core.
  cmbNucCore(bool needSaved = true);

  const static double CosSinAngles[6][2];

  // Destroys the Core.
  virtual ~cmbNucCore();

  vtkBoundingBox computeBounds();

  cmbNucCoreConnection* GetConnection(){return this->Connection;}

  virtual enumNucPartsType GetType() const {return CMBNUC_CORE;}

  void clearExceptAssembliesAndGeom();

  // Adds a new Assembly to the core. After adding the Assembly it
  // can be placed in the Core with the SetAssembly() method.
  void AddAssembly(cmbNucAssembly *assembly);
  bool AddAssemblyLink(cmbNucAssemblyLink * assemblyLink);

  // Remove the Assembly with label from the Core.
  void RemoveAssembly(const std::string &label);
  void RemoveAssemblyLink(const std::string &label);

  bool okToDelete(std::string const& label);

  // Returns the Assembly with label or index.
  // Returns 0 if no Assembly with label or index exists.
  cmbNucAssembly* GetAssembly(const std::string &label);
  cmbNucAssembly* GetAssembly(int idx);
  cmbNucAssemblyLink* GetAssemblyLink(const std::string &label);
  cmbNucAssemblyLink* GetAssemblyLink(int idx);

  bool label_unique(std::string n);
  bool label_unique(QString const& n)
  {
    return label_unique( n.toStdString() );
  }

  std::vector< cmbNucAssembly* > GetUsedAssemblies();
  std::vector< cmbNucAssemblyLink* > GetUsedLinks();

  // Return the number of assemblies in the core
  int GetNumberOfAssemblies() const;
  int GetNumberOfAssemblyLinks() const;

  // Sets the dimensions of the Assembly Core.
  void SetDimensions(int i, int j);
  // Returns the dimensions of the Assembly Core.

  std::pair<int, int> GetDimensions() const
  {
    return this->lattice.GetDimensions();
  }
  // Sets the contents of the Assembly (i, j) to name.
  void SetAssemblyLabel(int i, int j, const std::string &name,
                        const QColor& color)
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

  virtual void calculateRectTranslation(double /*lastPt*/[2],
                                        double & transX, double & transY);

  //Set the different from file and tests the h5m file;
  void setAndTestDiffFromFiles(bool diffFromFile);
  void fileChanged();
  void boundaryLayerChanged();
  bool changeSinceLastSave() const;
  bool changeSinceLastGenerate() const;
  bool boundaryLayerChangedSinceLastGenerate() const;

  std::string getFileName(){return CurrentFileName;}
  virtual std::string getTitle(){ return "Core"; }

  // Get/Set Assembly pitch
  double getAssemblyPitchX() const
  {
    return this->AssemblyPitchX;
  }
  double getAssemblyPitchY() const
  {
    return this->AssemblyPitchY;
  }
  void setAssemblyPitch(double x, double y)
  {
    this->AssemblyPitchX = x;
    this->AssemblyPitchY = y;
  }

  void setGeometryLabel(std::string geomType);

  void setFileName( std::string const& fname );

  void setGenerateDirectory(std::string const& dir);
  std::string const& getGenerateDirectory() const;

  std::string getMeshOutputFilename() const;
  void setMeshOutputFilename(std::string const& fname);

  std::string const& getExportFileName() const;
  void setExportFileName(std::string const& fname);

  void setHexSymmetry(int sym);

  cmbNucCoreParams & getParams()
  {
    return Params;
  }

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

  cmbNucDuctLibrary * getDuctLibrary()
  {
    return this->DuctLibrary;
  }

  virtual QString extractLabel(QString const& s)
  {
    return s;
  }

  virtual void setUsedLabels(std::map<QString, int> const& /*labels*/)
  {}

  std::map< std::string, std::set< Lattice::CellDrawMode > >
    getDrawModesForAssemblies();

  void addBoundaryLayer( boundaryLayer * bl); //Takes ownership
  std::vector< cmbNucCore::boundaryLayer*> const& getBoundaryLayers() const
  {
    return this->BoundaryLayers;
  }
  int getNumberOfBoundaryLayers() const;
  void removeBoundaryLayer(size_t bl);
  void clearBoundaryLayer();
  boundaryLayer * getBoundaryLayer(int bl) const;

private:

  double AssemblyPitchX;
  double AssemblyPitchY;

  cmbNucCoreParams Params;

  bool hasCylinder;
  double cylinderRadius;
  int cylinderOuterSpacing;

  std::vector<cmbNucAssembly*> Assemblies;
  std::vector<cmbNucAssemblyLink*> AssemblyLinks;
  std::vector< boundaryLayer *> BoundaryLayers;
  std::vector< boundaryLayer> ExportBoundaryLayers;

  cmbNucPinLibrary * PinLibrary;
  cmbNucDuctLibrary * DuctLibrary;
  cmbNucCoreConnection * Connection;

  QPointer<cmbNucDefaults> Defaults;

  std::string meshFilePrefix;
  std::string meshFileExtention;

  //TODO: we need to redo this.  We should move to a mode form, to keep state
  //of saved file, output mesh, and  boundary layers.
  //Maybe move the diff from file outside of this.
  bool DifferentFromFile;
  bool DifferentFromH5M;
  bool DifferentFromGenBoundaryLayer;

  int HexSymmetry;

  std::string CurrentFileName;
  std::string GenerateDirectory;
  std::string ExportFileName;

};

#endif // cmbNucCore_H
