#ifndef __cmbNucLattice_h__
#define __cmbNucLattice_h__

#include <QColor>
#include <QStringList>

#include <vector>
#include <string>
#include <map>
#include <cassert>

#include "cmbNucPartDefinition.h"

class Lattice : public AssyPartObj
{
public:

  enum CellDrawMode{RECT, HEX_FULL, HEX_FULL_30,
                    HEX_SIXTH_FLAT_BOTTOM, HEX_SIXTH_FLAT_CENTER, HEX_SIXTH_FLAT_TOP,
                    HEX_SIXTH_VERT_BOTTOM, HEX_SIXTH_VERT_CENTER, HEX_SIXTH_VERT_TOP,
                    HEX_TWELFTH_BOTTOM,    HEX_TWELFTH_CENTER,    HEX_TWELFTH_TOP };

  static std::string generate_string(std::string, CellDrawMode);

  // Represents a cell in the lattice view widget, containing
  // a label and a color.
  struct LatticeCell
  {
    LatticeCell() : label("xx"), color(Qt::white), valid(true), count(0) {}
    LatticeCell( LatticeCell const& other) : label(other.label), color(other.color), valid(other.valid), count(0) {}
    void setInvalid()
    {
      label = "";
      color = Qt::black;
      valid = false;
    }

    bool isBlank() const
    { return label.empty() || label == "xx" || label == "XX"; }

    std::string label;
    QColor color;
    bool valid;
    void inc()
    { count++; }
    void dec()
    { count--; }
    unsigned int getCount() const {return count;}
    protected:
    unsigned int count;
  };

  Lattice();

  Lattice( Lattice const& other );

  ~Lattice();

  Lattice& operator=(Lattice const& arg);

  std::string getLabel();

  virtual std::string getTitle();

  void setInvalidCells();

  // Sets the dimensions of the cell assembly.
  // For Hex type, i is number of layers, j will be ignored.
  // To force a resize of all layers, pass reset=true
  void SetDimensions(int i, int j, bool reset = false);

  // Returns the dimensions of the cell assembly.
  // For Hex type, first is number of layers, second is set to 6 (for hex)
  std::pair<int, int> GetDimensions() const;

  // Sets the contents of the cell (i, j) to name.
  // For Hex type, i is layer/ring index, j is index on that layer
  void SetCell(int i, int j, const std::string &name,
               const QColor& color, bool valid = true);
  void SetCell(int i, int j, const std::string &name);
  void SetCellColor(const std::string & label, const QColor& color);

  void setAsInvalid(int i, int j);

  // Returns the contents of the cell (i, j).
  // For Hex type, i is layer/ring index, j is index on that layer
  LatticeCell GetCell(int i, int j) const;

  // Returns the contents of the cell (I).
  LatticeCell GetCell(int i) const;

  // Clears the contents of the cell (i, j). This is equivalent
  // to calling SetCell(i, j, "xx").
  // For Hex type, i is layer/ring index, j is index on that layer
  void ClearCell(int i, int j);
  bool ClearCell(const std::string &label);

  bool getValidRange(int layer, int & start, int & end) const;

  bool replaceLabel(const std::string &oldL, const std::string &newL);

  enumNucPartsType GetType() const;

  // get number of cells
  int GetNumberOfCells();

  // get/set Geometry type (hex or rect)
  void SetGeometryType(enumGeometryType type);

  enumGeometryType GetGeometryType();

  void SetGeometrySubType(int type);
  int GetGeometrySubType() const;

  size_t getSize() const
  {return Grid.size();}
  size_t getSize(unsigned int i)const
  { return Grid[i].size(); }

  bool labelUsed(const std::string &l) const;

  Lattice::CellDrawMode getDrawMode(int index, int layer) const;

  void setFullCellMode(Lattice::CellDrawMode m)
  {
    if(this->enGeometryType == HEXAGONAL)
    {
      FullCellMode = m;
    }
    else
    {
      FullCellMode = RECT;
    }
  }

  Lattice::CellDrawMode getFullCellMode() const
  {
    assert(FullCellMode == RECT || FullCellMode == HEX_FULL || FullCellMode == HEX_FULL_30);
    return FullCellMode;
  }

  bool fillRing(int r, int c, std::string const& label);

protected:
  class LatticeCellReference
  {
  public:
    LatticeCellReference(): cell(NULL)
    {}
    LatticeCellReference(LatticeCellReference const& other)
    : cell(other.cell)
    {
      if(cell) cell->inc();
    }
    ~LatticeCellReference()
    { if(cell) cell->dec();  }

    void setCell(LatticeCell * c)
    {
      if(c == cell) return;
      if(cell && cell->getCount()) cell->dec();
      if(c) c->inc();
      cell = c;
    }

    LatticeCell * getCell() { return cell; }
    LatticeCell const* getCell() const { return cell; }

  protected:
    LatticeCell * cell;
  };

  LatticeCell * getCell(std::string label);

  void setUpGrid(Lattice const & other);

  std::vector< std::vector<LatticeCellReference> > Grid;
  std::map<std::string, LatticeCell*> LabelToCell;
  enumGeometryType enGeometryType;
  int subType;
  std::vector< std::pair<int, int> > validRange;
  void computeValidRange();
  Lattice::CellDrawMode FullCellMode;
};

class LatticeContainer: public AssyPartObj
{
public:
  Lattice & getLattice()
  { return this->lattice; }
  virtual QString extractLabel(QString const&) = 0;
  virtual void fillList(QStringList & l) = 0;
  virtual AssyPartObj * getFromLabel(const std::string &) = 0;
  virtual bool IsHexType() = 0;
  virtual void calculateRectPt(unsigned int i, unsigned int j, double pt[2]) = 0;
  virtual void calculateRectTranslation(double lastPt[2], double & transX, double & transY) = 0;
  virtual void setUsedLabels(std::map<QString, int> const& labels) = 0;
protected:
  Lattice lattice;
};

#endif
