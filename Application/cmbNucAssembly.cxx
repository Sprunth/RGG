
#include "cmbNucAssembly.h"

#include <iostream>
#include <algorithm>
#include <fstream>
#include <limits>

#include "cmbNucDuctSource.h"
#include "cmbNucMaterialColors.h"
#include "vtkCmbConeSource.h"
#include "vtkCmbLayeredConeSource.h"

#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkAppendPolyData.h"
#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkPolyDataNormals.h"
#include "vtkNew.h"

#include <QMap>

cmbNucAssembly::cmbNucAssembly()
{
  this->Data = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  this->MeshSize = 2.0;
}

cmbNucAssembly::~cmbNucAssembly()
{
  AssyPartObj::deleteObjs(this->PinCells);
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

void cmbNucAssembly::RemoveMaterial(const std::string &name)
{
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
    pincell->RemoveMaterial(name);
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

    if(input.eof())
      {
      break;
      }
    else if(value == "end")
      {
      break;
      }
    else if(value.empty())
      {
      input.clear();
      continue;
      }
    else if(value == "geometrytype")
      {
      input >> this->GeometryType;
      }
    else if(value == "materials")
      {
      int count;
      input >> count;

      cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
      for(int i = 0; i < count; i++)
        {
        std::string mname, mlabel;
        input >> mname >> mlabel;
        if(!matColorMap->MaterialColorMap().contains(mname.c_str()))
          {
          matColorMap->AddMaterial(mname.c_str(), mlabel.c_str(),
            1.0, 1.0, 1.0, 1.0);
          }
        else
          {
          // replace the label
          const QColor &color = matColorMap->MaterialColorMap()[mname.c_str()].second;
          matColorMap->AddMaterial(mname.c_str(), mlabel.c_str(), color);
          }
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
            int layers;
            input >> layers;
            Cylinder* cylinder = new Cylinder();
            if(layers > pincell->GetNumberOfLayers())
              {
              cylinder->materials.resize(layers);
              pincell->radii.resize(layers);
              }

            input >> cylinder->x
                  >> cylinder->y
                  >> cylinder->z1
                  >> cylinder->z2;
            for(int c=0; c < layers; c++)
              {
              input >> pincell->radii[c];
              }
            for(int c=0; c < layers; c++)
              {
              input >> cylinder->materials[c];
              }
            pincell->cylinders.push_back(cylinder);
            }
          else if(value == "frustum")
            {
            int layers;
            input >> layers;
            Frustum* frustum = new Frustum();
            if(layers > pincell->GetNumberOfLayers())
              {
              frustum->materials.resize(layers);
              pincell->radii.resize(layers);
              }

            input >> frustum->x
                >> frustum->y
                >> frustum->z1
                >> frustum->z2;

            for(int c=0; c < layers; c++)
              {
              input >> pincell->radii[c];
              }
            for(int c=0; c < layers; c++)
              {
              input >> frustum->materials[c];
              }
            pincell->frustums.push_back(frustum);
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
        input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        this->AssyLattice.SetDimensions(x, y);

        if(input.peek() == '!')
          {
          input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
          }

        for(size_t i = 0; i < x; i++)
          {
          for(size_t j = 0; j < y; j++)
            {
            input >> this->AssyLattice.Grid[i][j];
            }
            input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
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
  QMap<std::string, std::string> materials;
  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  matColorMap->GetAssemblyMaterials(this, materials);
  output << "Materials " << materials.count();
  foreach(std::string name, materials.keys())
    {
    output << " " << name << " " << materials[name];
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
        << cylinder->r << " ";
      for(int material = 0; material < cylinder->materials.size(); material++)
        {
        output << cylinder->materials[material] << " ";
        }
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
        << frustum->r2 << " ";
      for(int material = 0; material < frustum->materials.size(); material++)
        {
        output << frustum->materials[material] << " ";
        }
      if(j==pincell->frustums.size()-1) output << "\n";
      }
    }

  // assembly
  output << "assembly " << this->AssyLattice.Grid.size() << " " << this->AssyLattice.Grid[0].size() << "\n";
  for(size_t i = 0; i < this->AssyLattice.Grid.size(); i++)
    {
    for(size_t j = 0; j < this->AssyLattice.Grid[i].size(); j++)
      {
      std::string label = this->AssyLattice.Grid[i][j];
      if(label.empty())
        {
        label = "xx";
        }
      output << label << " ";
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
        PinCell* pincell = this->GetPinCell(type);
        if(pincell && (pincell->cylinders.size()+pincell->frustums.size())>0)
          {
          // create polydata for the pincell
          vtkMultiBlockDataSet *dataSet = this->CreatePinCellMultiBlock(pincell);
          vtkNew<vtkMultiBlockDataSet> pinDataSet;
          pinDataSet->SetNumberOfBlocks(dataSet->GetNumberOfBlocks());
          for(int block=0; block<dataSet->GetNumberOfBlocks(); block++)
            { 
            vtkMultiBlockDataSet* sectionBlock =
              vtkMultiBlockDataSet::SafeDownCast(dataSet->GetBlock(block));
            if(!sectionBlock)
              {
              continue;
              }
            vtkNew<vtkMultiBlockDataSet> transdataSet;
            transdataSet->SetNumberOfBlocks(sectionBlock->GetNumberOfBlocks());
            for(int layer=0; layer<sectionBlock->GetNumberOfBlocks(); layer++)
              {
              vtkPolyData* polyBlock =
                vtkPolyData::SafeDownCast(sectionBlock->GetBlock(layer));
              if(!polyBlock)
                {
                continue;
                }

              // move the polydata to the correct position
              vtkTransform *transform = vtkTransform::New();
              double radius = pincell->cylinders.size()>0 ? pincell->cylinders[0]->r :
                pincell->frustums[0]->r1;
              transform->Translate(chamberStart + i * cellLength + radius,
                                   chamberStart + j * cellLength + radius,
                                   0);
              vtkNew<vtkTransformFilter> filter;
              filter->SetTransform(transform);
              transform->Delete();
              filter->SetInputDataObject(polyBlock);
              filter->Update();
              transdataSet->SetBlock(layer, filter->GetOutput());
              }
            pinDataSet->SetBlock(block, transdataSet.GetPointer());
            }
          dataSet->Delete();
          this->Data->SetBlock(i*this->AssyLattice.Grid.size()+j, pinDataSet.GetPointer());
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

vtkMultiBlockDataSet* cmbNucAssembly::CreatePinCellMultiBlock(PinCell* pincell)
{
  if(pincell->cylinders.size() + pincell->frustums.size() == 0)
    {
    return vtkMultiBlockDataSet::New();
    }

  // There are two child multibock, one for cylinders, one for frustums
  vtkMultiBlockDataSet *dataSet = vtkMultiBlockDataSet::New();
  dataSet->SetNumberOfBlocks(pincell->cylinders.size() + pincell->frustums.size());

  // build all cylinders and frustums
  const int PinCellResolution = 16;
  size_t numCyls = pincell->cylinders.size();
  for(size_t j = 0; j < numCyls; j++)
    {
    Cylinder *cylinder = pincell->cylinders[j];

    vtkSmartPointer<vtkCmbLayeredConeSource> coneSource =
      vtkSmartPointer<vtkCmbLayeredConeSource>::New();
    coneSource->SetNumberOfLayers(pincell->GetNumberOfLayers());
    coneSource->SetBaseCenter(0, 0, cylinder->z1);
    coneSource->SetHeight(cylinder->z2 - cylinder->z1);

    for(int k = 0; k < pincell->GetNumberOfLayers(); k++)
      {
      coneSource->SetBaseRadius(k, pincell->radii[k] * cylinder->r);
      coneSource->SetTopRadius(k, pincell->radii[k] * cylinder->r);
      }
    coneSource->SetResolution(PinCellResolution);
    double direction[] = { 0, 0, 1 };
    coneSource->SetDirection(direction);
    coneSource->Update();
    dataSet->SetBlock(j, coneSource->GetOutput());
    }

  for(size_t j = 0; j < pincell->frustums.size(); j++)
    {
    Frustum* frustum = pincell->frustums[j];

    vtkSmartPointer<vtkCmbLayeredConeSource> coneSource =
      vtkSmartPointer<vtkCmbLayeredConeSource>::New();
    coneSource->SetNumberOfLayers(pincell->GetNumberOfLayers());
    coneSource->SetBaseCenter(0, 0, frustum->z1);
    coneSource->SetHeight(frustum->z2 - frustum->z1);

    for(int k = 0; k < pincell->GetNumberOfLayers(); k++)
      {
      coneSource->SetBaseRadius(k, pincell->radii[k] * frustum->r1);
      coneSource->SetTopRadius(k, pincell->radii[k] * frustum->r2);
      }
    coneSource->SetResolution(PinCellResolution);
    double direction[] = { 0, 0, 1 };
    coneSource->SetDirection(direction);
    coneSource->Update();
    dataSet->SetBlock(numCyls+j, coneSource->GetOutput());
    }

/*
  // Now append each layer's polydata together to form a multiblock with
  // appended layers as blocks. ASSUMING all sections have same layers.
  for(int layer=0; layer<pincell->GetNumberOfLayers; layer++)
    {
    vtkAppendPolyData *merger = vtkAppendPolyData::New();
    for(size_t j = 0; j <cylinderSrcs.size(); j++)
      {
      vtkMultiBlockDataSet *mbds = cylinderSrcs[j]->GetOutput();
      merger->AddInputData(vtkPolyData::SafeDownCast(mbds->GetBlock(layer)));
      }
    for(int k = 0; k < frustumSrcs.size(); k++)
      {
      vtkMultiBlockDataSet *mbds = frustumSrcs[k]->GetOutput();
      merger->AddInputData(vtkPolyData::SafeDownCast(mbds->GetBlock(layer)));
      }
 
    merger->Update();
    vtkPolyDataNormals *normals = vtkPolyDataNormals::New();
    normals->SetInputConnection(merger->GetOutputPort());
    merger->Delete();
    normals->Update();

    vtkPolyData *polyData = vtkPolyData::New();
    polyData->DeepCopy(normals->GetOutput());
    normals->Delete();
    polyData->Delete();
    }
*/
  return dataSet;
}
