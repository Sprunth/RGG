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

void Lattice::SetDimensions(int i, int j, bool reset)
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
          this->Grid[k][0].valid = true;
        }
        else
        {
          // for each layer, we need 6*Layer cells
          int cols = 6*k;
          this->Grid[k].resize(cols);
          for(int j = 0; j < cols; j++)
          {
            int start  = 0;
            if(subType != 0 && !(subType & ANGLE_360))
            {
              start = (subType & FLAT)?(k):(k-(k)/2);
              cols = ((subType & FLAT)?(k+1):(((k+1)-(k+2)%2)))+start;
              if(subType & ANGLE_30)
              {
                start = 2*k - k/2;
                cols = (k%2 ? (k+1)/2 :(k+2)/2) + start;
              }
            }
            if(start <= j && j < cols)
            {
              this->Grid[k][j].label = "xx";
              this->Grid[k][j].color = Qt::white;
              this->Grid[k][j].valid = true;
            }
            else
            {
              this->Grid[k][j].label = "";
              this->Grid[k][j].color = Qt::black;
              this->Grid[k][j].valid = false;
            }
          }
        }
      }
    }
  }
}

std::pair<int, int> Lattice::GetDimensions() const
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
  true;
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

void Lattice::replaceLabel(const std::string &oldL, const std::string &newL)
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
    return 1 + 3*(int)this->Grid.size()*((int)this->Grid.size() - 1);
  }
  else
  {
    return (int)(this->Grid.size()*this->Grid[0].size());
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
