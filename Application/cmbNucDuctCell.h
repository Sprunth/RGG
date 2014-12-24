#ifndef _cmbNucDuctCell_h_
#define _cmbNucDuctCell_h_
#include "cmbNucPartDefinition.h"

#include <QObject>
#include <QSet>

#include <vtkBoundingBox.h>

class DuctConnection : public QObject
{
  Q_OBJECT
public:
  void sendChange();
signals:
  void Changed();
};

class DuctCell;

class Duct : public AssyPartObj
{
public:
  friend class DuctCell;
  Duct(double height, double thickX, double thickY);
  Duct(Duct * previous, bool resize = true);
  ~Duct();
  DuctConnection * GetConnection();
  enumNucPartsType GetType() const;
  double GetLayerThick(size_t layer, size_t t = 0) const;

  void SetNumberOfLayers(int i);
  size_t NumberOfLayers() const;

  double * getNormThick(int i);

  QPointer<cmbNucMaterial> getMaterial(int i);

  void setMaterial( int i, QPointer<cmbNucMaterial> mat );

  void insertLayer( int a );
  void removeLayer( int a );

  bool operator==(const Duct& obj);

  QSet< cmbNucMaterial* > getMaterials();

  std::string getLabel(){return label;}
  virtual std::string getTitle(){ return "Duct: " + label; }

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

  double x;
  double y;
  double thickness[2];
  std::string label;
protected:
  std::vector<cmbNucMaterialLayer> Materials;
  DuctConnection * Connection;
  double z1;
  double z2;
};

class DuctCell : public AssyPartObj
{
public:
  DuctCell();
  ~DuctCell();
  void fill(DuctCell* other);
  DuctConnection * GetConnection();
  enumNucPartsType GetType() const;
  void RemoveDuct(Duct* duct, bool merge_prev = true);
  //Takes ownership
  void AddDuct(Duct* duct);
  size_t numberOfDucts() const;
  Duct * getDuct(int i);
  Duct * getPrevious();
  void getZRange(double & z1, double & z2);
  QSet< cmbNucMaterial* > getMaterials();
  bool GetInnerDuctSize(double & x, double & y);
  vtkBoundingBox computeBounds(bool hex);
  double getLength();
  void setLength(double l);
  virtual std::string getTitle(){ return "Duct: " + AssyPartObj::getTitle(); }
  bool operator==(const DuctCell& obj);
  void sort();
protected:
  std::vector<Duct*> Ducts;
  DuctConnection * Connection;
};


#endif
