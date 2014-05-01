#ifndef _cmbNucDuctCell_h_
#define _cmbNucDuctCell_h_
#include "cmbNucPartDefinition.h"

#include <QObject>
#include <QSet>

class DuctConnection : public QObject
{
  Q_OBJECT
public:
signals:
  void Changed();
};

class Duct : public AssyPartObj
{
public:
  Duct(enumNucPartsType type=CMBNUC_ASSY_RECT_DUCT);
  ~Duct();
  DuctConnection * GetConnection();
  enumNucPartsType GetType() const;
  void SetType(enumNucPartsType type);
  double GetLayerThick(size_t layer, size_t t = 0) const;

  void SetNumberOfLayers(int i);
  size_t NumberOfLayers() const;

  double * getNormThick(int i);

  QPointer<cmbNucMaterial> getMaterial(int i);

  void setMaterial( int i, QPointer<cmbNucMaterial> mat );

  bool operator==(const Duct& obj);

  QSet< cmbNucMaterial* > getMaterials();

  double x;
  double y;
  double z1;
  double z2;
  double thickness[2];
  enumNucPartsType enType;
protected:
  std::vector<cmbNucMaterialLayer> Materials;
  DuctConnection * Connection;
};

class DuctCell : public AssyPartObj
{
public:
  DuctCell();
  ~DuctCell();
  DuctConnection * GetConnection();
  enumNucPartsType GetType() const;
  void RemoveDuct(Duct* duct);
  //Takes ownership
  void AddDuct(Duct* duct);
  size_t numberOfDucts() const;
  Duct * getDuct(int i);
  QSet< cmbNucMaterial* > getMaterials();
protected:
  std::vector<Duct*> Ducts;
  DuctConnection * Connection;
};


#endif