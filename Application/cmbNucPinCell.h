#ifndef _cmbNucPinCell_h_
#define _cmbNucPinCell_h_

#include <QObject>
#include <QSet>

#include "cmbNucPartDefinition.h"
#include "cmbNucMaterial.h"
#include "vtkBoundingBox.h"

class PinSubPart;
class PinCell;

class PinConnection: public QObject
{
  Q_OBJECT
  friend class PinCell;
public:
  PinConnection():pc(NULL){}
  void EmitChangeSignal()
  {
    emit Changed();
  }
  void EmitDeleted(PinCell * pc)
  {
    emit Deleted(pc);
  }
public slots:
  void clearOldData();
signals:
  void Changed();
  void CellMaterialChanged();
  void Deleted(PinCell*);
private:
  PinCell * pc;
};

class PinSubPart: public AssyPartObj
{
public:
  enum End{BOTTOM=0, TOP=1};
  PinSubPart(double z1 = 0, double z2= 10);
  virtual ~PinSubPart();
  std::string getLabel(){return "PinPart";}
  virtual std::string getTitle(){ return "PinPart"; }

  PinConnection* GetConnection() const;

  QPointer<cmbNucMaterial> GetMaterial(int i);
  void SetMaterial(int i, QPointer<cmbNucMaterial> material);
  void SetNumberOfLayers(int numLayers);
  std::size_t GetNumberOfLayers() const;
  QSet< cmbNucMaterial* > getMaterials();

  //sets others values here if they are different.
  //If values have changed, we return true.
  virtual bool fill(PinSubPart const* other);
  virtual PinSubPart * clone() const = 0;

  virtual double getNormalizedThickness(int layer, PinSubPart::End end) = 0;
  virtual void setNormalizedThickness(int layer, PinSubPart::End end, double thick) = 0;
  virtual double getRadius(int layer, PinSubPart::End end) = 0;
  virtual double getRadius(PinSubPart::End end) const = 0;
  virtual void setRadius(PinSubPart::End end, double r) = 0;
  virtual double length() const { return z2 - z1; }
  virtual void reverseRadii() {};

  cmbNucMaterialLayer const& getMaterialLayer(int i) const
  { return *Materials[i]; }
  //Takes ownership
  void setMaterialLayer(int i, cmbNucMaterialLayer * m);

  virtual bool equal(PinSubPart const* other) const;

  virtual std::vector<PinSubPart *> split(std::vector<double>::const_iterator b,
                                          std::vector<double>::const_iterator end) = 0;

  double x;
  double y;
  double z1;
  double z2;
protected:
  void setConnection(cmbNucMaterialLayer * layer);
  std::vector< cmbNucMaterialLayer * > Materials;
  PinConnection * Connection;
private:
  PinSubPart( const PinSubPart & )
  {}
};

class Cylinder : public PinSubPart
{
public:
  Cylinder(double rin, double z1, double z2);
  Cylinder(PinSubPart const* other);
  virtual enumNucPartsType GetType() const;
  bool operator==(const Cylinder& obj);
  double getNormalizedThickness(int layer);
  void setNormalizedThickness(int layer, double thick);
  double getRadius(int layer);

  PinSubPart * clone() const;

  virtual double getNormalizedThickness(int layer, PinSubPart::End /*end*/)
  {
    return this->getNormalizedThickness(layer);
  }

  virtual void setNormalizedThickness(int layer, PinSubPart::End /*end*/, double thick)
  {
    this->setNormalizedThickness(layer, thick);
  }

  virtual double getRadius(int layer, PinSubPart::End /*end*/)
  {
    return this->getRadius(layer);
  }

  virtual double getRadius(PinSubPart::End) const
  {
    return r;
  }

  virtual void setRadius(PinSubPart::End, double rin)
  {
    this->r = rin;
  }

  std::vector<PinSubPart *> split(std::vector<double>::const_iterator b,
                                  std::vector<double>::const_iterator end);

  double r;
};

class Frustum : public PinSubPart
{
public:
  Frustum(double const* rin, double z1, double z2);
  Frustum(PinSubPart const* other);
  virtual enumNucPartsType GetType() const;
  bool operator==(const Frustum& obj);
  double getNormalizedThickness(int layer, PinSubPart::End end);
  void setNormalizedThickness(int layer, PinSubPart::End end, double thick);
  double getRadius(int layer, PinSubPart::End end);
  virtual void reverseRadii()
  {
    double t = r[0];
    r[0] = r[1];
    r[1] = t;
  }


  std::vector<PinSubPart *> split(std::vector<double>::const_iterator b,
                                  std::vector<double>::const_iterator end);

  PinSubPart * clone() const;

  virtual double getRadius(PinSubPart::End end) const
  {
    return r[end];
  }

  virtual void setRadius(PinSubPart::End end, double rin)
  {
    this->r[end] = rin;
  }

  double r[2];
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

  enumNucPartsType GetType() const;

  int NumberOfSections() const;

  void RemoveSection(AssyPartObj* obj);

  double Radius(int idx) const;
  QPointer<cmbNucMaterial> Material(int layer);

  void SetRadius(int idx, double radius);
  void SetMaterial(int idx, QPointer<cmbNucMaterial> material);

  void InsertLayer(int layer);
  void DeleteLayer(int layer);

  QColor GetLegendColor() const;

  void SetLegendColor(const QColor& color);

  int GetNumberOfLayers();

  void SetNumberOfLayers(int numLayers);

  bool fill(PinCell const* other);

  //These take ownership
  void AddPart(PinSubPart * part);

  PinSubPart* GetPart(int i) const;
  size_t GetNumberOfParts() const;

  PinConnection* GetConnection() const;
  QSet< cmbNucMaterial* > getMaterials();

  virtual std::string getTitle(){ return "PinCell: " + AssyPartObj::getTitle(); }

  QPointer<cmbNucMaterial> getCellMaterial();
  void setCellMaterial(QPointer<cmbNucMaterial> material);
  bool cellMaterialSet() const;

  vtkBoundingBox computeBounds( bool isHex );

  QColor legendColor;
  vtkSmartPointer<vtkMultiBlockDataSet> CachedData;
  bool cutaway;

  std::vector<double> getPinLayers() const;

  void splitPin(std::vector<double> const& layers);

  void setHeight(double nh);

  double getZ0() const;
  void setZ0(double z0);

  bool operator==(PinCell const& other);

protected:
  std::vector<PinSubPart*> Parts;
  PinConnection * Connection;
  cmbNucMaterialLayer CellMaterial;

  void sort();
  void connectSubPart(PinSubPart * part);
};

#endif
