
#include "cmbNucAssembly.h"

#include <iostream>
#include <algorithm>
#include <fstream>
#include <limits>

#include "cmbNucDuctSource.h"
#include "cmbNucMaterialColors.h"
#include "vtkCmbConeSource.h"
#include "vtkCmbLayeredConeSource.h"
#include <vtkClipClosedSurface.h>
#include <vtkPlaneCollection.h>

#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkAppendPolyData.h"
#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkPolyDataNormals.h"
#include "vtkNew.h"
#include "vtkMath.h"

#include "vtkXMLMultiBlockDataWriter.h"
#include <QMap>

cmbNucAssembly::cmbNucAssembly()
{
  this->Data = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  this->MeshSize = 2.0;

  this->RadialMeshSize = 0.15;
  this->AxialMeshSize = 20.0;
  this->RotateDirection = "Z";
  this->RotateAngle = -30;

}

cmbNucAssembly::~cmbNucAssembly()
{
  AssyPartObj::deleteObjs(this->PinCells);
}

void cmbNucAssembly::UpdateGrid()
{
  std::pair<int, int> dim = this->AssyLattice.GetDimensions();
  for(size_t i = 0; i < dim.first; i++)
    {
    size_t layerCells = this->AssyLattice.GetGeometryType() == HEXAGONAL ?
      6*i : dim.second;
    for(size_t j = 0; j < layerCells; j++)
      {
      std::string label = this->AssyLattice.GetCell(i, j).label;
      PinCell* pc = this->GetPinCell(label);
      if(pc)
        {
        this->AssyLattice.SetCell(i, j, label, pc->GetLegendColor());
        }
      else
        {
        this->AssyLattice.ClearCell(i, j);
        }
      }
    }
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
  this->AssyLattice.ClearCell(label);
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

bool cmbNucAssembly::IsHexType()
{
  std::string strGeoType = this->GeometryType;
  std::transform(this->GeometryType.begin(), this->GeometryType.end(),
    strGeoType.begin(), ::tolower);
  return strGeoType == "hexagonal";
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
      if(this->IsHexType())
        {
        this->AssyLattice.SetGeometryType(HEXAGONAL);
        }
      else
        {
        this->AssyLattice.SetGeometryType(RECTILINEAR);
        }
      this->AssyLattice.SetDimensions(0, 0);
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
        std::transform(mname.begin(), mname.end(), mname.begin(), ::tolower);
        if(!matColorMap->MaterialColorMap().contains(mname.c_str()))
          {
          matColorMap->AddMaterial(mname.c_str(), mlabel.c_str(),
            1.0, 1.0, 1.0, 1.0);
          }
        else
          {
          // replace the label
          const QColor &color = matColorMap->MaterialColorMap()[mname.c_str()].Color;
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
      if(this->IsHexType())
        {
        duct->SetType(CMBNUC_ASSY_HEX_DUCT);
        for(int i = 0; i < materials; i++)
          {
          input >> duct->thicknesses[i*2];
          duct->thicknesses[i*2+1] = duct->thicknesses[i*2];
          }
        }
      else
        {
        for(int i = 0; i < materials*2; i++)
          {
          input >> duct->thicknesses[i];
          }
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
      // for Hex type, the pitch is next input.
      // TODO: Need to handle case that there is no "pitch" here for HEX.
      double hexPicth = -1.0;
      if(this->IsHexType())
        {
        input >> hexPicth;
        }

      for(int i = 0; i < count; i++)
        {
        int lc = i % 10; // Pick a default pin color (for now!)
        PinCell* pincell = new PinCell();
        std::string firstMaterial;
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

        // initialize for HEX pincell pitch
        if(hexPicth >= 0.0)
          {
          pincell->pitchX=pincell->pitchY=pincell->pitchZ = hexPicth;
          }

        for(int j = 0; j < attribute_count; j++)
          {
          input >> value;
          std::transform(value.begin(), value.end(), value.begin(), ::tolower);

          if(value == "pitch")
            {
            // only one field for HEX type
            if(this->IsHexType())
              {
              double dHexPinPitch;
              input >> dHexPinPitch;
              pincell->pitchX=pincell->pitchY=pincell->pitchZ = dHexPinPitch;
              }
            else
              {
              input >> pincell->pitchX
                >> pincell->pitchY
                >> pincell->pitchZ;
              }
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

            cylinder->r = pincell->radii.back();
            pincell->cylinders.push_back(cylinder);

            // let alpha be the normalization factor for the layers (outer most would be 1.0)
            double alpha = 1.0 / cylinder->r;
            for(int c=0; c < layers; c++)
              {
              // Normalize the layer
              pincell->radii[c] *= alpha;

              // Get the material of the layer
              std::string mname;
              input >> mname;
              std::transform(mname.begin(), mname.end(), mname.begin(), ::tolower);
              // Lets save the first material to use to set the pin's color legend
              if (firstMaterial == "")
                {
                firstMaterial = mname;
                }
              cylinder->materials[c] = mname;
              }
            }
          else if(value == "frustum")
            {
            int layers;
            input >> layers;
            Frustum* frustum = new Frustum();
            if(layers > pincell->GetNumberOfLayers())
              {
              frustum->materials.resize(layers);
              pincell->radii.resize(layers*2);
              }

            input >> frustum->x
                >> frustum->y
                >> frustum->z1
                >> frustum->z2;

            for(int c=0; c < layers; c++)
              {
              input >> pincell->radii[c*2+0];
              input >> pincell->radii[c*2+1];
              }

            frustum->r1 = pincell->radii[(2*layers)-1];
            frustum->r2 = pincell->radii[(2*layers)-2];
            pincell->frustums.push_back(frustum);

            // let alpha be the normalization factor for the layers (outer most would be 1.0) for first end of the frustrum
            // let beta be the normalization factor for the layers (outer most would be 1.0) for other end of the frustrum
            double alpha = 1.0 / frustum->r2;
            double beta = 1.0 / frustum->r1;
            for(int c=0; c < layers; c++)
              {
              // Normalize the layer
              pincell->radii[2*c] *= alpha;
              pincell->radii[(2*c)+1] *= beta;

              // Get the material of the layer
              std::string mname;
              input >> mname;
              std::transform(mname.begin(), mname.end(), mname.begin(), ::tolower);
              // Lets save the first material to use to set the pin's color legend
              if (firstMaterial == "")
                {
                firstMaterial = mname;
                }
              frustum->materials[c] = mname;
              }

            // normalize radii
            }
          }
        cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
        pincell->SetLegendColor(matColorMap->MaterialColorMap()[firstMaterial.c_str()].Color);
        this->AddPinCell(pincell);
        }
      }
      else if(value == "assembly")
        {
        size_t x=0;
        size_t y=0;
        if(this->IsHexType())
          {
          input >> x;
          }
        else
          {
          input >> x >> y;
          }

        input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        this->AssyLattice.SetDimensions(x, y);

        if(input.peek() == '!')
          {
          input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
          }

        if(this->IsHexType())
          {
          // assuming a full hex assembly, NOT partial
          size_t maxN = 2*x - 1;
          std::vector<std::vector<std::string> > hexArray;
          hexArray.resize(maxN);
          size_t numCols, delta=0;

          for(size_t i = 0; i < maxN; i++)
            {
            if(i<x) // first half of HEX
              {
              numCols = i+x;
              }
            else // second half of HEX
              {
              delta++;
              numCols = maxN - delta;
              }
            hexArray[i].resize(numCols);
            for(size_t j = 0; j < numCols; j++)
              {
              input >> hexArray[i][j];
              }
            input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }

          // now we fill the hex Lattice with hexArray,
          // starting from out most layer and work toward center
          // for each layer, we have 6*Layer cells, except layer 0.
          for(int k = x-1; k >= 0; k--) // HEX layers
            {
            size_t numRows = 2*k + 1;
            size_t startRow = x-1-k;
            size_t startCol = startRow;
            size_t layerIdx;
            for(size_t i = startRow; i < numRows+startRow; i++) // array rows
              {
              if(i==startRow || i==numRows+startRow - 1) // first row or last row
                {
                for(size_t j= startCol, ringIdx=0; j<k+1+startCol; j++, ringIdx++)
                  {
                  layerIdx = i==startRow ? ringIdx : 4*k-ringIdx;
                  this->AssyLattice.Grid[k][layerIdx].label = hexArray[i][j];
                  }
                }
              else // rows between first and last
                {
                // get the first and last column defined by start column
                layerIdx = 6*k-(i-startRow);
                this->AssyLattice.Grid[k][layerIdx].label = hexArray[i][startCol];
                layerIdx = k+(i-startRow);
                size_t colIdx = hexArray[i].size() -1 - startCol;
                this->AssyLattice.Grid[k][layerIdx].label = hexArray[i][colIdx];
                }
              }
            }
          }
        else
          {
          for(size_t i = 0; i < x; i++)
            {
            for(size_t j = 0; j < y; j++)
              {
              input >> this->AssyLattice.Grid[i][j].label;
              }
            input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
          }
        }
      else if(value == "tetmeshsize")
        {
        input >> this->MeshSize;
        }
      else if(value == "radialmeshsize")
        {
        input >> this->RadialMeshSize;
        }
      else if(value == "axialmeshsize")
        {
        input >> this->AxialMeshSize;
        }
      else if(value == "rotate")
        {
        input >> this->RotateDirection >> this->RotateAngle;
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
  output << "GeometryType " << this->GeometryType << "\n";

  // materials
  QMap<std::string, std::string> materials;
  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  matColorMap->GetAssemblyMaterials(this, materials);
  output << "Materials " << materials.count();
  foreach(std::string name, materials.keys())
    {
    std::string material_name = materials[name];
    if(material_name.empty())
      {
      material_name = name;
      }

    output << " " << name << " " << material_name;
    }
  output << "\n";

  // ducts
  for(size_t i = 0; i < this->AssyDuct.Ducts.size(); i++)
    {
    Duct *duct = this->AssyDuct.Ducts[i];

    output << "duct " << duct->materials.size() << " ";
    output << std::showpoint << duct->x << " " << duct->y << " " << duct->z1 << " " << duct->z2;

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
      if(j==0) output << "cylinder " << pincell->GetNumberOfLayers() << " ";
      Cylinder* cylinder = pincell->cylinders[j];
      output << std::showpoint
        << cylinder->x << " "
        << cylinder->y << " "
        << cylinder->z1 << " "
        << cylinder->z2 << " ";
      for(int material = 0; material < cylinder->materials.size(); material++)
        {
        output << std::showpoint << pincell->radii[material] << " ";
        }
      for(int material = 0; material < cylinder->materials.size(); material++)
        {
        output << cylinder->materials[material] << " ";
        }
      if(j==pincell->cylinders.size()-1) output << "\n";
      }

    for(size_t j = 0; j < pincell->frustums.size(); j++)
      {
      if(j==0) output << "frustum " << pincell->GetNumberOfLayers() << " ";
      Frustum* frustum = pincell->frustums[j];
      output << std::showpoint
        << frustum->x << " "
        << frustum->y << " "
        << frustum->z1 << " "
        << frustum->z2 << " ";
      for(int material = 0; material < frustum->materials.size() / 2; material++)
        {
        output << std::showpoint << pincell->radii[material*2+0] << " ";
        output << std::showpoint << pincell->radii[material*2+1] << " ";
        }
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
      std::string label = this->AssyLattice.Grid[i][j].label;
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
  if(this->AssyDuct.Ducts.size()==0 || this->AssyLattice.Grid.size() == 0)
    {
    return NULL;
    }
  double outerDuctHeight = this->AssyDuct.Ducts[0]->thicknesses.back();
  double innerDuctHeight = this->AssyDuct.Ducts[0]->thicknesses.front();
  double chamberStart = outerDuctHeight - innerDuctHeight;
  double chamberEnd = innerDuctHeight;

  // setup data
  this->Data->SetNumberOfBlocks(this->AssyLattice.GetNumberOfCells() +
                                this->AssyDuct.Ducts.size());

  // For Hex type
  Duct *hexDuct = this->AssyDuct.Ducts[0];
  double layerCorners[6][2], hexRadius, hexDiameter, layerRadius;
  hexDiameter = hexDuct->thicknesses[0];
  hexRadius = hexDiameter / (double)(2 * cos(30.0 * vtkMath::Pi() / 180.0));
  hexRadius = hexRadius / (double)(2*this->AssyLattice.Grid.size()-1);

  double cellLength = (chamberEnd - chamberStart) / this->AssyLattice.Grid.size();

  for(size_t i = 0; i < this->AssyLattice.Grid.size(); i++)
    {
    // For hex geometry type, figure out the six corners first
    if(this->AssyLattice.GetGeometryType() == HEXAGONAL && i>0)
      {
      layerRadius = hexRadius * (2 * i);
      for(int c = 0; c < 6; c++)
        {
        double angle = 2 * (vtkMath::Pi() / 6.0) * (c + 3.5);
        layerCorners[c][0] = layerRadius * cos(angle);
        layerCorners[c][1] = layerRadius * sin(angle);
        }
      }

    size_t startBlock = this->AssyLattice.GetGeometryType() == HEXAGONAL ?
      (i==0 ? 0 : (1 + 3*i*(i-1))) : (i*this->AssyLattice.Grid.size());
    const std::vector<LatticeCell> &row = this->AssyLattice.Grid[i];
    for(size_t j = 0; j < row.size(); j++)
      {
      const std::string &type = row[j].label;

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

              if(this->AssyLattice.GetGeometryType() == HEXAGONAL)
                {
                double tX=hexDuct->x, tY=hexDuct->y, tZ=0.0;
                int cornerIdx;
                if(i == 1)
                  {
                  cornerIdx = j%6;
                  tX += layerCorners[cornerIdx][0];
                  tY += layerCorners[cornerIdx][1];
                  }
                else if( i > 1)
                  {
                  cornerIdx = j / i;
                  int idxOnEdge = j%i;
                  if(idxOnEdge == 0) // one of the corners
                    {
                    tX += layerCorners[cornerIdx][0];
                    tY += layerCorners[cornerIdx][1];
                    }
                  else
                    {
                    // for each layer, we should have (numLayers-2) middle hexes
                    // between the corners
                    double deltx, delty, numSegs = i, centerPos[2];
                    int idxNext = cornerIdx==5 ? 0 : cornerIdx+1;
                    deltx = (layerCorners[idxNext][0] - layerCorners[cornerIdx][0]) / numSegs;
                    delty = (layerCorners[idxNext][1] - layerCorners[cornerIdx][1]) / numSegs;
                    centerPos[0] = layerCorners[cornerIdx][0] + deltx * (idxOnEdge);
                    centerPos[1] = layerCorners[cornerIdx][1] + delty * (idxOnEdge);
                    tX += centerPos[0];
                    tY += centerPos[1];
                    }
                  }

                transform->Translate(tX, tY, tZ);
                }
              else
                {
                transform->Translate(chamberStart + i * cellLength + 0.5 * pincell->pitchX,
                  chamberStart + j * cellLength + 0.5 * pincell->pitchY,
                  0);
                }

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
          this->Data->SetBlock(startBlock+j, pinDataSet.GetPointer());
          }
        else
          {
          this->Data->SetBlock(startBlock+j, NULL);
          }
        }
      else
        {
        this->Data->SetBlock(startBlock+j, NULL);
        }
      }
    }

  // setup ducts
  double z, deltaZ, height;
  size_t numDucts = this->AssyDuct.Ducts.size();
  for(size_t i = 0; i < numDucts; i++)
    {
    Duct *duct = this->AssyDuct.Ducts[i];

    cmbNucDuctSource *ductSource = cmbNucDuctSource::New();
    z = duct->z1;
    height = duct->z2 - duct->z1;
    double deltaZ = height * 0.0005;
    // For first duct, move the Origin up in z by 0.05 % of the the Height so
    // that the bottoms of the pins are not covered by duct's bottom
    // For last duct, Reduce the height by 0.1 % of the Height so
    // that the tops of the pins are not covered by duct's top
    if(i == 0) // first duct
      {
      z = duct->z1 + deltaZ;
      // if more than one duct, first duct height need to be reduced by deltaZ
      height = numDucts > 1 ? height - deltaZ : height - 2*deltaZ;
      }
    else if (i == numDucts - 1) // last duct
      {
      height -= 2*deltaZ;
      }

    ductSource->SetOrigin(duct->x, duct->y, z);
    ductSource->SetHeight(height);
    ductSource->SetGeometryType(
      this->AssyLattice.GetGeometryType()== HEXAGONAL ?
      CMBNUC_ASSY_HEX_DUCT : CMBNUC_ASSY_RECT_DUCT);

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

vtkMultiBlockDataSet* cmbNucAssembly::CreatePinCellMultiBlock(PinCell* pincell, bool cutaway)
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

    if(cutaway)
      {
      vtkMultiBlockDataSet *coneData = coneSource->GetOutput();
      for(int block = 0; block < coneData->GetNumberOfBlocks(); block++)
        {
        vtkPolyData *coneLayerData =
          vtkPolyData::SafeDownCast(coneData->GetBlock(block));
        vtkNew<vtkClipClosedSurface> clipper;
        vtkNew<vtkPlaneCollection> clipPlanes;
        vtkNew<vtkPlane> plane;
        vtkNew<vtkPolyDataNormals> normals;
        plane->SetOrigin(0, 0 + 0.001 * block, 0);
        plane->SetNormal(0, 1, 0);
        clipPlanes->AddItem(plane.GetPointer());
        clipper->SetClippingPlanes(clipPlanes.GetPointer());
        clipper->SetActivePlaneId(0);
        clipper->SetClipColor(1.0,1.0,1.0);
        clipper->SetActivePlaneColor(1.0,1.0,0.8);
        clipper->GenerateOutlineOff();
        clipper->SetInputData(coneLayerData);
        clipper->GenerateFacesOn();
        normals->SetInputConnection(clipper->GetOutputPort());
        normals->Update();
        coneData->SetBlock(block, normals->GetOutput());
        }
      dataSet->SetBlock(j, coneData);
      }
    else
      {
      dataSet->SetBlock(j, coneSource->GetOutput());
      }
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

    if(cutaway)
      {
      vtkMultiBlockDataSet *coneData = coneSource->GetOutput();
      for(int block = 0; block < coneData->GetNumberOfBlocks(); block++)
        {
        vtkPolyData *coneLayerData =
          vtkPolyData::SafeDownCast(coneData->GetBlock(block));
        vtkNew<vtkClipClosedSurface> clipper;
        vtkNew<vtkPlaneCollection> clipPlanes;
        vtkNew<vtkPlane> plane;
        vtkNew<vtkPolyDataNormals> normals;
        plane->SetOrigin(0, 0 + 0.001 * block, 0);
        plane->SetNormal(0, 1, 0);
        clipPlanes->AddItem(plane.GetPointer());
        clipper->SetClippingPlanes(clipPlanes.GetPointer());
        clipper->SetActivePlaneId(0);
        clipper->SetClipColor(1.0,1.0,1.0);
        clipper->SetActivePlaneColor(1.0,1.0,0.8);
        clipper->GenerateOutlineOff();
        clipper->SetInputData(coneLayerData);
        clipper->GenerateFacesOn();
        normals->SetInputConnection(clipper->GetOutputPort());
        normals->Update();
        coneData->SetBlock(block, normals->GetOutput());
        }
      dataSet->SetBlock(numCyls+j, coneData);
      }
    else
      {
      dataSet->SetBlock(numCyls+j, coneSource->GetOutput());
      }
    }

  return dataSet;
}
