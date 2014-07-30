#ifndef __cmbNucLattice_h__
#define __cmbNucLattice_h__

#include <QColor>
#include <QStringList>

#include <vector>
#include <string>

#include "cmbNucPartDefinition.h"

class Lattice : public AssyPartObj
{
public:

  // Represents a cell in the lattice view widget, containing
  // a label and a color.
  struct LatticeCell
  {
    LatticeCell() : label("xx"), color(Qt::white), valid(true) {}
    void setInvalid()
    {
      label = "";
      color = Qt::black;
      valid = false;
    }

    std::string label;
    QColor color;
    bool valid;
  };

  Lattice();

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
  int GetGeometrySubType();

  std::vector<std::vector<LatticeCell> > Grid;
  enumGeometryType enGeometryType;
  int subType;
};

class LatticeContainer: public AssyPartObj
{
public:
  Lattice & getLattice()
  { return this->lattice; }
  virtual void fillList(QStringList & l) = 0;
  virtual AssyPartObj * getFromLabel(const std::string &) = 0;
  virtual bool IsHexType() = 0;
protected:
  Lattice lattice;
};

#endif
