#include "cmbNucLattice.h"
#include "cmbNucPartDefinition.h"

Lattice::Lattice()
{
  this->enGeometryType = RECTILINEAR;
  this->SetDimensions(4, 4);
  subType = FLAT | ANGLE_360;
}

std::string Lattice::getLabel()
{ return "Lattice"; }

std::string Lattice::getTitle(){ return "Lattice"; }

void Lattice::setInvalidCells()
{
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
      this->Grid[i][j].setInvalid();
    }
    for( size_t j = cols; j < this->Grid[i].size(); j++)
    {
      this->Grid[i][j].setInvalid();
    }
  }
}

void Lattice::SetDimensions(int iin, int jin, bool reset)
{
  if(this->enGeometryType == RECTILINEAR)
  {
    this->Grid.resize(iin);
    for(int k = 0; k < iin; k++)
    {
      this->Grid[k].resize(jin);
    }
  }
  else if(this->enGeometryType == HEXAGONAL)
  {
    int current = reset ? 0 : this->Grid.size();
    if(current == iin)
    {
      return;
    }

    this->Grid.resize(iin);

    if(iin > current)
    {
      for(int k = current; k < iin; k++)
      {
        if(k == 0)
        {
          this->Grid[k].resize(1);
          this->Grid[k][0].label = "xx";
          this->Grid[k][0].color = Qt::white;
          this->Grid[k][0].valid = true;
        }
        else
        {
          // for each layer, we need 6*Layer cells
          int cols = 6*k;
          this->Grid[k].resize(cols);
          int start, end;
          getValidRange(k, start, end);
          for(int c = 0; c < cols; c++)
          {
            if(start <= c && c <= end)
            {
              this->Grid[k][c].label = "xx";
              this->Grid[k][c].color = Qt::white;
              this->Grid[k][c].valid = true;
            }
            else
            {
              this->Grid[k][c].label = "";
              this->Grid[k][c].color = Qt::black;
              this->Grid[k][c].valid = false;
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
    start = 0;
    end = 6*layer-1;
    if(subType != 0 && !(subType & ANGLE_360))
    {
      start = (subType & FLAT)?(layer):(layer-(layer)/2);
      end = ((subType & FLAT)?(layer+1):(((layer+1)-(layer+2)%2)))+start-1;
      if(subType & ANGLE_30)
      {
        start = 2*layer - layer/2;
        end = (layer%2 ? (layer+1)/2 :(layer+2)/2) + start - 1;
      }
    }
    return true;
  }
  return false;
}

std::pair<int, int> Lattice::GetDimensions() const
{
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
  this->Grid[i][j].label = name;
  this->Grid[i][j].color = color;
  this->Grid[i][j].valid = valid;
}

void Lattice::SetCell(int i, int j, const std::string &name)
{
  this->Grid[i][j].label = name;
  this->Grid[i][j].color = Qt::white;
}

void Lattice::setAsInvalid(int i, int j)
{
  this->Grid[i][j].setInvalid();
}

Lattice::LatticeCell Lattice::GetCell(int i, int j) const
{
  return this->Grid[i][j];
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
    return this->Grid[j][k];
  }
  else
  {
    int size = static_cast<int>(this->Grid[0].size());
    int j = i / size;
    int k = i - (j*size);
    assert(j < static_cast<int>(this->Grid.size()));
    assert(k < static_cast<int>(this->Grid[j].size()));

    return this->Grid[j][k];
  }
}

void Lattice::ClearCell(int i, int j)
{
  this->SetCell(i, j, "xx", Qt::white);
}

bool Lattice::ClearCell(const std::string &label)
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

bool Lattice::replaceLabel(const std::string &oldL, const std::string &newL)
{
  bool result = false;
  for(size_t i = 0; i < this->Grid.size(); i++)
  {
    for(size_t j = 0; j < this->Grid[i].size(); j++)
    {
      if(this->GetCell(i, j).label == oldL)
      {
        result = true;
        this->SetCell(i, j, newL, Qt::white);
      }
    }
  }
  return result;
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

int Lattice::GetGeometrySubType()
{ return subType;}
