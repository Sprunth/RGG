
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
#include "vtkMath.h"

#include <QFileInfo>
#include <QDir>

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
  this->CoreLattice.SetDimensions(i, j);
}

void cmbNucCore::AddAssembly(cmbNucAssembly *assembly)
{
  if(this->Assemblies.size()==0)
    {
    if(assembly)
      {
      this->CoreLattice.SetGeometryType(
        assembly->AssyLattice.GetGeometryType());
      }
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
  this->CoreLattice.ClearCell(label);
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
  if(this->Assemblies.size()==0 || this->CoreLattice.Grid.size()==0
    || this->CoreLattice.Grid[0].size()==0)
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

  // Is this Hex type?
  bool isHex = Assemblies[0]->AssyLattice.GetGeometryType() == HEXAGONAL;

  // setup data
  size_t numBlocks = isHex ?
    (1 + 3*(int)this->CoreLattice.Grid.size()*((int)this->CoreLattice.Grid.size() - 1)) :
    this->CoreLattice.Grid.size()*this->CoreLattice.Grid[0].size();

  this->Data->SetNumberOfBlocks(numBlocks);

  for(size_t i = 0; i < this->CoreLattice.Grid.size(); i++)
    {
    size_t startBlock = isHex ?
      (i==0 ? 0 : (1 + 3*i*(i-1))) : (i*this->CoreLattice.Grid.size());

    const std::vector<LatticeCell> &row = this->CoreLattice.Grid[i];
    for(size_t j = 0; j < row.size(); j++)
      {
      const std::string &type = row[j].label;

      if(!type.empty() && type != "xx" && type != "XX")
        {
        cmbNucAssembly* assembly = this->GetAssembly(type);
        vtkSmartPointer<vtkMultiBlockDataSet> assemblyData = assembly->GetData();
        vtkNew<vtkTransform> transform;

        if(isHex)
          {
          Duct *hexDuct = assembly->AssyDuct.Ducts[0];
          double layerCorners[6][2], hexDiameter, layerRadius;
          hexDiameter = hexDuct->thicknesses.back();

          // For hex geometry type, figure out the six corners first
          if(isHex && i>0)
            {
            layerRadius = hexDiameter * i;
            for(int c = 0; c < 6; c++)
              {
              // Needs a little more thinking on math here?
              double angle = 2 * (vtkMath::Pi() / 6.0) * (c+3);
              layerCorners[c][0] = layerRadius * cos(angle);
              layerCorners[c][1] = layerRadius * sin(angle);
              }
            }

          double tX=startX, tY=startY, tZ=0.0;
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
          transform->Translate(startX + i * (outerDuctHeight+0.5),
            startY + j * (outerDuctHeight+0.5),
            0);
          }
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
       this->Data->SetBlock(startBlock+j, blockData.GetPointer());

/* // The following (better) method is not working ???, even the pipeline is
   // be default compositepipeline

             vtkNew<vtkTransformFilter> filter;
            filter->SetTransform(transform.GetPointer());
            filter->SetInputDataObject(assemblyData);
            filter->Update();
        this->Data->SetBlock(i*this->Grid.size()+j, filter->GetOutput());
*/
        vtkInformation* info = this->Data->GetMetaData(startBlock+j);
        info->Set(vtkCompositeDataSet::NAME(), assembly->label.c_str());
        }
      else
        {
        this->Data->SetBlock(startBlock+j, NULL);
        }
      }
    }

  return this->Data;
}

bool cmbNucCore::IsHexType()
{
  std::string strGeoType = this->GeometryType;
  std::transform(this->GeometryType.begin(), this->GeometryType.end(),
    strGeoType.begin(), ::tolower);
  return strGeoType == "hexflat" || strGeoType == "hexvertex";
}

void cmbNucCore::ReadFile(const std::string &FileName)
{
  std::ifstream input(FileName.c_str());
  if(!input.is_open())
    {
    std::cerr << "failed to open input file" << std::endl;
    return;
    }
  QFileInfo info(FileName.c_str());
  std::string strPath = info.dir().path().toStdString();

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
        this->CoreLattice.SetGeometryType(HEXAGONAL);
        }
      else
        {
        this->CoreLattice.SetGeometryType(RECTILINEAR);
        }
      this->CoreLattice.SetDimensions(0, 0);
      }
    else if(value == "assemblies")
      {
      int count;
      input >> count;
      if(this->IsHexType()) // just one pitch
        {
        input >> this->AssyemblyPitchX;
        this->AssyemblyPitchY = this->AssyemblyPitchX;
        }
      else
        {
        input >> this->AssyemblyPitchX >> this->AssyemblyPitchY;
        }

      input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      // read in assembly files
      for(int i = 0; i < count; i++)
        {
        std::string assyfilename, assylabel, tmpPath = strPath;
        input >> assyfilename >> assylabel;
        tmpPath.append("/").append(assyfilename);
        QFileInfo tmpInfo(tmpPath.c_str());
        tmpPath = strPath + "/" + tmpInfo.baseName().toStdString() + ".inp";
        QFileInfo assyInfo(tmpPath.c_str());
        if(assyInfo.exists())
          {
          this->loadAssemblyFromFile(tmpPath, assylabel);
          }
        if(i != count - 1)
          {
          input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
          }
        }
      }
    else if(value == "lattice")
      {
      size_t x=0;
      size_t y=0;
      if(this->IsHexType())
        {
        input >> x;
        y = x;
        }
      else
        {
        // the lattice 2d grid use y as rows, x as columns
        input >> y >> x;
        }

      input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

      this->CoreLattice.SetDimensions(x, y);

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
                this->CoreLattice.Grid[k][layerIdx].label = hexArray[i][j];
                }
              }
            else // rows between first and last
              {
              // get the first and last column defined by start column
              layerIdx = 6*k-(i-startRow);
              this->CoreLattice.Grid[k][layerIdx].label = hexArray[i][startCol];
              layerIdx = k+(i-startRow);
              size_t colIdx = hexArray[i].size() -1 - startCol;
              this->CoreLattice.Grid[k][layerIdx].label = hexArray[i][colIdx];
              }
            }
          }
        }
      else
        {
        for(size_t j = 0; j < y; j++)
          {
          for(size_t i = 0; i < x; i++)
            {
            input >> this->CoreLattice.Grid[i][j].label;
            }
          input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
          }
        }
      }
    else if(value == "background")
      {
      input >> this->BackgroudMeshFile;
      }
    input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

cmbNucAssembly* cmbNucCore::loadAssemblyFromFile(
  const std::string &fileName, const std::string &assyLabel)
{
  // read file and create new assembly
  cmbNucAssembly* assembly = new cmbNucAssembly;
  assembly->label = assyLabel;
  assembly->ReadFile(fileName);
  this->AddAssembly(assembly);
  return assembly;
}