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
  void ColorChanged();
};

class Duct : public AssyPartObj
{
public:
  struct MaterialLayer : public cmbNucMaterialLayer
  {
    MaterialLayer();
    MaterialLayer(const MaterialLayer & ml);
    double normThickness[2];
    bool operator==( const Duct::MaterialLayer & other ) const;
  };
  Duct(enumNucPartsType type=CMBNUC_ASSY_RECT_DUCT);
  ~Duct();
  DuctConnection * GetConnection();
  enumNucPartsType GetType();
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
  std::vector<Duct::MaterialLayer> Materials;
  DuctConnection * Connection;
};

class DuctCell : public AssyPartObj
{
public:
  DuctCell();
  ~DuctCell();
  DuctConnection * GetConnection();
  enumNucPartsType GetType();
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
