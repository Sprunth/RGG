#ifndef _cmbNucDuctCell_h_
#define _cmbNucDuctCell_h_
#include "cmbNucPartDefinition.h"

#include <QObject>
#include <QSet>

#include <vector>

#include <vtkBoundingBox.h>

class DuctConnection : public QObject
{
  Q_OBJECT
public:
  virtual ~DuctConnection(){}
  void sendChange();
  void sendDelete();
signals:
  void Changed();
  void Deleted();
};

class DuctCell;

class Duct : public AssyPartObj
{
public:
  friend class DuctCell;
  Duct(double height, double thickX, double thickY);
  Duct(Duct * previous, bool resize = true);
  virtual ~Duct();
  DuctConnection * GetConnection();
  enumNucPartsType GetType() const;
  double GetLayerThick(size_t layer, size_t t = 0) const;

  void SetNumberOfLayers(int i);
  size_t NumberOfLayers() const;

  double * getNormThick(int i);

  void splitMaterialLayer( std::vector<double> const& lx,
                           std::vector<double> const& ly ); //values are normalized

  QPointer<cmbNucMaterial> getMaterial(int i);
  cmbNucMaterialLayer const& getMaterialLayer(int i) const;
  //takes ownership
  void setMaterialLayer(int i, cmbNucMaterialLayer * ml);

  void setMaterial( int i, QPointer<cmbNucMaterial> mat );

  void insertLayer( int a );
  void removeLayer( int a );

  bool operator==(const Duct& obj);

  QSet< cmbNucMaterial* > getMaterials();

  virtual std::string getTitle(){ return "Duct"; }

  void setZ1(double z)
  {
    this->z1 = z;
  }

  void setZ2(double z)
  {
    this->z2 = z;
  }

  double getZ1() const
  {
    return this->z1;
  }

  double getZ2() const
  {
    return this->z2;
  }

  double getX() const
  {
    return this->x;
  }

  void setX( double xin )
  {
    this->x = xin;
  }

  double getY()
  {
    return this->y;
  }

  void setY( double yin )
  {
    this->y = yin;
  }

  double getThickness(size_t i) const
  {
    return thickness[i];
  }

  void setThickness(size_t i, double v)
  {
    thickness[i] = v;
  }

  bool isInnerDuctMaterial(QPointer<cmbNucMaterial> blMat) const;
  QSet< cmbNucMaterial* > getInterfaceMaterials(QPointer<cmbNucMaterial> blMat);
  void removeFakeBoundaryLayer(std::string blname);
protected:
  double thickness[2];
  double x;
  double y;
  std::vector<cmbNucMaterialLayer> Materials;
  DuctConnection * Connection;
  double z1;
  double z2;
};

class DuctCell : public AssyPartObj
{
public:
  DuctCell();
  virtual ~DuctCell();
  void fill(DuctCell* other);
  DuctConnection * GetConnection();
  enumNucPartsType GetType() const;
  void RemoveDuct(Duct* duct, bool merge_prev = true);
  //Takes ownership
  void AddDuct(Duct* duct);
  size_t numberOfDucts() const;
  Duct * getDuct(int i);
  void getZRange(double & z1, double & z2);
  QSet< cmbNucMaterial* > getMaterials();
  bool GetInnerDuctSize(double & x, double & y);
  vtkBoundingBox computeBounds(bool hex);
  double getLength();
  void setLength(double l);
  void setZ0(double z0);
  virtual std::string getTitle(){ return "Duct: " + AssyPartObj::getName(); }
  bool operator==(const DuctCell& obj);
  void sort();
  bool setDuctThickness(double t1, double t2);
  bool isUsed();
  void used();
  void freed();
  std::vector<double> getDuctLayers() const;
  void uniformizeMaterialLayers();
  //assumes that the ducts z1 and z2 are in layers
  void splitDucts( std::vector<double> const& layers );
  bool isInnerDuctMaterial( QPointer<cmbNucMaterial> blMat ) const;
  QSet< cmbNucMaterial* > getInterfaceMaterials(QPointer<cmbNucMaterial> blMat);
  void removeFakeBoundaryLayer(std::string blname);
protected:
  std::vector<Duct*> Ducts;
  DuctConnection * Connection;
  int useCount;
};


#endif
