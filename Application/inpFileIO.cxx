#include "inpFileIO.h"
#include "cmbNucAssembly.h"
#include "cmbNucCore.h"
#include "cmbNucMaterialColors.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <fstream>

#include <stdlib.h>

#include <QDebug>
#include <QMap>
/*
#define ASSEMBLY_KEYWORD 0x01000
#define CORE_KEYWORD 0x02000
#define BOTH_KEYWORD (ASSEMBLY_KEYWORD & CORE_KEYWORD)

#define both_keywords_macro() \
  fun( GeometryType) \
  fun( GeomEngine) \
  fun( Geometry)\
  fun( MergeTolerance) \
  fun( End) \
  fun( Info)

#define assembly_keywords_macro() \
  fun(Materials) \
  fun(StartPinId ) \
  fun(MeshType ) \
  fun(Duct ) \
  fun(Dimensions ) \
  fun(Pincells ) \
  fun(Cylinder ) \
  fun(Frustum ) \
  fun(CellMaterial ) \
  fun(HBlock ) \
  fun(MaterialSet_StartId ) \
  fun(Neumannset_StartId ) \
  fun(Section ) \
  fun(Center ) \
  fun(Move ) \
  fun(Rotate ) \
  fun(CreateSideset ) \
  fun(CreateFiles ) \
  fun(CreateMatFiles ) \
  fun(Save_Exodus ) \
  fun(RadialMeshSize ) \
  fun(TetMeshSize ) \
  fun(AxialMeshSize ) \
  fun(EdgeInterval ) \
  fun(List_NeumannSet_StartId ) \
  fun(List_MaterialSet_StartId ) \
  fun(NumSuperBlocks ) \
  fun(SuperBlocks )

#define core_keywords_macro() \
 fun( ProblemType ) \
 fun( Symmetry  ) \
 fun( SaveParallel  ) \
 fun( MeshInfo  ) \
 fun( NeumannSet  ) \
 fun( Extrude  ) \
 fun( Background  ) \
 fun( OutputFileName  )
*/

class inpFileHelper
{
public:
  std::map<std::string, std::string> materialLabelMap;
  template <typename TYPE>
  void readGeometryType( std::stringstream & input,
                         TYPE &v, Lattice &lat)
  {
    input >> v.GeometryType;
    if(v.IsHexType())
      {
      lat.SetGeometryType(HEXAGONAL);
      }
    else
      {
      lat.SetGeometryType(RECTILINEAR);
      }
    lat.SetDimensions(0, 0);
  }

  void readLattice( std::stringstream & input, bool isHex,
                    int hexSymmetry, Lattice &lat);
  void readMaterials( std::stringstream & input, cmbNucAssembly & assembly );
  void readDuct( std::stringstream & input, cmbNucAssembly & assembly );
  void readPincell( std::stringstream & input, cmbNucAssembly & assembly );

  void writeHeader( std::ofstream &output, std::string type );
  void writeMaterials( std::ofstream &output, cmbNucAssembly & assembly );
  void writeDuct( std::ofstream &output, cmbNucAssembly & assembly );
  void writePincell( std::ofstream &output, cmbNucAssembly & assembly );
  void writeLattice( std::ofstream &output, std::string key, bool isHex,
                     int hexSymmetry, Lattice &lat );
};

//============================================================================
//READING
//============================================================================


inpFileReader
::inpFileReader()
:Type(UNKNOWN)
{
}

inpFileReader::FileType
inpFileReader
::open(std::string fname)
{
  close();
  this->FileName = fname;
  std::ifstream input(FileName.c_str());
  if(!input.is_open())
    {
    close();
    return ERROR;
    }
  while(!input.eof())
    {
    std::string line("");
    std::getline(input,line);
    line = line.substr(0, line.find_first_of('!'));
    if(line.empty()) continue;
    std::stringstream ss(line);
    std::string tag;
    ss >> tag;
    std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
    if(tag == "assembly" && Type == UNKNOWN)
      {
      if(Type == UNKNOWN)
        Type = ASSEMBLY;
      else if( Type == CORE )
        {
        std::cerr << "Cannot distinguish file" << std::endl;
        close();
        return ERROR;
        }
      }
    else if( tag == "assemblies" )
      {
      if(Type == UNKNOWN)
        Type = CORE;
      else if( Type == ASSEMBLY )
        {
        std::cerr << "Cannot distinguish file" << std::endl;
        close();
        return ERROR;
        }
      }
    bool has_amp = line.find_first_of('&') != std::string::npos;
    if(has_amp)
      {
      std::replace(line.begin(), line.end(), '&', ' ');
      CleanFile += " " + line;
      }
    else
      {
      CleanFile += "\n" + line;
      }
    }
  return Type;
}

void inpFileReader
::close()
{
  CleanFile = "";
  Type = UNKNOWN;
  this->FileName= "";
}

bool inpFileReader
::read(cmbNucAssembly & assembly)
{
  if(Type != ASSEMBLY)
    return false;
  inpFileHelper helper;
  assembly.FileName = FileName;
  std::stringstream input(CleanFile);
  while(!input.eof())
  {
    std::string value;
    input >> value;

    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    qDebug() << value.c_str();

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
      helper.readGeometryType( input, assembly, assembly.AssyLattice );
      }
    else if(value == "materials")
      {
      helper.readMaterials( input, assembly );
      }
    else if(value == "duct")
      {
      helper.readDuct( input, assembly );
      }
    else if(value == "pincells")
      {
      helper.readPincell( input, assembly );
      }
    else if(value == "assembly")
      {
      helper.readLattice( input, assembly.IsHexType(), 1, assembly.AssyLattice );
      }
    else if(value == "tetmeshsize")
      {
      input >> assembly.Parameters->TetMeshSize;
      }
    else if(value == "radialmeshsize")
      {
      input >> assembly.Parameters->RadialMeshSize;
      }
    else if(value == "axialmeshsize")
      {
      input >> assembly.Parameters->AxialMeshSize;
      }
    else if(value == "rotate")
      {
      input >> assembly.Parameters->RotateXYZ >> assembly.Parameters->RotateAngle;
      }
    else if(value == "hblock")
      {
      std::getline(input, assembly.Parameters->HBlock);
      }
    else if(value == "geometry")
      {
      input >> assembly.Parameters->Geometry;
      }
    }
  return true;
}

bool inpFileReader
::read(cmbNucCore & core)
{
  if(Type != ASSEMBLY)
    return false;
  return true;
}

//============================================================================
//Writing
//============================================================================

#define WRITE_PARAM_VALUE(KEY, VALUE)\
if(params->isValueSet(params->VALUE))\
  output << #KEY << " " << params->VALUE << std::endl

#define TEST_PARAM_VALUE(VALUE) \
        params->isValueSet(params->VALUE)

bool inpFileWriter::write(std::string fname,
                          cmbNucAssembly & assembly,
                          bool updateFname)
{
  inpFileHelper helper;
  std::ofstream output(fname.c_str());
  if(!output.is_open())
    {
    return false;
    }
  if(updateFname)
    {
    assembly.FileName = fname;
    }
  helper.writeHeader(output,"Assembly");
  cmbAssyParameters * params = assembly.Parameters;

  WRITE_PARAM_VALUE(MeshType, MeshType);
  output << "GeometryType " << assembly.GeometryType << "\n";
  WRITE_PARAM_VALUE(Geometry, Geometry);
  helper.writeMaterials( output, assembly );
  helper.writeDuct( output, assembly );
  helper.writePincell( output, assembly );
  helper.writeLattice( output, "Assembly",
                       assembly.IsHexType(), 1,
                       assembly.AssyLattice );
  //Other Parameters
  WRITE_PARAM_VALUE(TetMeshSize, TetMeshSize);
  WRITE_PARAM_VALUE(EdgeInterval, EdgeInterval);
  WRITE_PARAM_VALUE(RadialMeshSize, RadialMeshSize);
  WRITE_PARAM_VALUE(MaterialSet_StartId, MaterialSet_StartId);
  WRITE_PARAM_VALUE(NeumannSet_StartId, NeumannSet_StartId);
  WRITE_PARAM_VALUE(Move, MoveXYZ);

  output << "Center";
  if(params->isValueSet(params->CenterXYZ))
  {
    output << " " << params->CenterXYZ;
  }
  output << "\n";

  //Rotate
  if(TEST_PARAM_VALUE(RotateXYZ) && TEST_PARAM_VALUE(RotateAngle))
  {
    output << "Rotate "
           << params->RotateXYZ << ' '
           << params->RotateAngle << "\n";
  }

  //Section
  if (TEST_PARAM_VALUE(SectionXYZ) && TEST_PARAM_VALUE(SectionOffset))
  {
    output << "Section " << params->SectionXYZ << ' '
           << params->SectionOffset << ' '
           << ((params->SectionReverse)?1:0) << "\n";
  }
  WRITE_PARAM_VALUE(AxialMeshSize, AxialMeshSize);
  WRITE_PARAM_VALUE(CreateSideset, CreateSideset);
  WRITE_PARAM_VALUE(MergeTolerance, MergeTolerance);
  WRITE_PARAM_VALUE(HBlock, HBlock);
  // end
  output << "end\n";
  output.close();

  return true;
}

bool inpFileWriter::write(std::string fname,
                          cmbNucCore & core,
                          bool updateFname)
{
  return true;
}

//============================================================================
//Helpers
//============================================================================

void inpFileHelper::writeHeader( std::ofstream & output, std::string type )
{
  output << "!   ########################################################\n";
  output << "!   " << type << " File Generated by RGG GUI\n";
  output << "!   ########################################################\n";
}

void inpFileHelper::writeMaterials( std::ofstream &output,
                                    cmbNucAssembly & assembly )
{
  QMap<std::string, std::string> materials;
  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  matColorMap->GetAssemblyMaterials(&assembly, materials);
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
}

void inpFileHelper::readMaterials( std::stringstream & input,
                                   cmbNucAssembly & assembly )
{
  int count;
  input >> count;
  std::string mlabel;

  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  for(int i = 0; i < count; i++)
    {
    std::string mname;
    input >> mname;
    input >> mlabel;

    materialLabelMap[mlabel] = mname;
    std::transform(mname.begin(), mname.end(), mname.begin(), ::tolower);
    if(!matColorMap->MaterialColorMap().contains(mname.c_str()))
      {
      matColorMap->AddMaterial(mname.c_str(), mlabel.c_str());
      }
    else
      {
      // replace the label
      const QColor &color = matColorMap->MaterialColorMap()[mname.c_str()].Color;
      matColorMap->AddMaterial(mname.c_str(), mlabel.c_str(), color);
      }
    }
}

void inpFileHelper::writeDuct( std::ofstream &output, cmbNucAssembly & assembly )
{
  for(size_t i = 0; i < assembly.AssyDuct.Ducts.size(); i++)
  {
    Duct *duct = assembly.AssyDuct.Ducts[i];

    output << "duct " << duct->materials.size() << " ";
    output << std::showpoint << duct->x << " " << duct->y << " " << duct->z1 << " " << duct->z2;

    if(assembly.IsHexType())
      {
      for(int i = 0; i <  duct->materials.size(); i++)
        {
        output << " " << duct->thicknesses[i*2];
        }
      }
    else
      {
      for(size_t j = 0; j < duct->thicknesses.size(); j++)
        {
        output << " " << duct->thicknesses[j];
        }
      }

    for(size_t j = 0; j < duct->materials.size(); j++)
    {
      output << " " << duct->materials[j];
    }
    output << "\n";
  }
}

void inpFileHelper::readDuct( std::stringstream & input, cmbNucAssembly & assembly )
{
  Duct* duct = new Duct();
  int materials;
  std::string mlabel;

  input >> materials
        >> duct->x
        >> duct->y
        >> duct->z1
        >> duct->z2;

  duct->thicknesses.resize(materials*2);
  if(assembly.IsHexType())
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
    input >> mlabel;
    duct->materials[i] = materialLabelMap[mlabel];
    }

  assembly.AssyDuct.Ducts.push_back(duct);
}

void inpFileHelper::writePincell( std::ofstream &output, cmbNucAssembly & assembly )
{
  output << "pincells " << assembly.PinCells.size() << "\n";

  for(size_t i = 0; i < assembly.PinCells.size(); i++)
    {
    PinCell* pincell = assembly.PinCells[i];

    // count of attribute lines for the pincell. equal to the number
    // of frustums plus cylinders plus one for the pitch.
    // We are writing multiple cylinders/frustums on one line.
    size_t count = (pincell->cylinders.size()>0 ? 1: 0) +
                   (pincell->frustums.size()>0 ? 1 : 0) + 1;

    output << pincell->name << " " << pincell->label << " " << count << "\n";

    output << "pitch " << pincell->pitchX;
    if(!assembly.IsHexType())
      {
      output << " " << pincell->pitchY << " " << pincell->pitchZ;
      }
    output << "\n";

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
        output << std::showpoint << pincell->radii[material]*cylinder->r << " ";
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
}

void inpFileHelper::readPincell( std::stringstream &input, cmbNucAssembly & assembly )
{
  std::string value;
  std::string mlabel;
  int count = 0;
  input >> count;
  // for Hex type, the pitch is next input.
  double hexPicth = -1.0;
  if(assembly.IsHexType())
    {
    std::string hexPicthStr;
    std::getline(input, hexPicthStr);
    remove_if(hexPicthStr.begin(), hexPicthStr.end(), isspace);
    if(!hexPicthStr.empty())
      {
      hexPicth = atof(hexPicthStr.c_str());
      }
    }

  for(int i = 0; i < count; i++)
    {
    int lc = i % 10; // Pick a default pin color (for now!)
    PinCell* pincell = new PinCell();
    std::string firstMaterial;
    int attribute_count = 0;
    input >> pincell->name;

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
        if(assembly.IsHexType())
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

          // Get the material of the layer - note that we read in the material label that
          // maps to the actual material
          std::string mname;
          input >> mlabel;
          mname = materialLabelMap[mlabel];
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

          // Get the material of the layer - note that we read in the material label that
          // maps to the actual material
          std::string mname;
          input >> mlabel;
          mname = materialLabelMap[mlabel];
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
    assembly.AddPinCell(pincell);
    }
}

void inpFileHelper::writeLattice( std::ofstream &output, std::string key, bool isHex,
                                 int hexSymmetry, Lattice &lat )
{
  output << key << " " << lat.Grid.size();
  if(!isHex)
    {
    output << " " << lat.Grid[0].size();
    }
  output << "\n";
  if(isHex)
    {
    if(hexSymmetry == 1)
      {
      size_t x = lat.Grid.size();
      size_t maxN = 2*x - 1;
      std::vector<std::vector<std::string> > hexArray;
      hexArray.resize(maxN);
      size_t numCols = 0;
      size_t delta=0;
      for(size_t i = 0; i < maxN; i++)
        {
        if(i<lat.Grid.size()) // first half of HEX
          {
          numCols = i+x;
          }
        else // second half of HEX
          {
          delta++;
          numCols = maxN - delta;
          }
        hexArray[i].resize(numCols);
        }

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
              hexArray[i][j] = lat.Grid[k][layerIdx].label;
              }
            }
          else // rows between first and last
            {
            // get the first and last column defined by start column
            layerIdx = 6*k-(i-startRow);
            hexArray[i][startCol] = lat.Grid[k][layerIdx].label;
            layerIdx = k+(i-startRow);
            size_t colIdx = hexArray[i].size() -1 - startCol;
            hexArray[i][colIdx] =lat.Grid[k][layerIdx].label;
            }
          }
        }

      for(size_t i = 0; i < hexArray.size(); ++i)
        {
        for (size_t j = 0; j < hexArray[i].size(); ++j)
          {
          std::string label = hexArray[i][j];
          if(label.empty())
            {
            label = "xx";
            }
          output << label << " ";
          }
        output << "\n";
        }
      }
    }
  else
    {
    for(size_t i = 0; i < lat.Grid.size(); i++)
      {
      for(size_t j = 0; j < lat.Grid[i].size(); j++)
        {
        std::string label = lat.Grid[i][j].label;
        if(label.empty())
          {
          label = "xx";
          }
        output << label << " ";
        }
      output << "\n";
      }
    }
}


void inpFileHelper::readLattice( std::stringstream & input,
                                 bool isHex, int hexSymmetry,
                                 Lattice &lattice )
{
  size_t x=0;
  size_t y=0;
  if(isHex)
    {
    input >> x;
    y = x;
    }
  else
    {
    // the lattice 2d grid use y as rows, x as columns
    input >> y >> x;
    }

  lattice.SetDimensions(x, y);

  if(isHex)
    {
    if(hexSymmetry == 1)
      {
      // a full hex assembly, NOT partial
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
              lattice.Grid[k][layerIdx].label = hexArray[i][j];
              }
            }
          else // rows between first and last
            {
            // get the first and last column defined by start column
            layerIdx = 6*k-(i-startRow);
            lattice.Grid[k][layerIdx].label = hexArray[i][startCol];
            layerIdx = k+(i-startRow);
            size_t colIdx = hexArray[i].size() -1 - startCol;
            lattice.Grid[k][layerIdx].label = hexArray[i][colIdx];
            }
          }
        }
      }
    else if(hexSymmetry == 6 || hexSymmetry == 12)
      {
      std::string tmpVal;
      for(size_t i = 0; i < x; i++)
        {
        size_t cols = i + 1;
        if(hexSymmetry == 12)
          {
          cols = i%2 ? (i+1)/2 :(i+2)/2;
          }
        for( size_t j = 0; j < cols; j++)
          {
          input >> tmpVal;
          lattice.Grid[i][j].label = tmpVal;
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
        std::string label;
        input >> label;
        lattice.Grid[i][j].label = label;
        }
      }
    }
}
