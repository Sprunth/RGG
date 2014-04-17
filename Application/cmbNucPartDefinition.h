#ifndef __cmbNucPartDefinition_h
#define __cmbNucPartDefinition_h

#include <vector>
#include <QColor>
#include <QDebug>
#include <sstream>
#include <assert.h>

#include <vtkSmartPointer.h>
#include "vtkMultiBlockDataSet.h"

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

enum enumGeometryType {
  RECTILINEAR,
  HEXAGONAL
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
    PinCell()
      : radii(1)
      {
      pitchX=0.0;
      pitchY=0.0;
      pitchZ=0.0;
      name=label="p1";
      radii[0] = 1.0;
      legendColor = Qt::white;
      cutaway = false;
      }

    ~PinCell()
      {
      this->deleteObjs(this->cylinders);
      this->deleteObjs(this->frustums);
      }

    enumNucPartsType GetType()
    { return CMBNUC_ASSY_PINCELL;}

    int NumberOfSections() const
    { return this->cylinders.size() + this->frustums.size();}

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

    double Radius(int idx) const
    {return this->radii[idx];}

    void SetRadius(int idx, double radius)
      {
      this->radii[idx] = radius;
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
    bool RemoveMaterial(const std::string& name)
      {
      bool change = false;
      for(size_t i = 0; i < this->cylinders.size(); i++){
        for(size_t j = 0; j < this->cylinders[i]->materials.size(); j++)
          {
          if(this->cylinders[i]->materials[j] == name)
            {
            this->cylinders[i]->materials[j] = "";
            change = true;
            }
          }
        }
      for(size_t i = 0; i < this->frustums.size(); i++){
        for(size_t j = 0; j < this->frustums[i]->materials.size(); j++)
          {
          if(this->frustums[i]->materials[j] == name)
            {
            this->frustums[i]->materials[j] = "";
            change = true;
            }
          }
        }
      return change;
      }

    QColor GetLegendColor() const
      {
      return this->legendColor;
      }

    void SetLegendColor(const QColor& color)
      {
      this->legendColor = color;
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
      for(size_t i = curNum; i < numLayers; i++)
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
    QColor legendColor;
    vtkSmartPointer<vtkMultiBlockDataSet> CachedData;
    bool cutaway;

  };

  class Duct : public AssyPartObj
  {
  public:
    struct Material
    {
      Material()
      {
        this->material = "";
        this->normThickness[0] = 1;
        this->normThickness[1] = 1;
      }
      std::string material;
      double normThickness[2];
      bool operator==( const Duct::Material & other ) const
      {
        return material == other.material &&
               normThickness[0] == other.normThickness[0] &&
               normThickness[1] == other.normThickness[1];
      }
    };
    Duct(enumNucPartsType type=CMBNUC_ASSY_RECT_DUCT)
      {
      x=0.0;y=0.0;z1=0.0;z2=4.0;
      Duct::Material m;
      materials.push_back(m);
      thickness[0] = 18;
      thickness[1] = 18;
      enType = type;
      }
    enumNucPartsType GetType()
    { return enType;}
    void SetType(enumNucPartsType type)
      { enType = type;}
    double GetLayerThick(size_t layer, size_t t = 0) const
    {
      return materials[layer].normThickness[t] * this->thickness[t];
    }

    bool operator==(const Duct& obj)
      {
      return this->x==obj.x && this->y==obj.y &&
        this->z1==obj.z1 && this->z2==obj.z2 &&
        this->materials==obj.materials &&
        this->thickness[0]==obj.thickness[0] &&
        this->thickness[1]==obj.thickness[1] &&
        this->enType == obj.enType;
      }
    double x;
    double y;
    double z1;
    double z2;
    double thickness[2];
    std::vector<Duct::Material> materials;
    enumNucPartsType enType;
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

  // Represents a cell in the lattice view widget, containing
  // a label and a color.
  struct LatticeCell
    {
    LatticeCell() : label("xx"), color(Qt::white) {}

    std::string label;
    QColor color;
    };

  class Lattice : public AssyPartObj
    {
  public:
    Lattice()
      {
      this->enGeometryType = RECTILINEAR;
      this->SetDimensions(4, 4);
      }

    // Sets the dimensions of the cell assembly.
    // For Hex type, i is number of layers, j will be ignored.
    // To force a resize of all layers, pass reset=true
    void SetDimensions(int i, int j, bool reset = false)
      {
      if(this->enGeometryType == RECTILINEAR)
        {
        this->Grid.resize(i);
        for(int k = 0; k < i; k++)
          {
          this->Grid[k].resize(j);
          }
        }
      else if(this->enGeometryType == HEXAGONAL)
        {
        int current = reset ? 0 : this->Grid.size();
        if(current == i)
          {
          return;
          }

        this->Grid.resize(i);

        if(i > current)
          {
          for(int k = current; k < i; k++)
            {
            if(k == 0)
              {
              this->Grid[k].resize(1);
              this->Grid[k][0].label = "xx";
              this->Grid[k][0].color = Qt::white;
              }
            else
              {
              // for each layer, we need 6*Layer cells
              this->Grid[k].resize(6 * k);
              for(int j = 0; j < 6 * k; j++)
                {
                this->Grid[k][j].label = "xx";
                this->Grid[k][j].color = Qt::white;
                }
              }
            }
          }
        }
      }
    // Returns the dimensions of the cell assembly.
    // For Hex type, first is number of layers, second is set to 6 (for hex)
    std::pair<int, int> GetDimensions() const
      {
      if(this->enGeometryType == RECTILINEAR)
        {
        return std::make_pair((int)this->Grid.size(), (int)this->Grid[0].size());
        }
      else
        {
        return std::make_pair((int)this->Grid.size(), 6);
        }
      }

    // Sets the contents of the cell (i, j) to name.
    // For Hex type, i is layer/ring index, j is index on that layer
    void SetCell(int i, int j, const std::string &name, const QColor& color)
      {
      this->Grid[i][j].label = name;
      this->Grid[i][j].color = color;
      }

    // Returns the contents of the cell (i, j).
    // For Hex type, i is layer/ring index, j is index on that layer
    LatticeCell GetCell(int i, int j) const
      {
      return this->Grid[i][j];
      }

    // Returns the contents of the cell (I).
    LatticeCell GetCell(int i) const
      {
      // Convert to j,k
      int s = (int)this->Grid.size();
      // For Hex type, This is different
      if(this->enGeometryType == HEXAGONAL)
        {
        int j = 0, k = 0;
        int totalNum = 0, preTotal;
        for(int layer = 0; layer < s; layer++)
          {
          preTotal = totalNum;
         // totalNum += (6*layer);
          totalNum = 1 + 3*layer*(layer+1);
          if(i < totalNum)
            {
            j = layer;
            k = i-preTotal;
            break;
            }
          }
        return this->Grid[j][k];
        }
      else
        {
        int s = this->Grid[0].size();
        int j = i / s;
        int k = i - (j*s);
        assert(j < this->Grid.size());
        assert(k < this->Grid[j].size());

        return this->Grid[j][k];
        }
      }

    // Clears the contents of the cell (i, j). This is equivalent
    // to calling SetCell(i, j, "xx").
    // For Hex type, i is layer/ring index, j is index on that layer
    void ClearCell(int i, int j)
      {
      this->SetCell(i, j, "xx", Qt::white);
      }
    bool ClearCell(const std::string &label)
      {
      bool r = false;
      for(size_t i = 0; i < this->Grid.size(); i++)
        {
        for(size_t j = 0; j < this->Grid[i].size(); j++)
          {
          if(this->GetCell(i, j).label == label)
            {
            r = true;
            this->ClearCell(i, j);
            }
          }
        }
      return r;
      }

    void replaceLabel(const std::string &oldL, const std::string &newL)
      {
      for(size_t i = 0; i < this->Grid.size(); i++)
        {
        for(size_t j = 0; j < this->Grid[i].size(); j++)
          {
          if(this->GetCell(i, j).label == oldL)
            {
            this->SetCell(i, j, newL, Qt::white);
            }
          }
        }
      }

    enumNucPartsType GetType()
      { return CMBNUC_ASSY_LATTICE;}

    // get number of cells
    int GetNumberOfCells()
      {
      if(this->Grid.size() == 0)
        {
        return 0;
        }
      if(this->enGeometryType == HEXAGONAL)
        {
        return 1 + 3*(int)this->Grid.size()*((int)this->Grid.size() - 1);
        }
      else
        {
        return (int)(this->Grid.size()*this->Grid[0].size());
        }
      }
    // get/set Geometry type (hex or rect)
    void SetGeometryType(enumGeometryType type)
    { this->enGeometryType = type; }
    enumGeometryType GetGeometryType()
    { return this->enGeometryType; }

    std::vector<std::vector<LatticeCell> > Grid;
    enumGeometryType enGeometryType;
    };

  class HexMap
    {
    public:
      HexMap()
        {
        this->SetNumberOfLayers(0);
        }

      // Sets the dimensions of the cell assembly.
      void SetNumberOfLayers(int layers)
        {
        int current = this->Grid.size();
        if(current == layers)
          {
          return;
          }

        this->Grid.resize(layers);

        if(layers>current )
          {
          for(int k = current; k < layers; k++)
            {
            if(k==0)
              {
              this->Grid[k].resize(1);
              this->Grid[k][0].label = "xx";
              this->Grid[k][0].color = Qt::white;
              }
            else
              {
              // for each layer, we need 6*Layer cells
              this->Grid[k].resize(6 * k);
              for(int j = 0; j < 6 * k; j++)
                {
                this->Grid[k][j].label = "xx";
                this->Grid[k][j].color = Qt::white;
                }
              }
            }
          }
        }

      // Returns the layers of the hex grid.
      int numberOfLayers() const
        {
        return (int)this->Grid.size();
        }

      // Sets the contents of the cell (i, j) to name.
      void SetCell(int layer, int idx, const std::string &name, const QColor& color)
        {
        this->Grid[layer][idx].label = name;
        this->Grid[layer][idx].color = color;
        }

      // Returns the contents of the cell (i, j).
      LatticeCell GetCell(int layer, int idx) const
        {
        return this->Grid[layer][idx];
        }

      // Clears the contents of the cell (i, j). This is equivalent
      // to calling SetCell(i, j, "xx").
      void ClearCell(int layer, int idx)
        {
        this->SetCell(layer, idx, "xx", Qt::white);
        }

      // Stored as layered vectors. For each layer of hex cells
      // we have a vector with 6*Layer cells, where Layer is the
      // index into hex layers (starting 0), and Layer > 0.
      // For Layer==0, just one cell.
      std::vector<std::vector<LatticeCell> > Grid;
    };

#endif
