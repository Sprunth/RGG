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
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>

typedef cmbNucCoreParams::ExtrudeStruct ExtrudedType;
typedef cmbNucCoreParams::NeumannSetStruct NeumannSetType;
typedef std::vector<NeumannSetType> NeumannSetTypeVec;

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
  void readMaterials( std::stringstream & input, cmbNucAssembly &assembly );
  void readDuct( std::stringstream & input, cmbNucAssembly &assembly );
  void readPincell( std::stringstream & input, cmbNucAssembly &assembly );
  void readAssemblies( std::stringstream & input, cmbNucCore &core,
                       std::string strPath, bool readAssemblies );
  template<class TYPE> void read( std::stringstream & input,
                                  bool /*isHex*/,
                                  std::string /*mesg*/,
                                  TYPE &destination )
  {
    input >> destination;
  }

  void readUnknown( std::stringstream &input, std::string value,
                   std::vector<std::string> &unknowns)
  {
    std::string restOfLine;
    std::getline(input, restOfLine);
    unknowns.push_back(value + " "+restOfLine);
  }

  void writeHeader( std::ofstream &output, std::string type );
  void writeMaterials( std::ofstream &output, cmbNucAssembly &assembly );
  void writeDuct( std::ofstream &output, cmbNucAssembly &assembly );
  void writePincell( std::ofstream &output, cmbNucAssembly &assembly );
  void writeLattice( std::ofstream &output, std::string key, bool isHex,
                     int hexSymmetry, bool useAmp, Lattice &lat );
  void writeAssemblies( std::ofstream &output, std::string outFileName,
                        cmbNucCore &core );

  template<class TYPE> void write( std::ofstream &output,
                                   std::string key,
                                   bool /*isHex*/,
                                   std::string /*mesg*/,
                                   TYPE &value )
  {
    output << key << " " << value << "\n";
  }

  void writeUnknown( std::ofstream &output,
                     std::vector<std::string> &unknowns )
  {
    for(unsigned int i = 0; i < unknowns.size(); ++i)
      {
      output << unknowns[i] << "\n";
      }
  }
};

template<>
void inpFileHelper
::read<ExtrudedType>( std::stringstream & input,
                        bool /*isHex*/,
                        std::string /*mesg*/,
                        ExtrudedType &extrude )
{
  input >> extrude.Size >> extrude.Divisions;
}

template<>
void inpFileHelper
::read<NeumannSetTypeVec>( std::stringstream & input,
                             bool /*isHex*/,
                             std::string /*mesg*/,
                             NeumannSetTypeVec &extrude )
{
  NeumannSetType nst;
  input >> nst.Side >> nst.Id;
  std::getline(input, nst.Equation);
  extrude.push_back(nst);
}

template<>
void inpFileHelper
::read<bool>( std::stringstream & input,
                bool /*isHex*/,
                std::string mesg,
                bool &destination )
{
  std::string v;
  input >> v;
  std::transform(v.begin(), v.end(), v.begin(), ::tolower);
  destination = v == mesg;
}

template<>
void inpFileHelper::
write<ExtrudedType>( std::ofstream &output,
                     std::string key,
                     bool /*isHex*/,
                     std::string /*mesg*/,
                     ExtrudedType &extrude )
{
  output << key << " " << extrude.Size << " " << extrude.Divisions << "\n";
}

template<>
void inpFileHelper::
write<NeumannSetTypeVec>( std::ofstream &output,
                          std::string key,
                          bool /*isHex*/,
                          std::string /*mesg*/,
                          NeumannSetTypeVec &nvect )
{
  for(unsigned int i = 0; i <nvect.size(); ++i )
  {
    output << key << " " << nvect[i].Side << " " << nvect[i].Id << " " << nvect[i].Equation << "\n";
  }
}

template<>
void inpFileHelper::
write<bool>( std::ofstream &output,
             std::string key,
             bool /*isHex*/,
             std::string mesg,
             bool &/*d*/ )
{
  output << key << " " << mesg << "\n";
}



//============================================================================
//READING
//============================================================================

inpFileReader
::inpFileReader()
:Type(UNKNOWN_TYPE)
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
    return ERROR_TYPE;
    }
  bool had_amp = false;
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
    if(tag == "assembly" && Type == UNKNOWN_TYPE)
      {
      if(Type == UNKNOWN_TYPE)
        Type = ASSEMBLY_TYPE;
      else if( Type == CORE_TYPE )
        {
        std::cerr << "Cannot distinguish file" << std::endl;
        close();
        return ERROR_TYPE;
        }
      }
    else if( tag == "assemblies" )
      {
      if(Type == UNKNOWN_TYPE)
        Type = CORE_TYPE;
      else if( Type == ASSEMBLY_TYPE )
        {
        std::cerr << "Cannot distinguish file" << std::endl;
        close();
        return ERROR_TYPE;
        }
      }
    bool found_amp = line.find_first_of('&') != std::string::npos;
    if(found_amp)
      {
      std::replace(line.begin(), line.end(), '&', ' ');
      }

    if(had_amp)
      {
      CleanFile += " " + line;
      }
    else
      {
      CleanFile += "\n" + line;
      }
    had_amp = found_amp;
    }
  return Type;
}

void inpFileReader
::close()
{
  CleanFile = "";
  Type = UNKNOWN_TYPE;
  this->FileName= "";
}

bool inpFileReader
::read(cmbNucAssembly & assembly)
{
  if(Type != ASSEMBLY_TYPE)
    return false;
  inpFileHelper helper;
  assembly.FileName = FileName;
  assembly.clear();
  assembly.setAndTestDiffFromFiles(false); //Should not be different
  std::stringstream input(CleanFile);
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
      helper.readGeometryType( input, assembly, assembly.AssyLattice );
      }
    else if(value == "materials")
      {
      helper.readMaterials( input, assembly );
      }
    else if(value == "duct" || value == "dimensions")
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
    else if(value == "move")
      {
      input >> assembly.Parameters->MoveXYZ[0]
            >> assembly.Parameters->MoveXYZ[1]
            >> assembly.Parameters->MoveXYZ[2];
      }
    else if(value == "hblock")
      {
      std::getline(input, assembly.Parameters->HBlock);
      }
    else if(value == "geometry")
      {
      input >> assembly.Parameters->Geometry;
      }
    else if(value == "center")
      {
      std::string tmp;
      std::getline(input, tmp);
      if(!tmp.empty())
        {
        assembly.Parameters->CenterXYZ = tmp;
        }
      }
#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, MSG)\
    else if(value == #Key)\
      { \
      helper.read(input, assembly.IsHexType(), MSG, assembly.Parameters->Var);\
      }
    ASSYGEN_EXTRA_VARABLE_MACRO()
#undef FUN_SIMPLE
    else
      {
      helper.readUnknown(input, value, assembly.Parameters->UnknownParams);
      }
    }
  return true;
}

bool inpFileReader
::read(cmbNucCore & core, bool read_assemblies)
{
  if(Type != CORE_TYPE)
    return false;
  QFileInfo info(FileName.c_str());
  std::string strPath = info.dir().path().toStdString();

  inpFileHelper helper;
  core.FileName = FileName;
  core.h5mFile = (info.completeBaseName() + ".h5m").toStdString();
  std::stringstream input(CleanFile);
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
      helper.readGeometryType( input, core, core.CoreLattice );
      }
    else if(value == "symmetry")
      {
      input >> core.HexSymmetry;
      }
    else if(value == "assemblies")
      {
      helper.readAssemblies( input, core, strPath, read_assemblies );
      }
    else if(value == "lattice")
      {
      helper.readLattice( input, core.IsHexType(), core.HexSymmetry, core.CoreLattice );
      }
    else if(value == "background")
      {
      getline(input, core.Params.Background);
      core.Params.Background = QString(core.Params.Background.c_str()).trimmed().toStdString();
      //check to make sure the file exists.
      QFileInfo tmpFI( QDir(strPath.c_str()),
                       core.Params.Background.c_str() );
      if(!tmpFI.exists())
        {
        QMessageBox msgBox;
        msgBox.setText( QString(core.Params.Background.c_str()) +
                        QString(" was not found in same director as the core inp file."));
        msgBox.exec();
        }
      core.Params.BackgroundFullPath = tmpFI.absoluteFilePath().toStdString();

      }
    else if(value == "outputfilename")
      {
      core.h5mFile.clear();
      getline(input, core.h5mFile);
      core.h5mFile = QString(core.h5mFile.c_str()).trimmed().toStdString();
      }
#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, MSG) \
    else if( value == #Key) \
      {\
        helper.read(input, core.IsHexType(), MSG, core.Params.Var);\
      }
#define FUN_STRUCT(TYPE,X,Var,Key,DEFAULT, MSG) FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, MSG)
      EXTRA_VARABLE_MACRO()
#undef FUN_SIMPLE
#undef FUN_STRUCT
    else //unknown
      {
        helper.readUnknown(input, value, core.Params.UnknownKeyWords);
      }
    }
  core.setAndTestDiffFromFiles(false);
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
                       assembly.IsHexType(),
                       1, false,
                       assembly.AssyLattice );
  //Other Parameters
  WRITE_PARAM_VALUE(TetMeshSize, TetMeshSize);
  WRITE_PARAM_VALUE(RadialMeshSize, RadialMeshSize);
  if(params->MoveXYZ[0]!=0 || params->MoveXYZ[1]!=0 || params->MoveXYZ[2]!=0)
    {
    output << "Move "
          << params->MoveXYZ[0] << " "
          << params->MoveXYZ[1] << " "
          << params->MoveXYZ[2] << "\n";
    }

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
  WRITE_PARAM_VALUE(HBlock, HBlock);
#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, DK)\
if(params->isValueSet(params->Var))\
  output << #Key << " " << params->Var << "\n";
  ASSYGEN_EXTRA_VARABLE_MACRO()
#undef FUN_SIMPLE

  for(unsigned int i = 0; i < assembly.Parameters->UnknownParams.size(); ++i)
  {
    output << assembly.Parameters->UnknownParams[i] << "\n";
  }

  // end
  output << "end\n";
  output.close();
  assembly.setAndTestDiffFromFiles(false);

  return true;
}
#undef WRITE_PARAM_VALUE

bool inpFileWriter::write(std::string fname,
                          cmbNucCore & core,
                          bool updateFname)
{
  inpFileHelper helper;
  QFileInfo info(fname.c_str());
  std::ofstream output(fname.c_str());
  if(!output.is_open())
    {
    return false;
    }
  if(updateFname)
    {
    core.FileName = fname;
    }
  if(core.h5mFile.empty())
  {
    core.h5mFile = (info.completeBaseName() + ".h5m").toStdString();
  }
  core.computePitch();
  helper.writeHeader(output,"Assembly");
  output << "Symmetry "  << core.HexSymmetry << "\n";
  output << "GeometryType " << core.GeometryType << "\n";
  helper.writeAssemblies( output, fname, core );
  helper.writeLattice( output, "Lattice", core.IsHexType(),
                       core.HexSymmetry, true, core.CoreLattice );
  if( !core.Params.Background.empty() &&
      QFileInfo(core.Params.BackgroundFullPath.c_str()).exists() )
    {
    QFile src(core.Params.BackgroundFullPath.c_str());
    QFile dest( QFileInfo(info.dir(), core.Params.Background.c_str()).absoluteFilePath() );
    if(!dest.exists() || dest.remove())
      {
      src.copy(dest.fileName());
      }
    output << "Background " << core.Params.Background << "\n";
    }
  else if( !core.Params.Background.empty() )
    {
    QMessageBox msgBox;
    msgBox.setText( QString(core.Params.Background.c_str()) +
                   QString(" was not found.  We are not writing Background to output inp file."));
    msgBox.exec();
    }

#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, MSG) \
  if( core.Params.Var##IsSet() ) \
    {\
    helper.write(output, #Key, core.IsHexType(), MSG, core.Params.Var); \
    }
#define FUN_STRUCT(TYPE,X,Var,Key,DEFAULT, MSG) FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, MSG)

  EXTRA_VARABLE_MACRO()
#undef FUN_SIMPLE
#undef FUN_STRUCT

  output << "outputfilename "
         << core.h5mFile << "\n";

  helper.writeUnknown(output, core.Params.UnknownKeyWords);

  output << "End\n";
  output.close();

  core.setAndTestDiffFromFiles(false);

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
  QSet<cmbNucMaterial*> materials = assembly.getMaterials();
  output << "Materials " << materials.count();
  foreach( QPointer<cmbNucMaterial> mat, materials)
  {
    output << " " << mat->getName().toStdString() << ' ' << mat->getLabel().toStdString();
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
    std::transform(mname.begin(), mname.end(), mname.begin(), ::tolower);
    input >> mlabel;

    if(!matColorMap->nameUsed(mname.c_str()))
      {
      if(matColorMap->labelUsed(mlabel.c_str())) //label is being used. Relabel
      {
        QPointer<cmbNucMaterial> mat = matColorMap->getMaterialByLabel(mlabel.c_str());
        if(matColorMap->labelUsed(mat->getName()))
          {
          mat->setLabel("__RELABELED_BY_"+QString(mname.c_str())+"__"
                        +mat->getLabel()+"__RELABELED__");
          }
        else
          {
          mat->setLabel(mat->getName());
          }
      }
      matColorMap->AddMaterial(mname.c_str(), mlabel.c_str());
      }
    else
      {
      // replace the label
      QPointer<cmbNucMaterial> mat = matColorMap->getMaterialByName(mname.c_str());
      if(mat->getLabel() != QString(mlabel.c_str()))
        {
        if(matColorMap->labelUsed(mlabel.c_str())) //label is being used. Relabel
          {
            QPointer<cmbNucMaterial> other = matColorMap->getMaterialByLabel(mlabel.c_str());
            if(matColorMap->labelUsed(other->getName()))
            {
              other->setLabel("__RELABELED_BY_"+QString(mname.c_str())+"__"
                            +other->getLabel()+"__RELABELED__");
            }
            else
            {
              other->setLabel(other->getName());
            }
          }
        mat->setLabel(mlabel.c_str());
        }
      }
    }
}

void inpFileHelper::writeDuct( std::ofstream &output, cmbNucAssembly & assembly )
{
  for(size_t i = 0; i < assembly.AssyDuct.numberOfDucts(); i++)
  {
    Duct *duct = assembly.AssyDuct.getDuct(i);

    output << "duct " << duct->NumberOfLayers() << " ";
    output << std::showpoint << duct->x << " " << duct->y << " " << duct->z1 << " " << duct->z2;
    for(int i = 0; i <  duct->NumberOfLayers(); i++)
      {
      output << " " << duct->GetLayerThick(i, 0);
      if(!assembly.IsHexType())
        {
          output << " " << duct->GetLayerThick(i, 1);
        }
      }
    for(size_t j = 0; j < duct->NumberOfLayers(); j++)
      {
      output << " " << duct->getMaterial(j)->getLabel().toStdString();
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

  duct->SetNumberOfLayers(materials);
  double maxV[] = {0,0};
  for(int i = 0; i < materials; i++)
  {
    double tmpD[2];
    if(assembly.IsHexType())
    {
      input >> tmpD[0];
      tmpD[1] = tmpD[0];
    }
    else
    {
      input >> tmpD[0] >> tmpD[1];
    }
    if(tmpD[0]> maxV[0]) maxV[0] = tmpD[0];
    if(tmpD[1]> maxV[1]) maxV[1] = tmpD[1];

    duct->getNormThick(i)[0] = tmpD[0];
    duct->getNormThick(i)[1] = tmpD[1];
  }
  if(assembly.IsHexType())
    {
    duct->SetType(CMBNUC_ASSY_HEX_DUCT);
    }

  duct->thickness[0] = maxV[0];
  duct->thickness[1] = maxV[1];

  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();

  for(int i = 0; i < materials; i++)
    {
    input >> mlabel;
    duct->setMaterial(i, matColorMap->getMaterialByLabel(mlabel.c_str()));
    duct->getNormThick(i)[0] /= maxV[0];
    duct->getNormThick(i)[1] /= maxV[1];
    }

  assembly.AssyDuct.AddDuct(duct);
}

void inpFileHelper::writePincell( std::ofstream &output, cmbNucAssembly & assembly )
{
  output << "pincells " << assembly.PinCells.size() << "\n";
  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();

  for(size_t i = 0; i < assembly.PinCells.size(); i++)
    {
    PinCell* pincell = assembly.PinCells[i];

    // count of attribute lines for the pincell. equal to the number
    // of frustums plus cylinders plus one for the pitch.
    // We are writing multiple cylinders/frustums on one line.
    size_t count = (pincell->NumberOfCylinders()>0 ? 1: 0) +
                   (pincell->NumberOfFrustums()>0 ? 1 : 0) + 1;

    output << pincell->name << " " << pincell->label << " " << count << "\n";

    output << "pitch " << pincell->pitchX;
    if(!assembly.IsHexType())
      {
      output << " " << pincell->pitchY << " " << pincell->pitchZ;
      }
    else
      {
      output << " " << pincell->pitchZ;
      }
    output << "\n";

    for(size_t j = 0; j < pincell->NumberOfCylinders(); j++)
      {
      if(j==0) output << "cylinder " << pincell->GetNumberOfLayers() << " ";
      Cylinder* cylinder = pincell->GetCylinder(j);
      output << std::showpoint
             << cylinder->x << " "
             << cylinder->y << " "
             << cylinder->z1 << " "
             << cylinder->z2 << " ";
      for(int material = 0; material < cylinder->GetNumberOfLayers(); material++)
        {
        output << std::showpoint << pincell->radii[material]*cylinder->r << " ";
        }
      for(int material = 0; material < cylinder->GetNumberOfLayers(); material++)
        {
        output << cylinder->GetMaterial(material)->getLabel().toStdString() << " ";
        }
      if(j==pincell->NumberOfCylinders()-1) output << "\n";
      }

    for(size_t j = 0; j < pincell->NumberOfFrustums(); j++)
      {
      if(j==0) output << "frustum " << pincell->GetNumberOfLayers() << " ";
      Frustum* frustum = pincell->GetFrustum(j);
      output << std::showpoint
             << frustum->x << " "
             << frustum->y << " "
             << frustum->z1 << " "
             << frustum->z2 << " ";
      double r1 = frustum->r1;
      double r2 = frustum->r2;
      for(int atr = 0; atr < frustum->GetNumberOfLayers(); atr+=2)
        {
        output << std::showpoint << pincell->radii[atr*2+0]*r2 << " ";
        output << std::showpoint << pincell->radii[atr*2+1]*r1 << " ";
        }
      for(int material = 0; material < frustum->GetNumberOfLayers(); material++)
        {
        output << frustum->GetMaterial(material)->getLabel().toStdString() << " ";
        }
      if(j==pincell->NumberOfFrustums()-1) output << "\n";
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

  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();

  for(int i = 0; i < count; i++)
    {
    int lc = i % 10; // Pick a default pin color (for now!)
    PinCell* pincell = new PinCell();
    QPointer<cmbNucMaterial> firstMaterial = NULL;
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
          input >> dHexPinPitch >> pincell->pitchZ;
          pincell->pitchX = pincell->pitchY = dHexPinPitch;
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
          cylinder->SetNumberOfLayers(layers);
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
        pincell->AddCylinder(cylinder);

        // let alpha be the normalization factor for the layers (outer most would be 1.0)
        double alpha = 1.0 / cylinder->r;
        for(int c=0; c < layers; c++)
          {
          // Normalize the layer
          pincell->radii[c] *= alpha;

          // Get the material of the layer - note that we read in the material label that
          // maps to the actual material
          input >> mlabel;
          QPointer<cmbNucMaterial> tmp = matColorMap->getMaterialByLabel(mlabel.c_str());
          qDebug() << tmp->getName() << tmp->getLabel() << mlabel.c_str();
          // Lets save the first material to use to set the pin's color legend
          if (firstMaterial == NULL)
            {
            firstMaterial = tmp;
            }
          cylinder->SetMaterial(c,tmp);
          }
        }
      else if(value == "cellmaterial")
        {
        double tmp;
        double v;
        std::string material;
        input >> tmp >> v >> material;
        }
      else if(value == "frustum")
        {
        int layers;
        input >> layers;
        Frustum* frustum = new Frustum();
        if(layers > pincell->GetNumberOfLayers())
          {
          frustum->SetNumberOfLayers(layers);
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
        pincell->AddFrustum(frustum);

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
          QPointer<cmbNucMaterial> tmp = matColorMap->getMaterialByLabel(mlabel.c_str());
          // Lets save the first material to use to set the pin's color legend
          if (firstMaterial == NULL)
            {
            firstMaterial = tmp;
            }
          frustum->SetMaterial(c,tmp);
          }

        // normalize radii
        }
      }
    cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
    if(firstMaterial != NULL)
      {
      pincell->SetLegendColor(firstMaterial->getColor());
      }
    assembly.AddPinCell(pincell);
    }
}

void inpFileHelper::writeLattice( std::ofstream &output, std::string key, bool isHex,
                                 int hexSymmetry, bool useAmp, Lattice &lat )
{
  output << key;
  if(!isHex)
    {
    output  << " " << lat.Grid[0].size();
    output << " " << lat.Grid.size();
    }
  else
    {
      output << " " << lat.Grid.size();
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
        if(i < hexArray.size()-1 && useAmp)
          output << "&";
        output << "\n";
        }
      }
    else if(hexSymmetry == 6 || hexSymmetry == 12)
      {
      std::cout << "TODO" << std::endl;
      }
    }
  else
    {
    for(size_t i = 0; i < lat.Grid.size(); i++)
      {
      const size_t sizeati = lat.Grid[i].size();
      size_t ati = lat.Grid.size() - i - 1;
      for(size_t j = 0; j < sizeati; j++)
        {
        std::string label = lat.Grid[ati][j].label;
        if(label.empty())
          {
          label = "xx";
          }
        output << label << " ";
        }
      if(useAmp && i < lat.Grid.size()-1)
        {
        output << "&";
        }
      output << "\n";
      }
    }
}


void inpFileHelper::readLattice( std::stringstream & input,
                                 bool isHex, int hexSymmetry,
                                 Lattice &lattice )
{
  size_t cols=0;
  size_t rows=0;
  if(isHex)
    {
    input >> rows;
    cols = rows;
    }
  else
    {
    // the lattice 2d grid use y as rows, x as columns
    input >> cols >> rows;
    }

  lattice.SetDimensions(rows, cols);

  if(isHex)
    {
    size_t x = rows;
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
    for(size_t i = 0; i < rows; i++)
      {
      size_t ati = rows-i-1;
      for(size_t j = 0; j < cols; j++)
        {
          assert(i < lattice.Grid.size());
          assert(j < lattice.Grid[i].size());
        std::string label;
        input >> label;
        lattice.Grid[ati][j].label = label;
        }
      }
    }
}

void inpFileHelper::readAssemblies( std::stringstream &input,
                                    cmbNucCore &core,
                                    std::string strPath,
                                    bool readAssy )
{
  int count;
  cmbNucAssembly *subAssembly;
  input >> count;
  input >> core.AssyemblyPitchX;
  if(core.IsHexType()) // just one pitch
  {
    core.AssyemblyPitchY = core.AssyemblyPitchX;
  }
  else
  {
    input >> core.AssyemblyPitchY;
  }

  // read in assembly files
  for(int i = 0; i < count; i++)
    {

    std::string assyfilename, assylabel, tmpPath = strPath;
    input >> assyfilename >> assylabel;
    tmpPath.append("/").append(assyfilename);
    QFileInfo tmpInfo(tmpPath.c_str());
    tmpPath = strPath + "/" + tmpInfo.completeBaseName().toStdString() + ".inp";
    QFileInfo assyInfo(tmpPath.c_str());
    if(assyInfo.exists() && readAssy)
      {
      subAssembly = core.loadAssemblyFromFile(tmpPath, assylabel);
      }
    }
}

void inpFileHelper::writeAssemblies( std::ofstream &output,
                                     std::string outFileName,
                                     cmbNucCore &core )
{
  QFileInfo info(outFileName.c_str());
  std::string strPath = info.dir().path().toStdString();
  std::string coreName = info.fileName().toStdString();
  std::vector< cmbNucAssembly* > usedAssemblies = core.GetUsedAssemblies();

  output << "Assemblies " << usedAssemblies.size();
  output << " " << core.AssyemblyPitchX;
  if(!core.IsHexType())
  {
    output << " " << core.AssyemblyPitchY;
  }
  output << "\n";
  for(unsigned int i = 0; i < usedAssemblies.size(); ++i)
    {
    //construct assemply file name
    //Look to see if it already has a fname
    cmbNucAssembly & assembly = *(usedAssemblies[i]);
    std::string assemblyName = assembly.FileName;
    if(assemblyName.empty())
      {
      //construct a name
      assemblyName = "assembly_"+assembly.label;
      if(assemblyName + ".inp" == coreName)
        {
        assemblyName = "assembly_a_"+assembly.label;
        }
      assembly.FileName = strPath+"/"+assemblyName+".inp";
      }
    else
      {
      QFileInfo temp(assemblyName.c_str());
      assemblyName = temp.completeBaseName().toStdString();
      if(temp.dir()  == info.dir())
        {
        assemblyName = temp.completeBaseName().toStdString();
        }
      else
        {
        assemblyName = (temp.dir().path() + "/" + temp.completeBaseName()).toStdString();
        }
      }
    output << assemblyName << ".cub " << assembly.label << "\n";
    }
}
