
#include "cmbNucCore.h"

#include <iostream>
#include <algorithm>
#include <fstream>
#include <limits>

#include "cmbNucAssembly.h"

#include "vtkTransform.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkTransformFilter.h"

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

void cmbNucCore::SetDimensions(int i, int j)
{
  cmbNucAssembly* assy = this->GetAssembly(0);
  if(assy && assy->AssyLattice.GetGeometryType() == HEXAGONAL)
    {
    int current = this->Grid.size();
    if(current == i)
      {
      return;
      }
    this->Grid.resize(i);
    if(i>current )
      {
      for(int k = current; k < i; k++)
        {
        if(k==0)
          {
          this->Grid[k].resize(1);
          }
        else
          {
          // for each layer, we need 6*Layer cells
          this->Grid[k].resize(6*k);
          }
        }
      }
   }
  else
    {
    this->Grid.resize(i);
    for(int k = 0; k < i; k++)
      {
      this->Grid[k].resize(j);
      }
    }
}

void cmbNucCore::AddAssembly(cmbNucAssembly *assembly)
{
  if(this->Assemblies.size()==0)
    {
    this->SetDimensions(1, 1);
    }
  this->Assemblies.push_back(assembly);
  if(this->Assemblies.size() == 1)
    {
    this->SetAssemblyLabel(0, 0, assembly->label, Qt::white);
    }
  // the new assembly need to be in the grid
}

void cmbNucCore::RemoveAssembly(const std::string &label)
{
  for(size_t i = 0; i < this->Assemblies.size(); i++)
    {
    if(this->Assemblies[i]->label == label)
      {
      delete this->Assemblies[i];
      this->Assemblies.erase(this->Assemblies.begin() + i);
      break;
      }
    }
  // update the Grid
  std::pair<int, int> dim = this->GetDimensions();
  for(size_t i = 0; i < this->Grid.size(); i++)
    {
    for(size_t j = 0; j < this->Grid[i].size(); j++)
      {
      if(this->GetAssemblyLabel(i, j).label == label)
        {
        this->ClearAssemblyLabel(i, j);
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

cmbNucAssembly* cmbNucCore::GetAssembly(int idx)
{
  return idx<this->Assemblies.size() ? this->Assemblies[idx] : NULL;
}

vtkSmartPointer<vtkMultiBlockDataSet> cmbNucCore::GetData()
{
  if(this->Assemblies.size()==0 || this->Grid.size()==0
    || this->Grid[0].size()==0)
    {
    return NULL;
    }
  // we need at least one duct
  if(Assemblies[0]->AssyDuct.Ducts.size()==0)
    {
    return NULL;
    }

  double startX = this->Assemblies[0]->AssyDuct.Ducts[0]->x;
  double startY = this->Assemblies[0]->AssyDuct.Ducts[0]->y;
  double outerDuctHeight = this->Assemblies[0]->AssyDuct.Ducts[0]->thicknesses.back();
//  double chamberStart = outerDuctHeight - innerDuctHeight;
//  double chamberEnd = innerDuctHeight;


  // setup data
  this->Data->SetNumberOfBlocks(this->Grid.size()*this->Grid[0].size());

  for(size_t i = 0; i < this->Grid.size(); i++)
    {
    const std::vector<LatticeCell> &row = this->Grid[i];

    for(size_t j = 0; j < row.size(); j++)
      {
      const std::string &type = row[j].label;

      if(!type.empty() && type != "xx" && type != "XX")
        {
        cmbNucAssembly* assembly = this->GetAssembly(type);
        vtkSmartPointer<vtkMultiBlockDataSet> assemblyData = assembly->GetData();
        vtkNew<vtkTransform> transform;
        transform->Translate(startX + i * (outerDuctHeight+0.5),
                             startY + j * (outerDuctHeight+0.5),
                             0);
 // transform block by block --- got to have better ways
        vtkNew<vtkMultiBlockDataSet> blockData;
        blockData->SetNumberOfBlocks(assemblyData->GetNumberOfBlocks());
        // move the assembly to the correct position
        for(int idx=0; idx<assemblyData->GetNumberOfBlocks(); idx++)
          {
          // Brutal. I wish the SetDefaultExecutivePrototype had workd :(
          if(vtkDataObject* objBlock = assemblyData->GetBlock(idx))
            {
            if(vtkMultiBlockDataSet* assyPartBlock =
              vtkMultiBlockDataSet::SafeDownCast(objBlock))
              {
              vtkNew<vtkMultiBlockDataSet> assyPartObjs;
              assyPartObjs->SetNumberOfBlocks(assyPartBlock->GetNumberOfBlocks());
              for(int b=0; b<assyPartBlock->GetNumberOfBlocks(); b++)
                {
                vtkDataObject* apBlock = assyPartBlock->GetBlock(b);
                if(vtkMultiBlockDataSet* pinPartBlock =
                  vtkMultiBlockDataSet::SafeDownCast(apBlock)) // pins
                  {
                  vtkNew<vtkMultiBlockDataSet> pinPartObjs;
                  pinPartObjs->SetNumberOfBlocks(pinPartBlock->GetNumberOfBlocks());
                  for(int p=0; p<pinPartBlock->GetNumberOfBlocks(); p++)
                    {
                    vtkNew<vtkTransformFilter> filter;
                    filter->SetTransform(transform.GetPointer());
                    filter->SetInputDataObject(pinPartBlock->GetBlock(p));
                    filter->Update();
                    pinPartObjs->SetBlock(p, filter->GetOutput());
                    }
                  assyPartObjs->SetBlock(b, pinPartObjs.GetPointer());
                  }
                else // ducts
                  {
                  vtkNew<vtkTransformFilter> filter;
                  filter->SetTransform(transform.GetPointer());
                  filter->SetInputDataObject(apBlock);
                  filter->Update();
                  assyPartObjs->SetBlock(b, filter->GetOutput());
                  }
                }
              blockData->SetBlock(idx, assyPartObjs.GetPointer());
              }
            else // pins
              {
              vtkNew<vtkTransformFilter> filter;
              filter->SetTransform(transform.GetPointer());
              filter->SetInputDataObject(assemblyData->GetBlock(idx));
              filter->Update();
              blockData->SetBlock(idx, filter->GetOutput());
              }
            }
          else
            {
            blockData->SetBlock(idx, NULL);
            }
          }
       this->Data->SetBlock(i*this->Grid.size()+j, blockData.GetPointer());

/* // The following (better) method is not working ???, even the pipeline is
   // be default compositepipeline

             vtkNew<vtkTransformFilter> filter;
            filter->SetTransform(transform.GetPointer());
            filter->SetInputDataObject(assemblyData);
            filter->Update();
        this->Data->SetBlock(i*this->Grid.size()+j, filter->GetOutput());
*/
        vtkInformation* info = this->Data->GetMetaData(i*this->Grid.size()+j);
        info->Set(vtkCompositeDataSet::NAME(), assembly->label.c_str());
        }
      else
        {
        this->Data->SetBlock(i*this->Grid.size()+j, NULL);
        }
      }
    }

  return this->Data;
}
