
#include "cmbNucCore.h"

#include <iostream>
#include <algorithm>
#include <fstream>
#include <limits>

#include "cmbNucAssembly.h"

#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"

cmbNucCore::cmbNucCore()
{
  this->Data = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  this->AssyemblyPitchX = this->AssyemblyPitchY = 23.5;
}

cmbNucCore::~cmbNucCore()
{
  for(std::vector<cmbNucAssembly*>::iterator fit=this->Assemblies.begin();
    fit!=this->Assemblies.end(); ++fit)
    {
    if(*fit)
      {
      delete *fit;
      }
    }
  this->Assemblies.clear();
}

void cmbNucCore::AddAssembly(cmbNucAssembly *assembly)
{
  this->Assemblies.push_back(assembly);
  // the new assembly need to be in the grid 
}

void cmbNucCore::RemoveAssembly(const std::string &label)
{
  for(size_t i = 0; i < this->Assemblies.size(); i++)
    {
    if(this->Assemblies[i]->label == label)
      {
      this->Assemblies.erase(this->Assemblies.begin() + i);
      break;
      }
    }
  // update the Grid
  std::pair<int, int> dim = this->GetDimensions();
  for(size_t i = 0; i < dim.first; i++)
    {
    for(size_t j = 0; j < dim.second; j++)
      {
      if(this->GetAssembly(i, j) == label)
        {
        this->SetAssembly(i, j, "xx");
        }
      }
    }
}

cmbNucAssembly* cmbNucCore::GetAssembly(const std::string &label)
{
  for(size_t i = 0; i < this->Assemblies.size(); i++)
    {
    if(this->Assemblies[i]->label == label)
      {
      return this->Assemblies[i];
      }
    }

  return 0;
}

vtkSmartPointer<vtkMultiBlockDataSet> cmbNucCore::GetData()
{
  if(this->Assemblies.size()==0)
    {
    return NULL;
    }

  // setup data
  this->Data->SetNumberOfBlocks(this->Assemblies.size());

  for(size_t i = 0; i < this->Grid.size(); i++)
    {
    const std::vector<std::string> &row = this->Grid[i];

    for(size_t j = 0; j < row.size(); j++)
      {
      const std::string &type = row[j];

      if(!type.empty() && type != "xx" && type != "XX")
        {
        cmbNucAssembly* assembly = this->GetAssembly(type);
        this->Data->SetBlock(i*this->Grid.size()+j, assembly->GetData());
        }
      else
        {
        this->Data->SetBlock(i*this->Grid.size()+j, NULL);
        }
      }
    }

  return this->Data;
}
