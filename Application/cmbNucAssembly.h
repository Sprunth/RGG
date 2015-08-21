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
#include "cmbNucLattice.h"
#include "cmbNucPinCell.h"
#include "cmbNucDuctCell.h"

#include "vtkSmartPointer.h"
#include "vtkPolyData.h"
#include "vtkBoundingBox.h"

class vtkCompositeDataDisplayAttributes;
class inpFileReader;
class inpFileHelper;
class inpFileWriter;
class cmbNucDefaults;
class cmbNucPinLibrary;
class cmbNucDuctLibrary;

#define ASSY_NOT_SET_VALUE -100001
#define ASSY_NOT_SET_KEY "NotSet"

#define ASSYGEN_EXTRA_VARABLE_MACRO() \
FUN_SIMPLE(std::string, QString, StartPinId,               startpinid,               ASSY_NOT_SET_KEY, "") \
FUN_SIMPLE(std::string, QString, MeshType,                 meshtype,                 ASSY_NOT_SET_KEY, "")\
FUN_SIMPLE(std::string, QString, CellMaterial,             cellmaterial,             ASSY_NOT_SET_KEY, "") \
FUN_SIMPLE(std::string, QString, CreateMatFiles,           creatematfiles,           ASSY_NOT_SET_KEY, "") \
FUN_SIMPLE(bool,        bool,    Save_Exodus,              save_exodus,              false,            "on") \
FUN_SIMPLE(std::string, QString, NeumannSet_StartId,       neumannset_startid,       ASSY_NOT_SET_KEY, "") \
FUN_SIMPLE(std::string, QString, List_NeumannSet_StartId,  list_neumannset_startid,  ASSY_NOT_SET_KEY, "") \
FUN_SIMPLE(std::string, QString, MaterialSet_StartId,      materialset_startid,      ASSY_NOT_SET_KEY, "") \
FUN_SIMPLE(std::string, QString, List_MaterialSet_StartId, list_materialset_startid, ASSY_NOT_SET_KEY, "") \
FUN_SIMPLE(std::string, QString, NumSuperBlocks,           numsuperblocks,           ASSY_NOT_SET_KEY, "") \
FUN_SIMPLE(std::string, QString, SuperBlocks,              superblocks,              ASSY_NOT_SET_KEY, "") \
FUN_SIMPLE(std::string, QString, CreateSideset,            createsideset,            ASSY_NOT_SET_KEY, "") \
FUN_SIMPLE(int,         QString, EdgeInterval,             edgeinterval,             ASSY_NOT_SET_VALUE, "") \
FUN_SIMPLE(double,      QString, MergeTolerance,           mergetolerance,           ASSY_NOT_SET_VALUE, "") \
FUN_SIMPLE(std::string, QString, MeshScheme,               meshscheme,               ASSY_NOT_SET_KEY, "")

class cmbAssyParameters
{
public:
  cmbAssyParameters()
  {
  this->Geometry = this->MeshType
    = this->CenterXYZ = this->HBlock
    = this->Info = ASSY_NOT_SET_KEY;
  this->RadialMeshSize = this->AxialMeshSize
    = this->TetMeshSize = this->CreateFiles
    = ASSY_NOT_SET_VALUE;
  MoveXYZ[0] = MoveXYZ[1] = MoveXYZ[2] = 0.0;
#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, DK) Var = DEFAULT;
    ASSYGEN_EXTRA_VARABLE_MACRO()
#undef FUN_SIMPLE
  }

  static bool isKeySet(const std::string& key)
  { return key != ASSY_NOT_SET_KEY; }
  static bool isValueSet(const std::string& val)
  { return val != ASSY_NOT_SET_KEY; }
  static bool isValueSet(const bool& val)
  { return val; }
  template<typename T> static bool isValueSet(const T& val)
  { return val != ASSY_NOT_SET_VALUE; }

  void fill(cmbAssyParameters * other);

  // Geometry     {Volume | Surface}
  std::string Geometry;

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
  // [Move <x> <y> <z> ]
  double MoveXYZ[3];
  // HBlock
  std::string HBlock;

#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, DK) TYPE Var;
  ASSYGEN_EXTRA_VARABLE_MACRO()
#undef FUN_SIMPLE
  std::vector<std::string> UnknownParams;

  //int getNeumannSetStartId(){return this->neumannSetStartId;}
  //int getMaterialSetStartId(){return this->materialSetStartId;}
  int neumannSetStartId, materialSetStartId;
};

class cmbNucAssembly;
class PinCell;

class cmbNucAssemblyConnection: public QObject
{
  Q_OBJECT
  friend class cmbNucAssembly;
public:
  virtual ~cmbNucAssemblyConnection() {}
private:
  cmbNucAssembly * v;
  void sendPitch(double x, double y);
public slots:
  void dataChanged();
  void geometryChanged();
  void pinDeleted(PinCell*);
signals:
  void dataChangedSig();
  void colorChanged();
  void pitchResult(double x, double y);
};

// Represents an assembly. Assemblies are composed of pin cells (cmbNucPinCell)
// and the surrounding ducting. Assemblies can be loaded and stored to files
// with the ReadFile() and Write() file methods. Assemblies are grouped together
// into cores (cmbNucCore).
class cmbNucAssembly : public LatticeContainer
{
public:
  class Transform
  {
  public:
    enum CONTROLS{HAS_REVERSE = 0x00001, HAS_VALUE = 0x00002, HAS_AXIS = 0x00004};
    enum AXIS{ X = 0, Y = 1, Z = 2 };
    Transform():Valid(false), axis(Z){}
    virtual ~Transform(){}
    bool isValid() const { return Valid; }
    //returns true if changed
    bool setValid( bool valid)
    {
      bool r = Valid != valid;
      Valid = valid;
      return r;
    }
    virtual double getValue() const = 0;
    virtual AXIS getAxis() const {return axis;}
    virtual std::string getLabel() const = 0;
    virtual bool reverse() const = 0;
    virtual std::ostream& write(std::ostream& os) const = 0;
    virtual int getControls() const = 0;
    void setAxis(std::string a);
  protected:
    bool Valid;
    AXIS axis;
  };
  class Rotate: public Transform
  {
  public:
    Rotate(): angle(0) {}
    Rotate(std::string a, double delta);
    Rotate(AXIS a, double delta);
    std::ostream& write(std::ostream& os) const;
    virtual double getValue() const {return angle;}
    virtual bool reverse() const {return false;}
    virtual std::string getLabel() const {return "Rotate";}
    virtual int getControls() const {return 6;}
  private:
    double angle;
  };
  class Section: public Transform
  {
  public:
    Section(): dir(1), value(0){}
    Section(std::string a, double v, std::string dir);
    Section(AXIS a, double v, int dir);
    std::ostream& write(std::ostream& os) const;
    virtual double getValue() const { return value; }
    virtual bool reverse() const {return dir == -1;}
    virtual std::string getLabel() const {return "Section";}
    virtual int getControls() const {return 7;}
  private:
    int dir;
    double value;
  };

  class PinCellUsed
  {
  public:
    PinCellUsed() : count(0){}
    int count;
    PinCell* pincell;
  };
public:

  //friend class inpFileReader;
  //friend class inpFileHelper;
  //friend class inpFileWriter;
  friend class cmbNucAssemblyConnection;

  // Creates an empty assembly.
  cmbNucAssembly();

  std::string getOutputExtension();

  // Destroys the assembly.
  virtual ~cmbNucAssembly();

  void setPinLibrary(cmbNucPinLibrary * p)
  {
    this->Pins = p;
  }

  void setDuctLibrary(cmbNucDuctLibrary * d);

  virtual QString extractLabel(QString const& s);
  virtual void setUsedLabels(std::map<QString, int> const& labels);

  const static double CosSinAngles[6][2];

  cmbNucAssemblyConnection * GetConnection() {return this->Connection; }

  virtual enumNucPartsType GetType() const {return CMBNUC_ASSEMBLY;}

  vtkBoundingBox computeBounds();

  // Adds a new pincell to the assebly. After adding the pincell it
  // can be placed in the assembly with the SetCell() method.
  void AddPinCell(PinCell *pincell);
  void SetPinCell(int i, PinCell *pc);

  // Remove the pincell with label from the assembly.
  void RemovePinCell(const std::string label);

  void geometryChanged();

  // Returns the pincell with label. Returns 0 if no pincell with
  // label exists.
  PinCell* GetPinCell(const std::string &label);
  PinCell* GetPinCell(int pc) const;
  std::size_t GetNumberOfPinCells() const;

  // The color to use to represent this assembly type in the lattice editor
  QColor GetLegendColor() const;
  void SetLegendColor(const QColor& color);

  cmbAssyParameters* GetParameters() {return this->Parameters;}

  //Set the different from file and tests the cub file;
  void setAndTestDiffFromFiles(bool diffFromFile);
  bool changeSinceLastGenerate() const;

  void GetDuctWidthHeight(double r[2]);

  void getZRange(double & z1, double & z2);

  void clear();

  QSet< cmbNucMaterial* > getMaterials();
  QSet< cmbNucMaterial* > getInterfaceMaterials(QPointer<cmbNucMaterial> in_material);
  QSet< cmbNucMaterial* > getOtherFixed(QPointer<cmbNucMaterial> boundaryLayerMaterial,
                                        QPointer<cmbNucMaterial> fixedMaterial);
  bool has_boundary_layer_interface(QPointer<cmbNucMaterial> in_material) const;

  void setFromDefaults(QPointer<cmbNucDefaults> d);
  void computeDefaults();

  void calculatePitch(double & x, double & y);
  void calculatePitch(int width, int height, double & x, double & y);
  void calculateRadius(double & r);
  void setPitch(double x, double y, bool testDiff = true);

  void centerPins();

  void setCenterPins(bool t);

  bool isPinsAutoCentered()
  {
    return KeepPinsCentered;
  }

  void setPath(std::string const& path);
  void setFileName(std::string const& fname);
  std::string getFileName();
  std::string getFileName(Lattice::CellDrawMode mode, size_t nom = 1);
  virtual std::string getTitle(){ return "Assembly: " + Label; }
  bool needToRunMode(Lattice::CellDrawMode mode, std::string & fname, size_t nom = 1);

  std::string getGeometryLabel() const;
  void setGeometryLabel(std::string geomType);

  QPointer<cmbNucDefaults> getDefaults()
  { return this->Defaults; }

  QPointer<cmbNucDefaults> Defaults;

  // Check if GeometryType is Hexagonal
  bool IsHexType();

  //Transforms exist for inp file import and export
  bool addTransform(Transform * in); //Will not add invalid xfroms, takes ownership

  bool removeOldTransforms(int newSize);
  Transform* getTransform(int i) const; //NULL if not found
  size_t getNumberOfTransforms() const;

  void fillList(QStringList & l);

  virtual AssyPartObj * getFromLabel(const std::string & s)
  {
    return this->GetPinCell(s);
  }

  virtual void calculateRectPt(unsigned int i, unsigned int j, double pt[2]);

  virtual void calculateRectTranslation(double lastPt[2], double & transX, double & transY)
  {
    transX = -lastPt[0]*0.5;
    transY = -lastPt[1]*0.5;
  }

  double getPinPitchX(){return this->pinPitchX;}
  double getPinPitchY(){return this->pinPitchY;}

  cmbNucPinLibrary * getPinLibrary()
  { return Pins; }

  cmbNucDuctLibrary * getDuctLibrary()
  { return Ducts; }

  DuctCell & getAssyDuct()
  {
    return *AssyDuct;
  }

  //return true when changed
  bool setDuctCell(DuctCell * AssyDuct, bool resetPitch = false);

  void adjustRotation();

  double getZAxisRotation() const;
  void setZAxisRotation(double d);

  cmbNucAssembly * clone(cmbNucPinLibrary * pl, cmbNucDuctLibrary * dl);

protected:
  std::vector<PinCell*> PinCells;
  cmbNucPinLibrary * Pins;
  cmbNucDuctLibrary * Ducts;
  DuctCell * AssyDuct;

  //Transforms exist for inp file import and export
  std::vector<Transform*> Transforms;

  Rotate zAxisRotation;

  bool KeepPinsCentered;

  cmbAssyParameters *Parameters;

  std::string GeometryType;

private:
  QColor LegendColor;

  bool DifferentFromCub;
  bool DifferentFromJournel;

  std::string ExportFilename;
  std::string Path;

  cmbNucAssemblyConnection * Connection;

  double pinPitchX, pinPitchY;

  bool checkJournel(std::string const& fname );
  bool checkCubitOutputFile(std::string const& fname );

};

#endif // cmbNucAssembly_H
