#include "cmbNucLattice.h"
#include "cmbNucPartDefinition.h"
#include <algorithm>
#include <cassert>

Lattice::Lattice()
:enGeometryType(RECTILINEAR),
 subType(FLAT | ANGLE_360)
{
  this->SetDimensions(4, 4);
}

Lattice::Lattice( Lattice const& other )
:enGeometryType(other.enGeometryType),
 subType(other.subType), FullCellMode(other.FullCellMode)
{
  setUpGrid(other);
  this->validRange = other.validRange;
}

Lattice::~Lattice()
{
  Grid.clear();
  for(std::map<std::string, LatticeCell*>::iterator i = LabelToCell.begin(); i != LabelToCell.end(); ++i)
  {
    delete i->second;
    i->second = NULL;
  }
  LabelToCell.clear();
}

Lattice& Lattice::operator=(Lattice const& other)
{
  //TODO update grid
  setUpGrid(other);
  this->subType = other.subType;
  this->enGeometryType = other.enGeometryType;
  this->FullCellMode = other.FullCellMode;
  this->validRange = other.validRange;
  return *this;
}

void Lattice::setUpGrid(Lattice const & other)
{
  this->Grid.clear();
  this->Grid.resize(other.Grid.size());
  for(unsigned int i = 0; i < other.Grid.size(); ++i)
  {
    this->Grid[i].resize(other.Grid[i].size());
  }

  for(std::map<std::string, LatticeCell*>::iterator i = this->LabelToCell.begin();
      i != this->LabelToCell.end(); ++i)
  {
    delete i->second;
    i->second = NULL;
  }

  this->LabelToCell.clear();

  for(std::map<std::string, LatticeCell*>::const_iterator i = other.LabelToCell.begin();
      i != other.LabelToCell.end(); ++i)
  {
    if(LabelToCell[i->first]!= NULL) delete LabelToCell[i->first];
    LabelToCell[i->first] = new LatticeCell(*(i->second));
  }
  //TODO update grid
  this->Grid.clear();
  this->Grid.resize(other.Grid.size());
  for(unsigned int i = 0; i < other.Grid.size(); ++i)
  {
    this->Grid[i].resize(other.Grid[i].size());
  }
  for(unsigned int i = 0; i < other.Grid.size(); ++i)
  {
    for(unsigned j = 0; j < other.Grid[i].size(); ++j)
    {
      LatticeCell * c = this->getCell(other.Grid[i][j].getCell()->label);
      int oc = c->getCount();
      (void)(oc);
      this->Grid[i][j].setCell(c);
      assert(c->getCount() == static_cast<unsigned int>(oc) + 1);
    }
  }
  this->computeValidRange();
}

std::string Lattice::getTitle(){ return "Lattice"; }

void Lattice::setInvalidCells()
{
  LatticeCell * invalid = this->getCell("");
  invalid->setInvalid();
  for(size_t i = 0; i < this->Grid.size(); i++)
  {
    size_t start = (subType & FLAT)?(i):(i-(i)/2);
    size_t cols = ((subType & FLAT)?(i+1):(((i+1)-(i+2)%2)))+start;
    if(subType & ANGLE_30)
    {
      start = 2*i - i/2;
      cols = (i%2 ? (i+1)/2 :(i+2)/2) + start;
    }
    for(unsigned int j = 0; j < start; ++j)
    {
      this->Grid[i][j].setCell(invalid);
    }
    for( size_t j = cols; j < this->Grid[i].size(); j++)
    {
      this->Grid[i][j].setCell(invalid);
    }
  }
}

void Lattice::SetDimensions(int iin, int jin, bool reset)
{
  LatticeCell * invalid = this->getCell("");
  invalid->setInvalid();
  LatticeCell * XX = this->getCell("xx");
  XX->color = Qt::white;
  XX->valid = true;
  if(this->enGeometryType == RECTILINEAR)
  {
    this->Grid.resize(iin);
    for(int k = 0; k < iin; k++)
    {
      int old = static_cast<int>(reset?0:this->Grid[k].size());
      this->Grid[k].resize(jin);
      for(int r = old; r < jin; ++r)
      {
        this->Grid[k][r].setCell(XX);
      }
    }
  }
  else if(this->enGeometryType == HEXAGONAL)
  {
    int current = reset ? 0 : static_cast<int>(this->Grid.size());
    if(current == iin)
    {
      return;
    }

    this->Grid.resize(iin);

    this->computeValidRange();

    if(iin > current)
    {
      for(int k = current; k < iin; k++)
      {
        if(k == 0)
        {
          this->Grid[k].resize(1);
          this->Grid[k][0].setCell(XX);
        }
        else
        {
          // for each layer, we need 6*Layer cells
          int cols = 6*k;
          this->Grid[k].resize(cols);
          int start = 0, end = 0;
          if(!getValidRange(k, start, end)) continue;
          for(int c = 0; c < cols; c++)
          {
            if(start <= c && c <= end)
            {
              this->Grid[k][c].setCell(XX);
            }
            else
            {
              this->Grid[k][c].setCell(invalid);
            }
          }
        }
      }
    }
  }
}

bool Lattice::getValidRange(int layer, int & start, int & end) const
{
  if(this->enGeometryType == HEXAGONAL)
  {
    start = this->validRange[layer].first;
    end = this->validRange[layer].second;
    return true;
  }
  return false;
}

void Lattice::computeValidRange()
{
  if(this->enGeometryType == HEXAGONAL)
  {
    this->validRange.resize(Grid.size());
    for( std::size_t layer = 0; layer < Grid.size(); ++layer)
    {
      this->validRange[layer].first = 0;
      if(layer == 0)
      {
        this->validRange[layer].second = 0;
      }
      else
      {
        const int tl = static_cast<int>(layer);
        this->validRange[layer].second = 6 * tl - 1;
        if(subType != 0 && !(subType & ANGLE_360))
        {
          this->validRange[layer].first = (subType & FLAT)?(tl):(tl - (tl / 2));
          this->validRange[layer].second =  ((subType & FLAT)?(tl + 1):
                                                              ((tl + 1) - (tl + 2) % 2))+
                                                                this->validRange[layer].first-1;
          if(subType & ANGLE_30)
          {
            this->validRange[layer].first = 2*tl - tl/2;
            this->validRange[layer].second = (layer%2 ? (tl+1)/2 :(tl+2)/2) + 
                                                                  this->validRange[layer].first - 1;
          }
        }
      }
    }
  }
}

std::pair<int, int> Lattice::GetDimensions() const
{
  if(this->Grid.size() == 0) return std::make_pair(0,0);
  if(this->enGeometryType == RECTILINEAR)
  {
    return std::make_pair(static_cast<int>(this->Grid.size()),
                          static_cast<int>(this->Grid[0].size()));
  }
  else
  {
    return std::make_pair(static_cast<int>(this->Grid.size()), 6);
  }
}

void Lattice::SetCell(int i, int j, const std::string &name,
                      const QColor& color, bool valid)
{
  LatticeCell * cr = this->getCell(name);
  cr->color = color;
  cr->valid = valid;
  this->Grid[i][j].setCell(cr);
}

void Lattice::SetCell(int i, int j, const std::string &name)
{
  LatticeCell * cr = this->getCell(name);
  this->Grid[i][j].setCell(cr);
}

void Lattice::SetCellColor(const std::string &name, const QColor& color)
{
  LatticeCell * cr = this->getCell(name);
  cr->color = color;
}

void Lattice::setAsInvalid(int i, int j)
{
  LatticeCell * cr = this->getCell("");
  cr->setInvalid();
  this->Grid[i][j].setCell(cr);
}

Lattice::LatticeCell Lattice::GetCell(int i, int j) const
{
  return *(this->Grid[i][j].getCell());
}

Lattice::LatticeCell Lattice::GetCell(int i) const
{
  // Convert to j,k
  int s = static_cast<int>(this->Grid.size());
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
    return this->GetCell(j,k);
  }
  else
  {
    int size = static_cast<int>(this->Grid[0].size());
    int j = i / size;
    int k = i - (j*size);
    assert(j < static_cast<int>(this->Grid.size()));
    assert(k < static_cast<int>(this->Grid[j].size()));

    return this->GetCell(j,k);
  }
}

void Lattice::ClearCell(int i, int j)
{
  LatticeCell * cr = this->getCell("xx");
  this->Grid[i][j].setCell(cr);
}

bool Lattice::ClearCell(const std::string &label)
{
  bool r = false;
  for(size_t i = 0; i < this->Grid.size(); i++)
  {
    for(size_t j = 0; j < this->Grid[i].size(); j++)
    {
      if(this->GetCell(static_cast<int>(i), 
                       static_cast<int>(j)).label == label)
      {
        r = true;
        this->ClearCell(static_cast<int>(i), 
                        static_cast<int>(j));
      }
    }
  }
  return r;
}

bool Lattice::replaceLabel(const std::string &oldL, const std::string &newL)
{
  std::map<std::string, LatticeCell *>::iterator iter = this->LabelToCell.find(oldL);
  if(iter == LabelToCell.end() || iter->second->getCount() == 0)
  {
    return false;
  }
  std::map<std::string, LatticeCell *>::iterator newI = LabelToCell.find(newL);
  if(newI == LabelToCell.end()) //easy
  {
    iter->second->label = newL;
    LabelToCell[newL] = iter->second;
    LabelToCell.erase(iter);
    return true;
  }
  //harder
  for(size_t i = 0; i < this->Grid.size(); i++)
  {
    for(size_t j = 0; j < this->Grid[i].size(); j++)
    {
      if(this->Grid[i][j].getCell() == iter->second)
      {
        this->Grid[i][j].setCell(newI->second);
      }
    }
  }
  return true;
}

enumNucPartsType Lattice::GetType() const
{ return CMBNUC_ASSY_LATTICE;}

int Lattice::GetNumberOfCells()
{
  if(this->Grid.size() == 0)
  {
    return 0;
  }
  if(this->enGeometryType == HEXAGONAL)
  {
    int tmp = static_cast<int>(this->Grid.size());
    return 1 + 3*tmp*(tmp - 1);
  }
  else
  {
    return static_cast<int>(this->Grid.size()*this->Grid[0].size());
  }
}

void Lattice::SetGeometryType(enumGeometryType type)
{
  if(this->enGeometryType == type) return;
  this->enGeometryType = type;
  if(type == RECTILINEAR) this->SetDimensions(4, 4);
  else this->SetDimensions(1, 1, true);
}

enumGeometryType Lattice::GetGeometryType()
{ return this->enGeometryType; }

void Lattice::SetGeometrySubType(int type)
{subType = type;}

int Lattice::GetGeometrySubType() const
{ return subType; }

bool Lattice::labelUsed(const std::string &l) const
{
  std::map<std::string, LatticeCell *>::const_iterator iter = LabelToCell.find(l);
  if(iter == LabelToCell.end())
  {
    return false;
  }
  return iter->second->getCount() != 0;
}

Lattice::LatticeCell * Lattice::getCell(std::string label)
{
  LatticeCell * result = NULL;
  std::map<std::string, LatticeCell *>::iterator iter = LabelToCell.find(label);
  if(iter == LabelToCell.end())
  {
    result = new LatticeCell;
    result->label = label;
    LabelToCell[label] = result;
  }
  else
  {
    result = iter->second;
  }
  return result;
}

Lattice::CellDrawMode Lattice::getDrawMode(int index, int layer) const
{
  if(this->enGeometryType == RECTILINEAR) return Lattice::RECT;
  int start = 0, end = 0;
  if(!this->getValidRange(layer, start, end))
  {
    return Lattice::RECT;
  }
  if(layer == 0)
  {
    if(this->subType & ANGLE_360)
    {
      return this->getFullCellMode();
    }
    else if(this->subType & ANGLE_60 && this->subType & FLAT)
    {
      return Lattice::HEX_SIXTH_FLAT_CENTER;
    }
    else if(this->subType & ANGLE_60 && this->subType & VERTEX)
    {
      return Lattice::HEX_SIXTH_VERT_CENTER;
    }
    else if(this->subType & ANGLE_30)
    {
      return Lattice::HEX_TWELFTH_CENTER;
    }
  }
  else if(this->subType & ANGLE_360)
  {
    return FullCellMode;
  }
  else if( this->subType & ANGLE_60 && this->subType & FLAT )
  {
    if(index == start)
    {
      return Lattice::HEX_SIXTH_FLAT_TOP;
    }
    else if(index == end)
    {
      return Lattice::HEX_SIXTH_FLAT_BOTTOM;
    }
    else
    {
      return Lattice::HEX_FULL;
    }
  }
  else if( this->subType & ANGLE_60 && this->subType & VERTEX )
  {
    if(index == start && layer % 2 == 0) return Lattice::HEX_SIXTH_VERT_TOP;
    else if(index == end && layer % 2 == 0) return Lattice::HEX_SIXTH_VERT_BOTTOM;
    return Lattice::HEX_FULL_30;
  }
  else if(this->subType & ANGLE_30)
  {
    if(index == end) return Lattice::HEX_TWELFTH_BOTTOM;
    else if(index == start && layer % 2 == 0) return Lattice::HEX_TWELFTH_TOP;
  }
  return Lattice::HEX_FULL;
}

std::string Lattice::generate_string(std::string in, CellDrawMode mode)
{
  switch(mode)
  {
    case RECT:
    case HEX_FULL:
    case HEX_FULL_30:
      return in;
    case HEX_SIXTH_FLAT_BOTTOM:
    case HEX_SIXTH_VERT_BOTTOM:
    case HEX_TWELFTH_BOTTOM:
      return in+"_bottom";
    case HEX_SIXTH_FLAT_CENTER:
    case HEX_SIXTH_VERT_CENTER:
    case HEX_TWELFTH_CENTER:
      return in + "_center";
    case HEX_SIXTH_FLAT_TOP:
    case HEX_SIXTH_VERT_TOP:
    case HEX_TWELFTH_TOP:
      return in + "_top";
  }
  assert(false);
  return in;
}

bool Lattice::fillRing(int r, int c, std::string const& label)
{
  bool change = false;
  if(this->enGeometryType == RECTILINEAR)
  {
    int ring = std::min(r, std::min(c, std::min(static_cast<int>(this->Grid.size()-1-r),
                                                static_cast<int>(this->Grid[0].size()-1-c))));
    for( std::size_t i = ring; i < this->Grid.size()-ring; ++i )
    {
      if(label != Grid[i][ring].getCell()->label)
      {
        change = true;
        this->SetCell(static_cast<int>(i),ring,label);
      }
    }
    for( std::size_t i = ring; i < this->Grid[0].size()-ring; ++i )
    {
      if(label != Grid[ring][i].getCell()->label)
      {
        change = true;
        this->SetCell(ring, static_cast<int>(i), label);
      }
    }
    int tmp = static_cast<int>(this->Grid.size()-1-ring);
    for( std::size_t i = ring; i < this->Grid[0].size()-ring; ++i )
    {
      if(label != Grid[tmp][i].getCell()->label)
      {
        change = true;
        this->SetCell(static_cast<int>(tmp),
                      static_cast<int>(i), label);
      }
    }
    tmp = static_cast<int>(this->Grid[0].size()) - 1 - ring;
    for( std::size_t i = ring; i < this->Grid.size() - ring; ++i )
    {
      if(label != Grid[i][tmp].getCell()->label)
      {
        change = true;
        this->SetCell(static_cast<int>(i),
                      static_cast<int>(tmp), label);
      }
    }
  }
  else if(this->enGeometryType == HEXAGONAL)
  {
    int start, end;
    if(getValidRange(r, start, end))
    {
      for(int j = start; j <= end; ++j)
      {
        if(label != Grid[r][j].getCell()->label)
        {
          change = true;
          this->SetCell(r,j,label);
        }
      }
    }
  }
  return change;
}
