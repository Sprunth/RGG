
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
  this->MeshSize = 2.0;
}

cmbNucAssembly::~cmbNucAssembly()
{
  AssyPartObj::deleteObjs(this->PinCells);
  AssyPartObj::deleteObjs(this->Materials);
}

void cmbNucAssembly::AddPinCell(PinCell *pincell)
{
  this->PinCells.push_back(pincell);
}

void cmbNucAssembly::RemovePinCell(const std::string &label)
{
  for(size_t i = 0; i < this->PinCells.size(); i++)
    {
    if(this->PinCells[i]->label == label)
      {
      delete this->PinCells[i];
      this->PinCells.erase(this->PinCells.begin() + i);
      break;
      }
    }
  // update the Grid
  std::pair<int, int> dim = this->AssyLattice.GetDimensions();
  for(size_t i = 0; i < dim.first; i++)
    {
    for(size_t j = 0; j < dim.second; j++)
      {
      if(this->AssyLattice.GetCell(i, j) == label)
        {
        this->AssyLattice.SetCell(i, j, "xx");
        }
      }
    }
}

void cmbNucAssembly::AddMaterial(Material *material)
{
  this->Materials.push_back(material);
}

void cmbNucAssembly::RemoveMaterial(const std::string &name)
{
  for(size_t i = 0; i < this->Materials.size(); i++)
    {
    if(this->Materials[i]->name == name)
      {
      delete this->Materials[i];
      this->Materials.erase(this->Materials.begin() + i);
      break;
      }
    }
  // update all places that references materials: ducts, pins
   for(size_t i = 0; i < this->AssyDuct.Ducts.size(); i++)
    {
    Duct *duct = this->AssyDuct.Ducts[i];
    for(size_t j = 0; j < duct->materials.size(); j++)
      {
      if(duct->materials[j] == name)
        {
        duct->materials[j] = "";
        }
     }
    }
  for(size_t i = 0; i < this->PinCells.size(); i++)
    {
    PinCell* pincell = this->PinCells[i];
    for(size_t j = 0; j < pincell->cylinders.size(); j++)
      {
      Cylinder* cylinder = pincell->cylinders[j];
      if(cylinder->material == name)
        {
        cylinder->material = "";
        }
      }
    for(size_t j = 0; j < pincell->frustums.size(); j++)
      {
      Frustum* frustum = pincell->frustums[j];
      if(frustum->material == name)
        {
        frustum->material = "";
        }
      }
    }

}

PinCell* cmbNucAssembly::GetPinCell(const std::string &label)
{
  for(size_t i = 0; i < this->PinCells.size(); i++)
    {
    if(this->PinCells[i]->label == label)
      {
      return this->PinCells[i];
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

      this->Materials.clear();

      for(int i = 0; i < count; i++)
        {
        Material* material = new Material();
        input >> material->name >> material->label;
        this->Materials.push_back(material);
        }
      }
    else if(value == "duct")
      {
      Duct* duct = new Duct();
      int materials;

      input >> materials
            >> duct->x
            >> duct->y
            >> duct->z1
            >> duct->z2;

      duct->thicknesses.resize(materials*2);
      for(int i = 0; i < materials*2; i++)
        {
        input >> duct->thicknesses[i];
        }

      duct->materials.resize(materials);
      for(int i = 0; i < materials; i++)
        {
        input >> duct->materials[i];
        }

      this->AssyDuct.Ducts.push_back(duct);
      }
    else if(value == "pincells")
      {
      int count = 0;
      input >> count;

      for(int i = 0; i < count; i++)
        {
        PinCell* pincell = new PinCell();
        int attribute_count = 0;
        input >> pincell->name;

        if(!pincell->name.empty() && pincell->name[0] == '!')
          {
          // skip comment line
          input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
          i--;
          continue;
          }

        input >> pincell->label >> attribute_count;
 
        for(int j = 0; j < attribute_count; j++)
          {
          input >> value;
          std::transform(value.begin(), value.end(), value.begin(), ::tolower);

          if(value == "pitch")
            {
            input >> pincell->pitchX
                  >> pincell->pitchY
                  >> pincell->pitchZ;
            //j--;  // Pitch does not count towards count!
            }
          else if(value == "cylinder")
            {
            int numCyliner;
            input >> numCyliner;
            for(int c=0; c<numCyliner; c++)
              {
              Cylinder* cylinder = new Cylinder();
              input >> cylinder->x
                  >> cylinder->y
                  >> cylinder->z1
                  >> cylinder->z2
                  >> cylinder->r
                  >> cylinder->material;
                pincell->cylinders.push_back(cylinder);
              }
            }
          else if(value == "frustum")
            {
            int numFrustum;
            input >> numFrustum;
            for(int f=0; f<numFrustum; f++)
              {
              Frustum* frustum = new Frustum();
              input >> frustum->x
                  >> frustum->y
                  >> frustum->z1
                  >> frustum->z2
                  >> frustum->r1
                  >> frustum->r2
                  >> frustum->material;
              pincell->frustums.push_back(frustum);
              }
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

        this->AssyLattice.SetDimensions(x, y);

        for(size_t i = 0; i < x; i++)
          {
          for(size_t j = 0; j < y; j++)
            {
            input >> this->AssyLattice.Grid[i][j];
            }
          }
        }
      else if(value == "tetmeshsize")
        {
        input >> this->MeshSize;
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
    output << " " << this->Materials[i]->name << " " << this->Materials[i]->label;
    }
  output << "\n";

  // ducts
  for(size_t i = 0; i < this->AssyDuct.Ducts.size(); i++)
    {
    Duct *duct = this->AssyDuct.Ducts[i];

    output << "duct " << duct->materials.size() << " ";
    output << duct->x << " " << duct->y << " " << duct->z1 << " " << duct->z2;

    for(size_t j = 0; j < duct->thicknesses.size(); j++)
      {
      output << " " << duct->thicknesses[j];
      }

    for(size_t j = 0; j < duct->materials.size(); j++)
      {
      output << " " << duct->materials[j];
      }
    output << "\n";
    }

  // pincells
  output << "pincells " << this->PinCells.size() << "\n";

  for(size_t i = 0; i < this->PinCells.size(); i++)
    {
    PinCell* pincell = this->PinCells[i];

    // count of attribute lines for the pincell. equal to the number
    // of frustums plus cylinders plus one for the pitch.
    // We are writing multiple cylinders/frustums on one line.
    size_t count = (pincell->cylinders.size()>0 ? 1: 0) +
                   (pincell->frustums.size()>0 ? 1 : 0) + 1;

    output << pincell->name << " " << pincell->label << " " << count << "\n";

    output << "pitch " << pincell->pitchX << " " << pincell->pitchY << " " << pincell->pitchZ << "\n";

    for(size_t j = 0; j < pincell->cylinders.size(); j++)
      {
      if(j==0) output << "cylinder " << pincell->cylinders.size() << " ";
      Cylinder* cylinder = pincell->cylinders[j];
      output << cylinder->x << " "
        << cylinder->y << " "
        << cylinder->z1 << " "
        << cylinder->z2 << " "
        << cylinder->r << " "
        << cylinder->material << " ";
      if(j==pincell->cylinders.size()-1) output << "\n";
      }

    for(size_t j = 0; j < pincell->frustums.size(); j++)
      {
      if(j==0) output << "frustum " << pincell->frustums.size() << " ";
      Frustum* frustum = pincell->frustums[j];
      output << frustum->x << " "
        << frustum->y << " "
        << frustum->z1 << " "
        << frustum->z2 << " "
        << frustum->r1 << " "
        << frustum->r2 << " "
        << frustum->material << " ";
      if(j==pincell->frustums.size()-1) output << "\n";
      }
    }

  // assembly
  output << "assembly " << this->AssyLattice.Grid.size() << " " << this->AssyLattice.Grid[0].size() << "\n";
  for(size_t i = 0; i < this->AssyLattice.Grid.size(); i++)
    {
    for(size_t j = 0; j < this->AssyLattice.Grid[i].size(); j++)
      {
      output << this->AssyLattice.Grid[i][j] << " ";
      }
    output << "\n";
    }

  output << "tetmeshsize " << this->MeshSize << "\n";

  // other parameters
  //output << "tetmeshsize 2.0\n";
  output << "createsideset no\n";
  output << "MaterialSet_StartId 1\n";
  output << "mergetolerance 1e-6\n";
  output << "info on\n";

  // end
  output << "end\n";
}

vtkSmartPointer<vtkMultiBlockDataSet> cmbNucAssembly::GetData()
{
  if(this->AssyDuct.Ducts.size()==0)
    {
    return NULL;
    }
  double outerDuctHeight = this->AssyDuct.Ducts[0]->thicknesses.back();
  double innerDuctHeight = this->AssyDuct.Ducts[0]->thicknesses.front();
  double chamberStart = outerDuctHeight - innerDuctHeight;
  double chamberEnd = innerDuctHeight;

  // setup data
  this->Data->SetNumberOfBlocks(this->AssyLattice.Grid.size() * this->AssyLattice.Grid[0].size() +
                                this->AssyDuct.Ducts.size());

  double cellLength = (chamberEnd - chamberStart) / this->AssyLattice.Grid.size();

  for(size_t i = 0; i < this->AssyLattice.Grid.size(); i++)
    {
    const std::vector<std::string> &row = this->AssyLattice.Grid[i];

    for(size_t j = 0; j < row.size(); j++)
      {
      const std::string &type = row[j];

      if(!type.empty() && type != "xx" && type != "XX")
        {
        PinCell* pincell = this->GetPinCell(type);//this->PinCells[0];
        if(pincell && (pincell->cylinders.size()+pincell->frustums.size())>0)
          {
          // create polydata for the pincell
          vtkPolyData *polyData = this->CreatePinCellPolyData(pincell);

          // move the polydata to the correct position
          vtkTransform *transform = vtkTransform::New();
          double radius = pincell->cylinders.size()>0 ? pincell->cylinders[0]->r :
            pincell->frustums[0]->r1;
          transform->Translate(chamberStart + i * cellLength + radius,
                               chamberStart + j * cellLength + radius,
                               0);
          vtkTransformPolyDataFilter *filter = vtkTransformPolyDataFilter::New();
          filter->SetTransform(transform);
          transform->Delete();
          filter->SetInputDataObject(polyData);
          polyData->Delete();
          filter->Update();
          this->Data->SetBlock(i*this->AssyLattice.Grid.size()+j, filter->GetOutput());
          filter->Delete();
          }
        else
          {
          this->Data->SetBlock(i*this->AssyLattice.Grid.size()+j, NULL);
          }
        }
      else
        {
        this->Data->SetBlock(i*this->AssyLattice.Grid.size()+j, NULL);
        }
      }
    }

  // setup ducts
  for(size_t i = 0; i < this->AssyDuct.Ducts.size(); i++)
    {
    Duct *duct = this->AssyDuct.Ducts[i];

    cmbNucDuctSource *ductSource = cmbNucDuctSource::New();
    ductSource->SetOrigin(duct->x, duct->y, duct->z1);
    ductSource->SetHeight(duct->z2 - duct->z1);

    for(size_t j = 0; j < duct->thicknesses.size()/2; j++)
      {
      ductSource->AddLayer(duct->thicknesses[2*j], duct->thicknesses[2*j+1]);
      }

    ductSource->Update();

    this->Data->SetBlock(this->Data->GetNumberOfBlocks() - i - 1, ductSource->GetOutput());
    ductSource->Delete();
    }

  return this->Data;
}

vtkPolyData* cmbNucAssembly::CreatePinCellPolyData(PinCell* pincell)
{
  vtkAppendPolyData *merger = vtkAppendPolyData::New();

  const int PinCellResolution = 16;

  for(size_t j = 0; j < pincell->cylinders.size(); j++)
    {
    Cylinder *cylinder = pincell->cylinders[j];

    vtkCmbConeSource *cylinderSource = vtkCmbConeSource::New();
    cylinderSource->SetBaseCenter(0, 0, cylinder->z1);
    cylinderSource->SetHeight(cylinder->z2 - cylinder->z1);
    cylinderSource->SetBaseRadius(cylinder->r);
    cylinderSource->SetTopRadius(cylinder->r);
    cylinderSource->SetResolution(PinCellResolution);
    double direction[] = { 0, 0, 1 };
    cylinderSource->SetDirection(direction);
    cylinderSource->Update();
    merger->AddInputData(cylinderSource->GetOutput());
    cylinderSource->Delete();
    }

  for(size_t j = 0; j < pincell->frustums.size(); j++)
    {
    Frustum* frustum = pincell->frustums[j];

    vtkCmbConeSource *coneSource = vtkCmbConeSource::New();
    coneSource->SetBaseCenter(0, 0, frustum->z1);
    coneSource->SetHeight(frustum->z2 - frustum->z1);
    coneSource->SetBaseRadius(frustum->r1);
    coneSource->SetTopRadius(frustum->r2);
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

std::string cmbNucAssembly::GetCellMaterial(int i)
{
// First find the cell
    std::string cellType = this->AssyLattice.GetCell(i);
    std::string result;
    // If the cell is xx then return ""
    if (cellType == "xx")
        {
        return result;
        }
    // Get the pin
    PinCell *pin = this->GetPinCell(cellType);
    if (pin)
        {
        return pin->GetMaterial();
        }
    return result;
}
