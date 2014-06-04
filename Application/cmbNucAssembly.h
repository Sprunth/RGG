#ifndef cmbNucAssembly_H
#define cmbNucAssembly_H

#include <string>
#include <vector>
#include <QColor>
#include <QObject>
#include <QDebug>
#include <QPointer>

#include "vtkMultiBlockDataSet.h"
#include "cmbNucPartDefinition.h"
#include "cmbNucPinCell.h"
#include "cmbNucDuctCell.h"
#include "vtkSmartPointer.h"
#include "vtkPolyData.h"

class vtkCompositeDataDisplayAttributes;
class inpFileReader;
class inpFileHelper;
class inpFileWriter;
class cmbNucDefaults;

#define ASSY_NOT_SET_VALUE -100001
#define ASSY_NOT_SET_KEY "NotSet"

#define ASSYGEN_EXTRA_VARABLE_MACRO() \
FUN_SIMPLE(std::string, QString, StartPinId,               startpinid,               ASSY_NOT_SET_KEY, "") \
FUN_SIMPLE(std::string, QString, MeshType,                 meshtype,                 ASSY_NOT_SET_KEY, "")\
FUN_SIMPLE(std::string, QString, CellMaterial,             cellmaterial,             ASSY_NOT_SET_KEY, "") \
FUN_SIMPLE(std::string, QString, CreateMatFiles,           creatematfiles,           ASSY_NOT_SET_KEY, "") \
FUN_SIMPLE(std::string, QString, Save_Exodus,              save_exodus,              ASSY_NOT_SET_KEY, "") \
FUN_SIMPLE(std::string, QString, NeumannSet_StartId,       neumannset_startid,       ASSY_NOT_SET_KEY, "") \
FUN_SIMPLE(std::string, QString, List_NeumannSet_StartId,  list_neumannset_startid,  ASSY_NOT_SET_KEY, "") \
FUN_SIMPLE(std::string, QString, MaterialSet_StartId,      materialset_startid,      ASSY_NOT_SET_KEY, "") \
FUN_SIMPLE(std::string, QString, List_MaterialSet_StartId, list_materialset_startid, ASSY_NOT_SET_KEY, "") \
FUN_SIMPLE(std::string, QString, NumSuperBlocks,           numsuperblocks,           ASSY_NOT_SET_KEY, "") \
FUN_SIMPLE(std::string, QString, SuperBlocks,              superblocks,              ASSY_NOT_SET_KEY, "") \
FUN_SIMPLE(std::string, QString, CreateSideset,            createsideset,            ASSY_NOT_SET_KEY, "") \
FUN_SIMPLE(int,         QString, EdgeInterval,             edgeinterval,             ASSY_NOT_SET_VALUE, "") \
FUN_SIMPLE(double,      QString, MergeTolerance,           mergetolerance,           ASSY_NOT_SET_VALUE, "")

class cmbAssyParameters
{
public:
  cmbAssyParameters()
  {
  this->Geometry = this->GeometryType = this->MeshType
    = this->CenterXYZ = this->HBlock
    = this->Info = this->SectionXYZ = ASSY_NOT_SET_KEY;
  this->RadialMeshSize = this->AxialMeshSize
    = this->TetMeshSize = this->CreateFiles
    = this->SectionOffset = ASSY_NOT_SET_VALUE;
  MoveXYZ[0] = MoveXYZ[1] = MoveXYZ[2] = 0.0;
  this->SectionReverse = false;
#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, DK) Var = DEFAULT;
    ASSYGEN_EXTRA_VARABLE_MACRO()
#undef FUN_SIMPLE
  }

  static bool isKeySet(const std::string& key)
  { return key != ASSY_NOT_SET_KEY; }
  static bool isValueSet(const std::string& val)
  { return val != ASSY_NOT_SET_KEY; }
  template<typename T> static bool isValueSet(const T& val)
  { return val != ASSY_NOT_SET_VALUE; }

  // Geometry     {Volume | Surface}
  std::string Geometry;
  // GeometryType {Hexagonal | Rectangular}
  std::string GeometryType;

  // [TetMeshSize <size>]
  double TetMeshSize;
  // [RadialMeshSize <size>]
  double RadialMeshSize;
  // [AxialMeshSize <size>]
  double AxialMeshSize;
  // [Center] {x | y | z}
  std::string CenterXYZ;
  // [Info {On | off}]
  std::string Info;
  // [CreateFiles <block-number-shifted>]
  int CreateFiles;
  // [Section {x | y | z} <offset> [reverse] ]
  std::string SectionXYZ;
  int SectionOffset;
  bool SectionReverse;
  // [Move <x> <y> <z> ]
  double MoveXYZ[3];
  // HBlock
  std::string HBlock;

#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, DK) TYPE Var;
  ASSYGEN_EXTRA_VARABLE_MACRO()
#undef FUN_SIMPLE
  std::vector<std::string> UnknownParams;
};

class cmbNucAssembly;

class cmbNucAssemblyConnection: public QObject
{
  Q_OBJECT
  friend class cmbNucAssembly;
private:
  cmbNucAssembly * v;
  void sendPitch(double x, double y);
public slots:
  void dataChanged();
  void calculatePitch();
signals:
  void dataChangedSig();
  void colorChanged();
  void pitchResult(double x, double y);
};

// Represents an assembly. Assemblies are composed of pin cells (cmbNucPinCell)
// and the surrounding ducting. Assemblies can be loaded and stored to files
// with the ReadFile() and Write() file methods. Assemblies are grouped together
// into cores (cmbNucCore).
class cmbNucAssembly : public AssyPartObj
{
public:
  class Transform
  {
  public:
    Transform():Valid(false){}
    virtual ~Transform(){}
    bool isValid() const { return Valid; }
    //returns true if changed
    bool setValid( bool valid)
    {
      bool r = Valid != valid;
      Valid = valid;
      return r;
    }
    virtual void apply(vtkMultiBlockDataSet * input, vtkMultiBlockDataSet * output) const = 0;
    virtual std::ostream& write(std::ostream& os) const = 0;
  protected:
    bool Valid;
  };
  class Rotate: public Transform
  {
  public:
    enum AXIS{ X = 0, Y = 1, Z = 2 };
    Rotate(): axis(Z), angle(0) {}
    Rotate(std::string a, double delta);
    virtual void apply(vtkMultiBlockDataSet * input, vtkMultiBlockDataSet * output) const;
    std::ostream& write(std::ostream& os) const;
  private:
    void setAxis(std::string a);
    AXIS axis;
    double angle;
  };
  class Section: public Transform
  {
  public:
    enum AXIS{ X = 0, Y = 1, Z = 2 };
    Section(): axis(X),value(0){}
    Section(std::string a, double v, std::string dir);
    virtual void apply(vtkMultiBlockDataSet * input, vtkMultiBlockDataSet * output) const;
    std::ostream& write(std::ostream& os) const;
  private:
    void setAxis(std::string a);
    AXIS axis;
    int dir;
    double value;
  };
public:

  friend class inpFileReader;
  friend class inpFileHelper;
  friend class inpFileWriter;
  friend class cmbNucAssemblyConnection;

  // Creates an empty assembly.
  cmbNucAssembly();

  // Destroys the assembly.
  ~cmbNucAssembly();

  cmbNucAssemblyConnection * GetConnection() {return this->Connection; }

  virtual enumNucPartsType GetType() const {return CMBNUC_ASSEMBLY;}

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
  PinCell* GetPinCell(int pc) const;
  std::size_t GetNumberOfPinCells() const;

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

  static void clip(vtkMultiBlockDataSet * input, vtkMultiBlockDataSet * output,
                   double * normal, int offset = 0);

  // creates the multiblock used to render the pincell. if cutaway is true the
  // pincell will be cut in half length-wise to show the interior layers.
  static vtkMultiBlockDataSet* CreatePinCellMultiBlock(PinCell *pincell,
                                                       bool isHex,
                                                       bool cutaway = false);

  cmbAssyParameters* GetParameters() {return this->Parameters;}

  //Set the different from file and tests the cub file;
  void setAndTestDiffFromFiles(bool diffFromFile);
  bool changeSinceLastSave() const;
  bool changeSinceLastGenerate() const;

  void GetDuctWidthHeight(double r[2]);

  void clear();

  QSet< cmbNucMaterial* > getMaterials();

  void setFromDefaults(QPointer<cmbNucDefaults> d);
  void computeDefaults();

  void calculatePitch(double & x, double & y);
  void calculateRadius(double & r);

  void centerPins();

  void setCenterPins(bool t);

  bool isPinsAutoCentered()
  {
    return KeepPinsCentered;
  }

  std::string getLabel(){return label;}
  std::string getFileName(){return FileName;}
  virtual std::string getTitle(){ return "Assembly: " + label; }

  // Expose assembly parts for UI access
  DuctCell AssyDuct;
  Lattice AssyLattice;
  std::string label;

  std::string FileName;

  std::string getGeometryLabel() const;
  void setGeometryLabel(std::string geomType);

  QPointer<cmbNucDefaults> getDefaults()
  { return this->Defaults; }

  QPointer<cmbNucDefaults> Defaults;

  // Check if GeometryType is Hexagonal
  bool IsHexType();

  bool addTransform(Transform * in); //Will not add invalid xfroms, takes ownership
  Transform* getTransform(int i) const; //NULL if not found
  size_t getNumberOfTransforms() const;

protected:
  std::vector<PinCell*> PinCells;

  std::vector<Transform*> Transforms;

  bool KeepPinsCentered;

  cmbAssyParameters *Parameters;

  friend class cmbNucMainWindow;

  vtkSmartPointer<vtkMultiBlockDataSet> CreateData();

  void computeRecOffset(unsigned int i, unsigned int j, double &tx, double &ty);

  std::string GeometryType;

private:
  vtkSmartPointer<vtkMultiBlockDataSet> Data;
  QColor LegendColor;


  bool DifferentFromFile;
  bool DifferentFromCub;

  cmbNucAssemblyConnection * Connection;

};

#endif // cmbNucAssembly_H
