#ifndef __cmbNucPartDefinition_h
#define __cmbNucPartDefinition_h

#include <vector>

enum enumNucPartsType
{
  CMBNUC_CORE,
  CMBNUC_ASSEMBLY,
  CMBNUC_ASSY_DUCTCELL,
  CMBNUC_ASSY_RECT_DUCT,
  CMBNUC_ASSY_HEX_DUCT,
  CMBNUC_ASSY_LATTICE,
  CMBNUC_ASSY_PINCELL,
  CMBNUC_ASSY_CYLINDER_PIN,
  CMBNUC_ASSY_FRUSTUM_PIN,
  CMBNUC_ASSY_BASEOBJ
};

  class AssyPartObj
  {
  public:
    AssyPartObj(){}
    virtual enumNucPartsType GetType() {return CMBNUC_ASSY_BASEOBJ;}
    bool operator==(const AssyPartObj&){return false;}
    template<class T> static void removeObj(T* obj, std::vector<T*>& objs)
      {
      for(typename std::vector<T*>::iterator fit=objs.begin();
        fit!=objs.end(); ++fit)
        {
        if(*fit == obj)
          {
          delete obj;
          objs.erase(fit);
          break;
          }
        }
      }
    template<class T> static void deleteObjs(std::vector<T*>& objs)
      {
      for(typename std::vector<T*>::iterator fit=objs.begin();
        fit!=objs.end(); ++fit)
        {
        if(*fit)
          {
          delete *fit;
          }
        }
      objs.clear();
      }
  };

  class Cylinder : public AssyPartObj
  {
  public:
    Cylinder() : materials(1)
    {
    x=0.0;y=0.0;z1=0.0;z2=4.0;r=1.6;
    }
    enumNucPartsType GetType()
    { return CMBNUC_ASSY_CYLINDER_PIN;}
    bool operator==(const Cylinder& obj)
    {
    return this->x==obj.x && this->y==obj.y &&
            this->z1==obj.z1 && this->z2==obj.z2 &&
            this->r==obj.r;
    }
    std::string GetMaterial(int i)
      {
      if(i>=0 && i<this->materials.size())
        {
        return this->materials[i];
        }
      return std::string();
      }
    void SetMaterial(int i, const std::string& material)
      {
      if(i>=0 && i<this->materials.size())
        {
        this->materials[i] = material;
        }
      }
    void SetNumberOfLayers(int numLayers)
      {
      this->materials.resize(numLayers);
      }

    std::vector<std::string> materials;
    double x;
    double y;
    double z1;
    double z2;
    double r;
  };

  class Frustum : public AssyPartObj
  {
  public:
    Frustum() : materials(1)
      {
      x=0.0;y=0.0;z1=4.0;z2=8.0;r1=1.6;r2=1.4;
      }
    enumNucPartsType GetType()
    { return CMBNUC_ASSY_FRUSTUM_PIN;}
    bool operator==(const Frustum& obj)
      {
      return this->x==obj.x && this->y==obj.y &&
        this->z1==obj.z1 && this->z2==obj.z2 &&
        this->r1==obj.r2 && this->r2==obj.r2;
      }
    std::string GetMaterial(int i)
      {
      if(i>=0 && i<this->materials.size())
        {
        return this->materials[i];
        }
      return std::string();
      }
    void SetMaterial(int i, const std::string& material)
      {
      if(i>=0 && i<this->materials.size())
        {
        this->materials[i] = material;
        }
      }   
    void SetNumberOfLayers(int numLayers)
    {
      this->materials.resize(numLayers);
    }
    std::vector<std::string> materials;
    double x;
    double y;
    double z1;
    double z2;
    double r1;
    double r2;
  };

  class PinCell : public AssyPartObj
  {
  public:
    PinCell()
      : radii(1)
      {
      pitchX=0.0;
      pitchY=0.0;
      pitchZ=0.0;
      name=label="p1";
      radii[0] = 1.0;
      }
    ~PinCell()
      {
      this->deleteObjs(this->cylinders);
      this->deleteObjs(this->frustums);
      }

    enumNucPartsType GetType()
    { return CMBNUC_ASSY_PINCELL;}
    void RemoveSection(AssyPartObj* obj)
      {
      if(!obj)
        {
        return;
        }
      if(obj->GetType() == CMBNUC_ASSY_CYLINDER_PIN)
        {
        this->RemoveCylinder(dynamic_cast<Cylinder*>(obj));
        }
      else if(obj->GetType() == CMBNUC_ASSY_FRUSTUM_PIN)
        {
        this->RemoveFrustum(dynamic_cast<Frustum*>(obj));
        }
      }

    void RemoveCylinder(Cylinder* cylinder)
      {
      this->removeObj(cylinder, this->cylinders);
      }
    void RemoveFrustum(Frustum* frustum)
      {
      this->removeObj(frustum, this->frustums);
      }
    void SetMaterial(int idx, const std::string& material)
      {
      for(size_t i = 0; i < this->cylinders.size(); i++){
        this->cylinders[i]->SetMaterial(idx, material);
        }
      for(size_t i = 0; i < this->frustums.size(); i++){
        this->frustums[i]->SetMaterial(idx, material);
        }
      }
    void RemoveMaterial(const std::string& name)
      {
      for(size_t i = 0; i < this->cylinders.size(); i++){
        for(size_t j = 0; j < this->cylinders[i]->materials.size(); j++)
          {
          if(this->cylinders[i]->materials[j] == name)
            {
            this->cylinders[i]->materials[j] = "";
            }
          }
        }
      for(size_t i = 0; i < this->frustums.size(); i++){
        for(size_t j = 0; j < this->frustums[i]->materials.size(); j++)
          {
          if(this->frustums[i]->materials[j] == name)
            {
            this->frustums[i]->materials[j] = "";
            }
          }
        }
      }

    int GetNumberOfLayers()
      {
      if(this->cylinders.size() > 0)
        {
        return this->cylinders[0]->materials.size();
        }
      else if(this->frustums.size() > 0)
        {
        return this->frustums[0]->materials.size();
        }
      return 0;
      }
    void SetNumberOfLayers(int numLayers)
      {
      for(size_t i = 0; i < this->cylinders.size(); i++){
        this->cylinders[i]->SetNumberOfLayers(numLayers);
        }
      for(size_t i = 0; i < this->frustums.size(); i++){
        this->frustums[i]->SetNumberOfLayers(numLayers);
        }
      size_t curNum = this->radii.size();
      this->radii.resize(numLayers);
      for(size_t i=curNum; i<numLayers; i++)
        {
        this->radii[i] = 1.0;
        }
      }
    std::string name;
    std::string label;
    double pitchX;
    double pitchY;
    double pitchZ;
    std::vector<Cylinder*> cylinders;
    std::vector<Frustum*> frustums;
    std::vector<double> radii;
  };

  class Duct : public AssyPartObj
  {
  public:
    Duct()
      {
      x=0.0;y=0.0;z1=0.0;z2=4.0;
      materials.push_back("");
      thicknesses.push_back(18.0);
      thicknesses.push_back(18.0);
      }
    enumNucPartsType GetType()
    { return CMBNUC_ASSY_RECT_DUCT;}
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

  class DuctCell : public AssyPartObj
  {
  public:
    DuctCell(){}
    ~DuctCell()
    {
    this->deleteObjs(this->Ducts);
    }
    enumNucPartsType GetType()
    { return CMBNUC_ASSY_DUCTCELL;}
    void RemoveDuct(Duct* duct)
    {
    this->removeObj(duct, this->Ducts);
    }
    std::vector<Duct*> Ducts;
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
    // Returns the contents of the cell (I).
    std::string GetCell(int i) const
      {
          // Convert to j,k
          int s = (int)this->Grid.size();
          int j = i / s;
          int k = i - (j*s);
          return this->Grid[j][k];
      }
    // Clears the contents of the cell (i, j). This is equivalent
    // to calling SetCell(i, j, "xx").
    void ClearCell(int i, int j)
      {
      this->SetCell(i, j, "xx");
      }
    enumNucPartsType GetType()
      { return CMBNUC_ASSY_LATTICE;}
    std::vector<std::vector<std::string> > Grid;
    };

#endif
