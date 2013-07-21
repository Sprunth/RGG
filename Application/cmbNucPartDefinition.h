#ifndef __cmbNucPartDefinition_h
#define __cmbNucPartDefinition_h

#include <vector>

enum enumNucPartsType
{
  ASSY_DUCTCELL=0,
  ASSY_RECT_DUCT,
  ASSY_HEX_DUCT,
  ASSY_LATTICE,
  ASSY_PINCELL,
  ASSY_CYLINDER_PIN,
  ASSY_FRUSTUM_PIN,
  ASSY_MATERIAL,
  ASSY_BASEOBJ
};

  class AssyPartObj
  {
  public:
    AssyPartObj(){}
    virtual enumNucPartsType GetType() {return ASSY_BASEOBJ;}
    bool operator==(const AssyPartObj&){return false;}
    template<class T> void removeObj(const T& obj, std::vector<T>& objs)
      {
      for(typename std::vector<T>::iterator fit=objs.begin();
        fit!=objs.end(); ++fit)
        {
        if(*fit == obj)
          {
          objs.erase(fit);
          break;
          }
        }
      }
  };

  class Cylinder : public AssyPartObj
  {
  public:
    Cylinder()
    {
    x=0.0;y=0.0;z1=0.0;z2=4.0;r=1.6;material="";
    }
    enumNucPartsType GetType()
    { return ASSY_CYLINDER_PIN;}
    bool operator==(const Cylinder& obj)
    {
    return this->x==obj.x && this->y==obj.y &&
            this->z1==obj.z1 && this->z2==obj.z2 &&
            this->r==obj.r && this->material==obj.material;
    } 
    double x;
    double y;
    double z1;
    double z2;
    double r;
    std::string material;
  };

  class Frustum : public AssyPartObj
  {
  public:
    Frustum()
      {
      x=0.0;y=0.0;z1=4.0;z2=8.0;r1=1.6;r2=1.4;material="";
      }
    enumNucPartsType GetType()
    { return ASSY_FRUSTUM_PIN;}
    bool operator==(const Frustum& obj)
      {
      return this->x==obj.x && this->y==obj.y &&
        this->z1==obj.z1 && this->z2==obj.z2 &&
        this->r1==obj.r2 && this->r2==obj.r2 &&
        this->material==obj.material;
      } 
    double x;
    double y;
    double z1;
    double z2;
    double r1;
    double r2;
    std::string material;
  };

  class PinCell : public AssyPartObj
  {
  public:
    PinCell()
      {
      pitchX=4.0;pitchY=4.0;pitchZ=4.0;name=label="newpin";
      }
    enumNucPartsType GetType()
    { return ASSY_PINCELL;}
    void RemoveCylinder(const Cylinder& cylinder)
      {
      this->removeObj(cylinder, this->cylinders);
      }
    void RemoveFrustum(const Frustum& frustum)
      {
      this->removeObj(frustum, this->frustums);
      }
    
    std::string name;
    std::string label;
    double pitchX;
    double pitchY;
    double pitchZ;
    std::vector<Cylinder> cylinders;
    std::vector<Frustum> frustums;
  };

  class Duct : public AssyPartObj
  {
  public:
    Duct()
      {
      x=0.0;y=0.0;z1=0.0;z2=4.0;
      }
    enumNucPartsType GetType()
    { return ASSY_RECT_DUCT;}
    bool operator==(const Duct& obj)
      {
      return this->x==obj.x && this->y==obj.y &&
        this->z1==obj.z1 && this->z2==obj.z2 &&
        this->materials==obj.materials &&
        this->thicknesses==obj.thicknesses;
      } 
    double x;
    double y;
    double z1;
    double z2;
    std::vector<std::string> materials;
    std::vector<double> thicknesses;
  };

  class Material : public AssyPartObj
  {
  public:
    Material()
      {
      name=label="newmaterial";
      }
    enumNucPartsType GetType()
    { return ASSY_MATERIAL;}
    std::string name;
    std::string label;
  };

  class DuctCell : public AssyPartObj
  {
  public:
    DuctCell(){}
    enumNucPartsType GetType()
    { return ASSY_DUCTCELL;}
    void RemoveDuct(const Duct& duct)
    {
    this->removeObj(duct, this->Ducts);
    }
    std::vector<Duct> Ducts;
  };

  class Lattice : public AssyPartObj
    {
  public:
    Lattice()
      {
      this->SetDimensions(4, 4);
      }

    // Sets the dimensions of the cell assembly.
    void SetDimensions(int i, int j)
      {
      this->Grid.resize(i);
      for(int k = 0; k < i; k++)
        {
        this->Grid[k].resize(j);
        }
      }
    // Returns the dimensions of the cell assembly.
    std::pair<int, int> GetDimensions() const
      {
      return std::make_pair((int)this->Grid.size(), (int)this->Grid[0].size());
      }
    // Sets the contents of the cell (i, j) to name.
    void SetCell(int i, int j, const std::string &name)
      {
      this->Grid[i][j] = name;
      }
    // Returns the contents of the cell (i, j).
    std::string GetCell(int i, int j) const
      {
      return this->Grid[i][j];
      }
    // Clears the contents of the cell (i, j). This is equivalent
    // to calling SetCell(i, j, "xx").
    void ClearCell(int i, int j)
      {
      this->SetCell(i, j, "xx");
      }
    enumNucPartsType GetType()
      { return ASSY_LATTICE;}
    std::vector<std::vector<std::string> > Grid;
    };

#endif
