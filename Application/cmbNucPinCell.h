#ifndef _cmbNucPinCell_h_
#define _cmbNucPinCell_h_

#include <QObject>
#include <QSet>

#include "cmbNucPartDefinition.h"
#include "cmbNucMaterial.h"

class PinSubPart;

class PinConnection: public QObject
{
  Q_OBJECT
public:
  void EmitChangeSignal()
  {
    emit Changed();
  }
signals:
  void Changed();
};

class PinSubPart: public AssyPartObj
{
public:
  PinSubPart();
  virtual ~PinSubPart();

  PinConnection* GetConnection() const;

  QPointer<cmbNucMaterial> GetMaterial(int i);
  void SetMaterial(int i, QPointer<cmbNucMaterial> material);
  void SetNumberOfLayers(int numLayers);
  std::size_t GetNumberOfLayers() const;
  void setConnection(cmbNucMaterialLayer & layer);
  QSet< cmbNucMaterial* > getMaterials();
  double x;
  double y;
  double z1;
  double z2;
protected:
  std::vector< cmbNucMaterialLayer > Materials;
  PinConnection * Connection;
};

class Cylinder : public PinSubPart
{
public:
  Cylinder();
  enumNucPartsType GetType();
  bool operator==(const Cylinder& obj);
  double r;
};

class Frustum : public PinSubPart
{
public:
  Frustum();
  enumNucPartsType GetType();
  bool operator==(const Frustum& obj);
  double r1;
  double r2;
};

// Represents a single pin cell. Pin cells can have multiple
// sections which are either cylinders (constant radius) or
// frustums (with start and end radii) aka truncated cones.
// Pin cells also have multiple layers specified by their thickness
// which can be assigned different material properties.
//
// Pin cells also have names (strings) and labels (usually two character
// strings). In other parts of the code (e.g. cmbNucAssembly) pin cells
// are refered to by their label.

class PinCell : public AssyPartObj
{
public:
  PinCell();

  ~PinCell();

  enumNucPartsType GetType();

  int NumberOfSections() const;

  void RemoveSection(AssyPartObj* obj);

  void RemoveCylinder(Cylinder* cylinder);

  void RemoveFrustum(Frustum* frustum);

  double Radius(int idx) const;

  void SetRadius(int idx, double radius);

  void SetMaterial(int idx, QPointer<cmbNucMaterial> material);

  QColor GetLegendColor() const;

  void SetLegendColor(const QColor& color);

  int GetNumberOfLayers();

  void SetNumberOfLayers(int numLayers);

  //These take ownership
  void AddCylinder(Cylinder* cylinder);
  void AddFrustum(Frustum* frustum);

  size_t NumberOfCylinders() const;
  size_t NumberOfFrustums() const;

  Cylinder* GetCylinder(int i) const;
  Frustum * GetFrustum(int i) const;

  PinSubPart* GetPart(int i) const;
  size_t GetNumberOfParts() const;

  PinConnection* GetConnection() const;
  QSet< cmbNucMaterial* > getMaterials();

  std::string name;
  std::string label;
  double pitchX;
  double pitchY;
  double pitchZ;
  std::vector<double> radii;
  QColor legendColor;
  vtkSmartPointer<vtkMultiBlockDataSet> CachedData;
  bool cutaway;

protected:
  std::vector<Cylinder*> Cylinders;
  std::vector<Frustum*> Frustums;
  PinConnection * Connection;
};

#endif
