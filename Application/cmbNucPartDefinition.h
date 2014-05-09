#ifndef __cmbNucPartDefinition_h
#define __cmbNucPartDefinition_h

#include <vector>
#include <QColor>
#include <QDebug>
#include <QPointer>
#include <QObject>
#include <sstream>
#include <assert.h>

#include <vtkSmartPointer.h>
#include "vtkMultiBlockDataSet.h"
#include "cmbNucMaterial.h"

enum enumNucPartsType
{
  CMBNUC_CORE,
  CMBNUC_ASSEMBLY,
  CMBNUC_ASSY_DUCTCELL,
  CMBNUC_ASSY_DUCT,
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
    virtual enumNucPartsType GetType() const {return CMBNUC_ASSY_BASEOBJ;}
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
    virtual std::string getLabel() = 0;
    virtual std::string getFileName(){return "";}
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

    std::string getLabel()
      { return "Lattice"; }

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

    enumNucPartsType GetType() const
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
