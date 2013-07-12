
#include "cmbNucAssembly.h"

#include <iostream>
#include <algorithm>
#include <fstream>
#include <limits>

#include "cmbNucDuctSource.h"
#include "vtkCmbConeSource.h"

#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkAppendPolyData.h"
#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkPolyDataNormals.h"

cmbNucAssembly::cmbNucAssembly()
{
  this->Data = vtkSmartPointer<vtkMultiBlockDataSet>::New();
}

cmbNucAssembly::~cmbNucAssembly()
{
}

void cmbNucAssembly::AddPinCell(const PinCell &pincell)
{
  this->PinCells.push_back(pincell);
}

void cmbNucAssembly::RemovePinCell(const std::string &label)
{
  for(size_t i = 0; i < this->PinCells.size(); i++)
    {
    if(this->PinCells[i].label == label)
      {
      this->PinCells.erase(this->PinCells.begin() + i);
      break;
      }
    }
}

PinCell* cmbNucAssembly::GetPinCell(const std::string &label)
{
  for(size_t i = 0; i < this->PinCells.size(); i++)
    {
    if(this->PinCells[i].label == label)
      {
      return &this->PinCells[i];
      }
    }

  return 0;
}

void cmbNucAssembly::ReadFile(const std::string &FileName)
{
  std::ifstream input(FileName.c_str());
  if(!input.is_open())
    {
    std::cerr << "failed to open input file" << std::endl;
    return;
    }

  while(!input.eof())
    {
    std::string value;
    input >> value;

    std::transform(value.begin(), value.end(), value.begin(), ::tolower);

    if(input.eof() || value.empty())
      {
      break;
      }
    if(value == "end")
      {
      break;
      }
    else if(value == "geometrytype")
      {
      input >> this->GeometryType;
      }
    else if(value == "materials")
      {
      int count;
      input >> count;

      this->Materials.resize(count);

      for(int i = 0; i < count; i++)
        {
        input >> this->Materials[i].name >> this->Materials[i].label;
        }
      }
    else if(value == "duct")
      {
      Duct duct;
      int materials;

      input >> materials
            >> duct.x
            >> duct.y
            >> duct.z1
            >> duct.z2;

      duct.thicknesses.resize(materials);
      for(int i = 0; i < materials; i++)
        {
        input >> duct.thicknesses[i];
        input >> duct.thicknesses[i];
        }

      duct.materials.resize(materials);
      for(int i = 0; i < materials; i++)
        {
        input >> duct.materials[i];
        }

      this->Ducts.push_back(duct);
      }
    else if(value == "pincells")
      {
      int count = 0;
      input >> count;

      for(int i = 0; i < count; i++)
        {
        PinCell pincell;
        int attribute_count = 0;
        input >> pincell.name;

        if(!pincell.name.empty() && pincell.name[0] == '!')
          {
          // skip comment line
          input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
          i--;
          continue;
          }

        input >> pincell.label >> attribute_count;

        for(int j = 0; j < attribute_count; j++)
          {
          input >> value;
          std::transform(value.begin(), value.end(), value.begin(), ::tolower);

          if(value == "pitch")
            {
            input >> pincell.pitchX
                  >> pincell.pitchY
                  >> pincell.pitchZ;
            }
          else if(value == "cylinder")
            {
            int layers;
            Cylinder cylinder;
            input >> layers
                  >> cylinder.x
                  >> cylinder.y
                  >> cylinder.z1
                  >> cylinder.z2
                  >> cylinder.r
                  >> cylinder.material;
            pincell.cylinders.push_back(cylinder);
            }
          else if(value == "frustum")
            {
            int layers;
            Frustum frustum;
            input >> layers
                  >> frustum.x
                  >> frustum.y
                  >> frustum.z1
                  >> frustum.z2
                  >> frustum.r1
                  >> frustum.r2
                  >> frustum.material;
            pincell.frustums.push_back(frustum);
            }
          }
        this->AddPinCell(pincell);
        }
      }
      else if(value == "assembly")
        {
        size_t x;
        size_t y;
        input >> x >> y;

        this->SetDimensions(x, y);

        for(size_t i = 0; i < x; i++)
          {
          for(size_t j = 0; j < y; j++)
            {
            input >> this->Grid[i][j];
            }
          }
        }

    input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

void cmbNucAssembly::WriteFile(const std::string &FileName)
{
  std::ofstream output(FileName.c_str());
  if(!output.is_open())
    {
    std::cerr << "failed to open output file" << std::endl;
    return;
    }

  output << "MeshType tet\n";
  output << "GeomEngine acis\n";
  output << "GeometryType rectangular\n";

  // materials
  output << "Materials " << this->Materials.size();
  for(size_t i = 0; i < this->Materials.size(); i++)
    {
    output << " " << this->Materials[i].name << " " << this->Materials[i].label;
    }
  output << "\n";

  // ducts
  for(size_t i = 0; i < this->Ducts.size(); i++)
    {
    const Duct &duct = this->Ducts[i];

    output << "duct " << duct.materials.size() << " ";
    output << duct.x << " " << duct.y << " " << duct.z1 << " " << duct.z2;

    for(size_t j = 0; j < duct.thicknesses.size(); j++)
      {
      output << " " << duct.thicknesses[i] << " " << duct.thicknesses[i];
      }

    for(size_t j = 0; j < duct.materials.size(); j++)
      {
      output << " " << duct.materials[i];
      }
    output << "\n";
    }

  // pincells
  output << "pincells " << this->PinCells.size() << "\n";

  for(size_t i = 0; i < this->PinCells.size(); i++)
    {
    const PinCell &pincell = this->PinCells[i];

    // count of attribute lines for the pincell. equal to the number
    // of frustums plus cylinders plus one for the pitch
    size_t count = pincell.cylinders.size() + pincell.frustums.size() + 1;

    output << pincell.name << " " << pincell.label << " " << count << "\n";

    output << "pitch " << pincell.pitchX << " " << pincell.pitchY << " " << pincell.pitchZ << "\n";

    for(size_t j = 0; j < pincell.cylinders.size(); j++)
      {
      output << "cylinder" << "\n";
      }

    for(size_t j = 0; j < pincell.frustums.size(); j++)
      {
      output << "frustum" << "\n";
      }
    }

  // assembly
  output << "assembly " << this->Grid.size() << " " << this->Grid[0].size() << "\n";
  for(size_t i = 0; i < this->Grid.size(); i++)
    {
    for(size_t j = 0; j < this->Grid[i].size(); j++)
      {
      output << this->Grid[i][j] << " ";
      }
    output << "\n";
    }

  // other parameters
  output << "tetmeshsize 2.0\n";
  output << "createsideset no\n";
  output << "MaterialSet_StartId 1\n";
  output << "mergetolerance 1e-6\n";
  output << "info on\n";

  // end
  output << "end\n";
}

vtkSmartPointer<vtkMultiBlockDataSet> cmbNucAssembly::GetData()
{
  double outerDuctHeight = this->Ducts[0].thicknesses.back();
  double innerDuctHeight = this->Ducts[0].thicknesses.front();
  double chamberStart = outerDuctHeight - innerDuctHeight;
  double chamberEnd = innerDuctHeight;

  // setup data
  this->Data->SetNumberOfBlocks(this->Grid.size() * this->Grid[0].size() + 1);

  double cellLength = (chamberEnd - chamberStart) / this->Grid.size();

  for(size_t i = 0; i < this->Grid.size(); i++)
    {
    const std::vector<std::string> &row = this->Grid[i];

    for(size_t j = 0; j < row.size(); j++)
      {
      const std::string &type = row[j];

      if(type != "xx" && type != "XX")
        {
        const PinCell &pincell = this->PinCells[0];

        // create polydata for the pincell
        vtkPolyData *polyData = this->CreatePinCellPolyData(pincell);

        // move the polydata to the correct position
        vtkTransform *transform = vtkTransform::New();
        transform->Translate(chamberStart + i * cellLength + pincell.cylinders[0].r,
                             chamberStart + j * cellLength + pincell.cylinders[0].r,
                             0);
        vtkTransformPolyDataFilter *filter = vtkTransformPolyDataFilter::New();
        filter->SetTransform(transform);
        transform->Delete();
        filter->SetInputDataObject(polyData);
        polyData->Delete();
        filter->Update();
        this->Data->SetBlock(i*this->Grid.size()+j, filter->GetOutput());
        filter->Delete();
        }
      else
        {
        this->Data->SetBlock(i*this->Grid.size()+j, NULL);
        }
      }
    }

  // setup ducts
  for(size_t i = 0; i < this->Ducts.size(); i++)
    {
    const Duct &duct = this->Ducts[i];

    cmbNucDuctSource *ductSource = cmbNucDuctSource::New();
    ductSource->SetOrigin(0, 0, 0);
    ductSource->SetHeight(4.0);

    for(size_t i = 0; i < duct.thicknesses.size(); i++)
      {
      ductSource->AddLayer(duct.thicknesses[i], duct.thicknesses[i]);
      }

    ductSource->Update();

    this->Data->SetBlock(this->Data->GetNumberOfBlocks() - 1, ductSource->GetOutput());
    ductSource->Delete();
    }

  return this->Data;
}

void cmbNucAssembly::SetDimensions(int i, int j)
{
  this->Grid.resize(i);

  for(int k = 0; k < i; k++)
    {
    this->Grid[k].resize(j);
    }
}

std::pair<int, int> cmbNucAssembly::GetDimensions() const
{
  return std::make_pair(this->Grid.size(), this->Grid[0].size());
}

void cmbNucAssembly::SetCell(int i, int j, const std::string &name)
{
  this->Grid[i][j] = name;
}

std::string cmbNucAssembly::GetCell(int i, int j) const
{
  return this->Grid[i][j];
}

void cmbNucAssembly::ClearCell(int i, int j)
{
  this->SetCell(i, j, "xx");
}

vtkPolyData* cmbNucAssembly::CreatePinCellPolyData(const PinCell &pincell)
{
  vtkAppendPolyData *merger = vtkAppendPolyData::New();

  const int PinCellResolution = 16;

  for(size_t j = 0; j < pincell.cylinders.size(); j++)
    {
    const Cylinder &cylinder = pincell.cylinders[j];

    vtkCmbConeSource *cylinderSource = vtkCmbConeSource::New();
    cylinderSource->SetBaseCenter(0, 0, cylinder.z1);
    cylinderSource->SetHeight(cylinder.z2 - cylinder.z1);
    cylinderSource->SetBaseRadius(cylinder.r);
    cylinderSource->SetTopRadius(cylinder.r);
    cylinderSource->SetResolution(PinCellResolution);
    double direction[] = { 0, 0, 1 };
    cylinderSource->SetDirection(direction);
    cylinderSource->Update();
    merger->AddInputData(cylinderSource->GetOutput());
    cylinderSource->Delete();
    }

  for(size_t j = 0; j < pincell.frustums.size(); j++)
    {
    const Frustum &frustum = pincell.frustums[j];

    vtkCmbConeSource *coneSource = vtkCmbConeSource::New();
    coneSource->SetBaseCenter(0, 0, frustum.z1);
    coneSource->SetHeight(frustum.z2 - frustum.z1);
    coneSource->SetBaseRadius(frustum.r1);
    coneSource->SetTopRadius(frustum.r2);
    coneSource->SetResolution(PinCellResolution);
    double direction[] = { 0, 0, 1 };
    coneSource->SetDirection(direction);
    coneSource->Update();

    merger->AddInputData(coneSource->GetOutput());
    coneSource->Delete();
    }

  vtkPolyDataNormals *normals = vtkPolyDataNormals::New();
  normals->SetInputConnection(merger->GetOutputPort());
  merger->Delete();
  normals->Update();

  vtkPolyData *polyData = vtkPolyData::New();
  polyData->DeepCopy(normals->GetOutput());
  normals->Delete();
  return polyData;
}
