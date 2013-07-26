
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
      delete this->Assemblies[i];
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
      if(this->GetAssemblyLabel(i, j) == label)
        {
        this->SetAssemblyLabel(i, j, "xx");
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

  double startX = this->Assemblies[0]->AssyDuct.Ducts[0]->x;
  double startY = this->Assemblies[0]->AssyDuct.Ducts[0]->y;
  double outerDuctHeight = this->Assemblies[0]->AssyDuct.Ducts[0]->thicknesses.back();
//  double chamberStart = outerDuctHeight - innerDuctHeight;
//  double chamberEnd = innerDuctHeight;


  // setup data
  this->Data->SetNumberOfBlocks(this->Grid.size()*this->Grid[0].size());

  for(size_t i = 0; i < this->Grid.size(); i++)
    {
    const std::vector<std::string> &row = this->Grid[i];

    for(size_t j = 0; j < row.size(); j++)
      {
      const std::string &type = row[j];

      if(!type.empty() && type != "xx" && type != "XX")
        {
        cmbNucAssembly* assembly = this->GetAssembly(type);
        vtkMultiBlockDataSet* assemblyData = assembly->GetData();
        vtkNew<vtkTransform> transform;
        transform->Translate(startX + i * outerDuctHeight,
                             startY + j * outerDuctHeight,
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
            if(vtkMultiBlockDataSet* ductBlock =
              vtkMultiBlockDataSet::SafeDownCast(objBlock))
              {
              vtkNew<vtkMultiBlockDataSet> ductObjs;
              ductObjs->SetNumberOfBlocks(ductBlock->GetNumberOfBlocks());
              for(int b=0; b<ductBlock->GetNumberOfBlocks(); b++)
                {
                vtkNew<vtkTransformFilter> filter;
                filter->SetTransform(transform.GetPointer());
                filter->SetInputDataObject(ductBlock->GetBlock(b));
                filter->Update();
                ductObjs->SetBlock(idx, filter->GetOutput());
                }
              blockData->SetBlock(idx, ductObjs.GetPointer());
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
