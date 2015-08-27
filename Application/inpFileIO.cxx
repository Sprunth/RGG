#include "inpFileIO.h"
#include "cmbNucAssembly.h"
#include "cmbNucAssemblyLink.h"
#include "cmbNucCore.h"
#include "cmbNucMaterialColors.h"
#include "cmbNucDefaults.h"
#include "cmbNucPinLibrary.h"
#include "cmbNucDuctLibrary.h"

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
  inpFileHelper() : keepGoing(false), renamePin(false)
  {
  }
  std::map<std::string, QPointer<cmbNucMaterial> > materialLabelMap;
  typedef std::map<std::string, QPointer<cmbNucMaterial> >::iterator map_iter;
  bool labelIsDifferent;
  bool keepGoing;
  bool renamePin;
  cmbNucPinLibrary::AddMode pinAddMode;
  std::vector<std::string> log;
  template <typename TYPE>
  bool readGeometryType( std::stringstream & input,
                         TYPE &v, Lattice &lat)
  {
    std::string in;
    input >> in;
    v.setGeometryLabel(in);
    lat.SetDimensions(0, 0);
    return true;
  }

  bool readLattice( std::stringstream & input, Lattice &lat);
  bool readMaterials( std::stringstream & input, cmbNucAssembly &assembly );
  bool readDuct( std::stringstream & input, bool is_hex, DuctCell * dc );
  bool readPincell( std::stringstream & input, cmbNucAssembly &assembly,
                   cmbNucPinLibrary * pl,
                   std::map<std::string, std::string> & newLabel );
  bool readAssemblies( std::stringstream & input, cmbNucCore &core,
                       std::string strPath, bool readAssemblies );
  template<class TYPE> bool read( std::stringstream & input,
                                  bool /*isHex*/,
                                  std::string /*mesg*/,
                                  TYPE &destination )
  {
    if(input)
    {
      input >> destination;
      return true;
    }
    return false;
  }

  bool readUnknown( std::stringstream &input, std::string value,
                   std::vector<std::string> &unknowns)
  {
    std::string restOfLine;
    std::getline(input, restOfLine);
    unknowns.push_back(value + " "+restOfLine);
    return true;
  }

  void writeHeader( std::ofstream &output, std::string type );
  void writeMaterials( std::ofstream &output,
                       std::vector<cmbNucCore::boundaryLayer*> const& bls,
                       cmbNucAssembly &assembly );
  void writeDuct( std::ofstream &output, cmbNucAssembly &assembly,
                  std::vector<cmbNucCore::boundaryLayer*> const& bls,
                  bool limited = false );
  void writePincell( std::ofstream &output,
                     std::vector<cmbNucCore::boundaryLayer*> const& bls,
                     cmbNucAssembly &assembly );
  void writeLattice( std::ofstream &output, std::string key, bool useAmp,
                     Lattice &lat, std::string forceLabel = "" );
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
bool inpFileHelper
::read<ExtrudedType>( std::stringstream & input,
                        bool /*isHex*/,
                        std::string /*mesg*/,
                        ExtrudedType &extrude )
{
  if(!input) return false;
  input >> extrude.Size >> extrude.Divisions;
  return true;
}

template<>
bool inpFileHelper
::read<NeumannSetTypeVec>( std::stringstream & input,
                             bool /*isHex*/,
                             std::string /*mesg*/,
                             NeumannSetTypeVec &extrude )
{
  if(!input) return false;
  NeumannSetType nst;
  input >> nst.Side >> nst.Id;
  std::getline(input, nst.Equation);
  extrude.push_back(nst);
  return true;
}

template<>
bool inpFileHelper
::read<bool>( std::stringstream & input,
                bool /*isHex*/,
                std::string mesg,
                bool &destination )
{
  if(!input) return false;
  std::string v;
  input >> v;
  std::transform(v.begin(), v.end(), v.begin(), ::tolower);
  destination = v == mesg;
  return true;
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
  keepGoing = false;
  renamePin = false;
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
::read(cmbNucAssembly & assembly, cmbNucPinLibrary * pl, cmbNucDuctLibrary * dl)
{
  if(Type != ASSEMBLY_TYPE)
    return false;
  inpFileHelper helper;
  helper.keepGoing = this->keepGoing;
  helper.renamePin = this->renamePin;
  helper.pinAddMode = this->pinAddMode;
  helper.labelIsDifferent = false;
  assembly.clear();
  assembly.setFileName(FileName);
  std::stringstream input(CleanFile);
  QFileInfo info(FileName.c_str());
  QDir at = info.absoluteDir();
  info = QFileInfo(at, "common.inp");
  if(info.exists())
  {
    inpFileReader defaults;
    defaults.open(info.absoluteFilePath().toStdString());
    defaults.read_defaults(assembly);
  }
  std::map<std::string, std::string> newLabel;
  DuctCell * dc = new DuctCell;
  dc->setName(assembly.getLabel() + "_Duct");
  std::vector<std::string> boundary_layer_materials;

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
    else if(value == "blmaterials") // Currently we ignore boundary layers.
    {                               // Does not fit well with current data
      int count;                    // structures
      input >> count;
      std::string blname;
      double bias;
      int intervals;
      for(int i = 0; i < count; ++i)
      {
        input >> blname >> bias >> intervals;
        boundary_layer_materials.push_back(blname);
      }
    }
    else if(value == "geometrytype")
    {
      if(!helper.readGeometryType(input, assembly,
                                  assembly.getLattice() )) return false;
    }
    else if(value == "materials")
    {
      if(!helper.readMaterials( input, assembly )) return false;
    }
    else if(value == "duct" || value == "dimensions")
    {
      if(!helper.readDuct( input, assembly.IsHexType(), dc )) return false;
    }
    else if(value == "pincells")
    {
      if(!helper.readPincell( input, assembly, pl, newLabel )) return false;
    }
    else if(value == "assembly")
    {
      if(!helper.readLattice( input, assembly.getLattice() )) return false;
    }
    else if(value == "tetmeshsize")
    {
      input >> assembly.GetParameters()->TetMeshSize;
    }
    else if(value == "radialmeshsize")
    {
      input >> assembly.GetParameters()->RadialMeshSize;
    }
    else if(value == "axialmeshsize")
    {
      input >> assembly.GetParameters()->AxialMeshSize;
      std::string tmp;
      std::getline(input, tmp); //some version add extra for
                                //each duct.  for now we just ignore them.
    }
    else if(value == "rotate")
    {
      std::string tmp; double a;
      input >> tmp >> a;
      assembly.addTransform(new cmbNucAssembly::Rotate(tmp, a));
    }
    else if(value == "section")
    {
      std::string tmp, tmp1; double a;
      input >> tmp >> a;
      std::getline(input, tmp1);
      assembly.addTransform(new cmbNucAssembly::Section(tmp, a, tmp1));
    }
    else if(value == "move")
    {
      input >> assembly.GetParameters()->MoveXYZ[0]
            >> assembly.GetParameters()->MoveXYZ[1]
            >> assembly.GetParameters()->MoveXYZ[2];
    }
    else if(value == "hblock")
    {
      std::getline(input, assembly.GetParameters()->HBlock);
    }
    else if(value == "geometry")
    {
      input >> assembly.GetParameters()->Geometry;
    }
    else if(value == "center")
    {
      std::string tmp;
      std::getline(input, tmp);
      if(!tmp.empty())
      {
        assembly.GetParameters()->CenterXYZ = tmp;
      }
    }
    else if(value == "save_exodus")
    {
      std::string tmp;
      std::getline(input, tmp);
      assembly.GetParameters()->Save_Exodus = true;
      if(!tmp.empty() && (tmp == "off" || tmp == "no"))
      {
        assembly.GetParameters()->Save_Exodus = false;
      }
    }
#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, MSG)         \
    else if(value == #Key)                              \
    {                                                   \
      if(!helper.read(input, assembly.IsHexType(), MSG, \
                      assembly.GetParameters()->Var))   \
        return false;                                   \
    }
    ASSYGEN_EXTRA_VARABLE_MACRO()
#undef FUN_SIMPLE
    else
    {
      if(!helper.readUnknown(input, value,
                             assembly.GetParameters()->UnknownParams))
      {
        return false;
      }
    }
  }
  DuctCell * dcp = dc;
  dl->addDuct(&dc);
  if(dcp != dc)
  {
    log.push_back("Duct " + (dc)->getName() + " matches current duct");
  }
  assembly.setDuctCell(dc);
  assembly.computeDefaults();
  assembly.setAndTestDiffFromFiles(helper.labelIsDifferent);
  if(!newLabel.empty())
  {
    for(std::map<std::string,std::string>::const_iterator iter = newLabel.begin();
        iter != newLabel.end(); ++iter)
    {
      assembly.getLattice().replaceLabel(iter->first, iter->second);
    }
  }
  this->keepGoing  = helper.keepGoing;
  this->renamePin  = helper.renamePin;
  this->pinAddMode = helper.pinAddMode;
  log.insert(log.end(), helper.log.begin(), helper.log.end());
  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();

  //clean up boundary layer
  for( unsigned int i = 0; i < boundary_layer_materials.size(); i++)
  {
    std::string bl_name = boundary_layer_materials[i];
    pl->removeFakeBoundaryLayer(bl_name);
    dl->removeFakeBoundaryLayer(bl_name);
    matColorMap->RemoveMaterialByLabel(bl_name.c_str());
  }

  return dc->getDuct(0) != NULL;
}

bool inpFileReader
::read(cmbNucCore & core, bool read_assemblies)
{
  if(Type != CORE_TYPE)
    return false;
  QFileInfo info(FileName.c_str());
  std::string strPath = info.dir().path().toStdString();

  inpFileHelper helper;
  core.setExportFileName(FileName);
  core.setMeshOutputFilename((info.completeBaseName() + ".h5m").toStdString());
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
      helper.readGeometryType( input, core, core.getLattice() );
    }
    else if(value == "symmetry")
    {
      int sym;
      input >> sym;
      core.setHexSymmetry(sym);
    }
    else if(value == "assemblies")
    {
      if(!helper.readAssemblies( input, core, strPath, read_assemblies )) return false;
    }
    else if(value == "lattice")
    {
      if(!helper.readLattice( input, core.getLattice() )) return false;
    }
    else if(value == "background")
    {
      getline(input, core.getParams().Background);
      core.getParams().Background = QString(core.getParams().Background.c_str()).trimmed().toStdString();
      //check to make sure the file exists.
      QFileInfo tmpFI( QDir(strPath.c_str()),
                       core.getParams().Background.c_str() );
      if(!tmpFI.exists())
      {
        core.getParams().BackgroundMode = cmbNucCoreParams::None;
        QMessageBox msgBox;
        msgBox.setText( QString(core.getParams().Background.c_str()) +
                        QString(" was not found in same director as the core inp file.  Will be ingored."));
        msgBox.exec();
      }
      else
      {
        core.getParams().BackgroundMode = cmbNucCoreParams::External;
      }
      core.getParams().BackgroundFullPath = tmpFI.absoluteFilePath().toStdString();
    }
    else if(value == "outputfilename")
    {
      std::string tmp_outFile;
      getline(input, tmp_outFile);
      core.setMeshOutputFilename(QString(tmp_outFile.c_str()).trimmed().toStdString());
    }
#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, MSG) \
    else if( value == #Key) \
    {\
        helper.read(input, core.IsHexType(), MSG, core.getParams().Var);\
    }
#define FUN_STRUCT(TYPE,X,Var,Key,DEFAULT, MSG) FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, MSG)
      EXTRA_VARABLE_MACRO()
#undef FUN_SIMPLE
#undef FUN_STRUCT
    else //unknown
    {
      if(!helper.readUnknown(input, value, core.getParams().UnknownKeyWords)) return false;
    }
  }
  core.calculateDefaults();
  QDir at = info.absoluteDir();
  info = QFileInfo(at, "common.inp");
  if(info.exists())
  {
    inpFileReader defaults;
    defaults.open(info.absoluteFilePath().toStdString());
    defaults.read_defaults(*core.GetDefaults());
    core.sendDefaults();
  }
  core.setAndTestDiffFromFiles(false);
  log = helper.log;
  return true;
}

bool inpFileReader::read_defaults(cmbNucAssembly & assembly)
{
  inpFileHelper helper;
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
    else if(value == "axialmeshsize")
    {
      input >> assembly.GetParameters()->AxialMeshSize;
      std::string tmp;
      std::getline(input, tmp); //some version add extra for each duct.  for now we just ignore them.
    }
    #define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, MSG)\
    else if(value == #Key)\
    { \
      helper.read(input, assembly.IsHexType(), MSG, assembly.GetParameters()->Var);\
    }
    ASSYGEN_EXTRA_VARABLE_MACRO()
    #undef FUN_SIMPLE

  }
  return true;
}

bool inpFileReader::read_defaults(cmbNucDefaults & defaults)
{
  inpFileHelper helper;
  std::stringstream input(CleanFile);
  std::string others;
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
    else if(value == "axialmeshsize")
    {
      double ams;
      input >> ams;
      std::string tmp;
      std::getline(input, tmp); //some version add extra for each duct.  for now we just ignore them.
      defaults.setAxialMeshSize(ams);
    }
    else if(value == "edgeinterval")
    {
      double ei;
      input >> ei;
      defaults.setEdgeInterval(ei);
    }
    else if(value == "meshtype")
    {
      std::string tmp;
      input >> tmp;
      defaults.setMeshType(tmp.c_str());
    }
    else
    {
      /*
       geomengine
       startpinid
       meshscheme
       info
       hblock
       geometrytype
       geometry
       createsideset
       createfiles
       mergetolerance
       radialmeshsize
       tetmeshsize  */
      //For now we just ignore the rest.
      std::string tmp;
      std::getline(input, tmp); //some version add extra for each duct.  for now we just ignore them.
      others += value + tmp;
    }
  }
  defaults.setUserDefined(QString(others.c_str()));
  return true;
}

//============================================================================
//Writing
//============================================================================

#define WRITE_PARAM_VALUE(KEY, VALUE)\
if(params->isValueSet(params->VALUE))\
  output << #KEY << " " << params->VALUE << std::endl

bool inpFileWriter::write(std::string fname,
                          cmbNucAssembly & assembly,
                          std::vector<cmbNucCore::boundaryLayer*> const& bls,
                          bool updateFname, bool limited)
{
  inpFileHelper helper;
  std::ofstream output(fname.c_str());
  if(!output.is_open())
  {
    return false;
  }
  if(updateFname && !limited)
  {
    assembly.setFileName(fname);
  }
  helper.writeHeader(output,"Assembly");
  cmbAssyParameters * params = assembly.GetParameters();

  output << "GeometryType " << assembly.getGeometryLabel() << "\n";
  WRITE_PARAM_VALUE(Geometry, Geometry);
  helper.writeMaterials( output, bls, assembly );
  if(!bls.empty()) //TODO: when there are more than one boundary layer
  {
    output << "BLMaterials " << 2;
    output << " " << bls[0]->interface_material->getLabel().toStdString()
           << "_bl" << 1 << ' ' << -bls[0]->Bias << " "
           << bls[0]->Intervals << " "
           << bls[0]->interface_material->getLabel().toStdString()
           << "_bl" << 2 << ' ' << bls[0]->Bias << " "
           << bls[0]->Intervals << "\n";
    /*for(unsigned int i = 0; i < bls.size(); ++i)
    {
      output << " " << bls[i]->interface_material->getLabel().toStdString()
             << "_bl" << i+1 << ' ' << bls[i]->Bias << " "
             << bls[i]->Intervals << "\n";
    }*/
  }
  helper.writeDuct( output, assembly, bls, limited );

  if(!limited)
  {
    helper.writePincell( output, bls, assembly );
    helper.writeLattice( output, "Assembly",
                         false,
                         assembly.getLattice() );
  }
  else
  {
    enumGeometryType type = assembly.getLattice().GetGeometryType();
    output << "Assembly";
    if(type == RECTILINEAR)
    {
      output  << " " << 1;
    }
    output << " " << 1;
    output << std::endl;
    output << "xx\n\n";
  }

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

  for( unsigned int i = 0; i < assembly.getNumberOfTransforms(); ++i)
  {
    assembly.getTransform(i)->write(output) << "\n";
  }
  WRITE_PARAM_VALUE(AxialMeshSize, AxialMeshSize);
  WRITE_PARAM_VALUE(HBlock, HBlock);
#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, DK)\
if(params->isValueSet(params->Var))\
  output << #Key << " " << params->Var << "\n";
  ASSYGEN_EXTRA_VARABLE_MACRO()
#undef FUN_SIMPLE

  for(unsigned int i = 0;
      i < assembly.GetParameters()->UnknownParams.size(); ++i)
  {
    output << assembly.GetParameters()->UnknownParams[i] << "\n";
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
  if(!output)
  {
    return false;
  }
  if(updateFname)
  {
    core.setExportFileName(fname);
  }
  core.computePitch();
  helper.writeHeader(output,"Assembly");
  enumGeometryType type = core.getLattice().GetGeometryType();
  int subType = core.getLattice().GetGeometrySubType();
  if(subType & JUST_ANGLE)
  {
    output << "Symmetry ";
    if(subType & ANGLE_360) output << 1 << "\n";
    else if (subType & ANGLE_60) output << 6 << "\n";
    else if (subType & ANGLE_30) output << 12 << "\n";
    else output << -1 << "\n";
  }
  output << "GeometryType ";
  if(type == RECTILINEAR) output << "Rectangular\n";
  else if(subType & FLAT ) output << "hexflat\n";
  else if(subType & VERTEX) output << "HexVertex\n";
  else output << "ERROR !INVALID TYPE IN SYSTEM\n";
  helper.writeAssemblies( output, fname, core );
  helper.writeLattice( output, "Lattice", true, core.getLattice() );
  if( core.getParams().BackgroundMode == cmbNucCoreParams::External  &&
      core.getParams().BackgroundFullPath.empty())
  {
    QFileInfo tmpFI( QFileInfo(core.getFileName().c_str()).dir(),
                     core.getParams().Background.c_str() );
    core.getParams().BackgroundFullPath = tmpFI.absoluteFilePath().toStdString();
  }
  if( ( ( core.getParams().BackgroundMode == cmbNucCoreParams::External  &&
          QFileInfo(core.getParams().BackgroundFullPath.c_str()).exists() ) ||
          core.getParams().BackgroundMode == cmbNucCoreParams::Generate ) &&
     !core.getParams().Background.empty() )
  {
    QFile src(core.getParams().BackgroundFullPath.c_str());
    QFile dest( QFileInfo(info.dir(),
                          core.getParams().Background.c_str()).absoluteFilePath() );
    if(src.fileName() != dest.fileName()  && (!dest.exists() || dest.remove()))
    {
      src.copy(dest.fileName());
    }
    output << "Background " << core.getParams().Background << "\n";
  }
  else if( core.getParams().BackgroundMode == cmbNucCoreParams::External &&
          !core.getParams().Background.empty() )
  {
    QMessageBox msgBox;
    msgBox.setText( QString(core.getParams().Background.c_str()) +
                   QString(" was not found.  We are not"
                           " writing Background to output inp file."));
    msgBox.exec();
  }
  else if( core.getParams().BackgroundMode == cmbNucCoreParams::Generate &&
           core.getParams().Background.empty() )
  {
    QMessageBox msgBox;
    msgBox.setText(QString("Could not generate a outer jacket"
                           " because no output file name given"));
    msgBox.exec();
  }

#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, MSG) \
  if( core.getParams().Var##IsSet() ) \
    {\
    helper.write(output, #Key, core.IsHexType(), MSG, core.getParams().Var); \
    }
#define FUN_STRUCT(TYPE,X,Var,Key,DEFAULT, MSG) FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, MSG)

  EXTRA_VARABLE_MACRO()
#undef FUN_SIMPLE
#undef FUN_STRUCT

  output << "outputfilename "
         << core.getMeshOutputFilename() << "\n";

  helper.writeUnknown(output, core.getParams().UnknownKeyWords);

  output << "End\n";

  qDebug() << ((output)?"is ok":"Something bad happen");

  output.close();

  //Write Defaults
  QPointer<cmbNucDefaults> defaults = core.GetDefaults();
  if(defaults!=NULL)
  {
    QDir at = info.absoluteDir();
    info = QFileInfo(at, "common.inp");
    std::ofstream outDef(info.absoluteFilePath().toStdString().c_str());
    if(!outDef.is_open())
    {
      return false;
    }
    double vd;
    int vi;
    QString vs;
    if(defaults->getAxialMeshSize(vd))
    {
      outDef << "axialmeshsize " << vd << std::endl;
    }
    if(defaults->getEdgeInterval(vi))
    {
      outDef << "edgeinterval " << vi << std::endl;
    }
    if(defaults->getMeshType(vs))
    {
      outDef << "meshtype " << vs.toStdString() << std::endl;
    }
    QString temp;
    defaults->getUserDefined(temp);
    outDef << temp.toStdString() << "\n";
    outDef << "end\n";
  }

  return true;
}

bool inpFileWriter::writeGSH(std::string fname, cmbNucCore & core,
                             std::string assyName)
{
  inpFileHelper helper;
  std::ofstream output(fname.c_str());
  if(!output.is_open())
  {
    return false;
  }
  helper.writeHeader(output,"Core");
  enumGeometryType type = core.getLattice().GetGeometryType();
  int subType = core.getLattice().GetGeometrySubType();
  if(subType & JUST_ANGLE)
  {
    output << "Symmetry ";
    if(subType & ANGLE_360) output << 1 << "\n";
    else if (subType & ANGLE_60) output << 6 << "\n";
    else if (subType & ANGLE_30) output << 12 << "\n";
    else output << -1 << "\n";
  }
  output << "ProblemType Geometry\n"; //This currently needs to written before geometryType.
  output << "GeometryType ";
  if(type == RECTILINEAR) output << "Rectangular\n";
  else if(subType & FLAT ) output << "hexflat\n";
  else if(subType & VERTEX) output << "HexVertex\n";
  else output << "ERROR !INVALID TYPE IN SYSTEM\n";
  output << "Assemblies " << 1;
  output << " " << core.getAssemblyPitchX();
  if(!core.IsHexType())
  {
    output << " " << core.getAssemblyPitchY();
  }
  output << "\n";
  output << QFileInfo(assyName.c_str()).completeBaseName().toLower().toStdString()
         << ".sat aa" << "\n";
  helper.writeLattice( output, "Lattice", true, core.getLattice(), "aa" );

  output << "outputfilename " +
            QFileInfo(fname.c_str()).completeBaseName().toLower().toStdString()
            + ".sat\n";
  output << "End\n";

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

bool sortByName(const cmbNucMaterial* s1, const cmbNucMaterial* s2)
{
  return s1->getName() < s2->getName();
}

void
inpFileHelper
::writeMaterials( std::ofstream &output,
                  std::vector<cmbNucCore::boundaryLayer*> const& bls,
                  cmbNucAssembly & assembly )
{
  QList<cmbNucMaterial*> materials = assembly.getMaterials().toList();
  qSort(materials.begin(), materials.end(), sortByName);
  output << "Materials " << materials.count() + bls.size() + bls.size();
  foreach( QPointer<cmbNucMaterial> mat, materials)
  {
    output << " " << mat->getName().toStdString() << ' ' << mat->getLabel().toStdString();
  }
  int c = 1;
  for(unsigned int i = 0; i < bls.size(); ++i)
  {
    QPointer<cmbNucMaterial> mat = bls[i]->interface_material;
    output << " " <<mat->getName().toStdString() << "_bl" << c << " "
           << mat->getLabel().toStdString() << "_bl" << c
           << " " <<mat->getName().toStdString() << "_bl" << c+1 << " "
           << mat->getLabel().toStdString() << "_bl" << c+1;
    c+=2;
  }
  output << "\n";
}

bool inpFileHelper::readMaterials( std::stringstream & input,
                                   cmbNucAssembly & /*assembly*/ )
{
  if(!input) return false;
  int countR;
  input >> countR;
  std::string mlabel;

  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  for(int i = 0; i < countR; i++)
    {
    std::string mname;
    input >> mname;
    input >> mlabel;
    QPointer< cmbNucMaterial > mat;

    if(!matColorMap->nameUsed(mname.c_str()))
      {
      QString savedLabel = mlabel.c_str();
      int count = 0;
      while(matColorMap->labelUsed(savedLabel))
        {
        savedLabel = QString(mlabel.c_str()) + QString::number(count++);
        labelIsDifferent = true;
        }
      mat = matColorMap->AddMaterial(mname.c_str(), savedLabel);
      }
    else
      {
      // replace the label
      mat = matColorMap->getMaterialByName(mname.c_str());
      if(mat->getLabel().toLower() != QString(mlabel.c_str()).toLower())
        {
        labelIsDifferent = true;
        }
      }
    std::transform(mlabel.begin(), mlabel.end(), mlabel.begin(), ::tolower);
    std::transform(mname.begin(), mname.end(), mname.begin(), ::tolower);
    materialLabelMap[mlabel] = mat;
    materialLabelMap[mname] = mat;
    }
  return true;
}

static const double sin60cos30 = 0.86602540378443864676372317075294;

void
inpFileHelper
::writeDuct( std::ofstream &output, cmbNucAssembly & assembly,
             std::vector<cmbNucCore::boundaryLayer*> const& bls, bool limited )
{
  for(size_t ad = 0; ad < assembly.getAssyDuct().numberOfDucts(); ad++)
  {
    Duct *duct = assembly.getAssyDuct().getDuct(static_cast<int>(ad));
    int nl = static_cast<int>(duct->NumberOfLayers());

    //TODO handle multiple layers of for boundary
    //TODO handle multiple boundary layer types
    cmbNucCore::boundaryLayer* bl_for_assembly = NULL;
    int numBoundary = 0;
    for(size_t i = 0; i < bls.size() && !limited; ++i)
    {
      if(duct->isInnerDuctMaterial(bls[i]->interface_material))
      {
        bl_for_assembly = bls[i];
        numBoundary = 1;
        break;
      }
    }

    output << "duct " << (limited?1:(nl+numBoundary)) << " ";
    output << std::showpoint << duct->getX() << " " << duct->getY() << " "
           << duct->getZ1() << " " << duct->getZ2();


    for(int i = limited?nl-1:0; i <  nl; i++)
    {
      double thick = 0;
      if(bl_for_assembly != NULL &&
         duct->getMaterial(i) == bl_for_assembly->interface_material)
      {
        double mult = (assembly.IsHexType())?0.5/sin60cos30:0.5;

        thick = bl_for_assembly->Thickness / mult;
      }
      output << " " << duct->GetLayerThick(i, 0) - thick;
      if(!assembly.IsHexType())
      {
          output << " " << duct->GetLayerThick(i, 1) - thick;
      }
      if(bl_for_assembly != NULL &&
         duct->getMaterial(i) == bl_for_assembly->interface_material)
      {
        output << " " << duct->GetLayerThick(i, 0);
        if(!assembly.IsHexType())
        {
          output << " " << duct->GetLayerThick(i, 1);
        }
      }
    }
    for(int j = limited?nl-1:0; j < nl; j++)
    {
      output << " " << duct->getMaterial(j)->getLabel().toStdString();
      if(bl_for_assembly != NULL &&
         duct->getMaterial(j) == bl_for_assembly->interface_material)
      {
        output << " "
               << bl_for_assembly->interface_material->getLabel().toStdString()
               + "_bl2"; //TODO this needs to be reconsidered
      }
    }
    output << "\n";
  }
}

bool
inpFileHelper
::readDuct( std::stringstream & input, bool is_hex, DuctCell * dc )
{
  if(!input) return false;
  Duct* duct = new Duct(0,0,0);
  int materials;
  std::string mlabel;
  double x, y, z1, z2;

  input >> materials >> x >> y >> z1 >> z2;
  duct->setZ1(z1);
  duct->setZ2(z2);
  duct->setX(x);
  duct->setY(y);

  duct->SetNumberOfLayers(materials);
  double maxV[] = {0,0};
  for(int i = 0; i < materials; i++)
  {
    double tmpD[2];
    if(is_hex)
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

  duct->setThickness(0, maxV[0]);
  duct->setThickness(1, maxV[1]);

  for(int i = 0; i < materials; i++)
  {
    input >> mlabel;
    QPointer< cmbNucMaterial > mat =
      cmbNucMaterialColors::instance()->getUnknownMaterial();
    std::transform(mlabel.begin(), mlabel.end(), mlabel.begin(), ::tolower);
    map_iter it = materialLabelMap.find(mlabel);
    if(it != materialLabelMap.end())
    {
      mat = it->second;
    }
    else
    {
      labelIsDifferent = true;
    }
    duct->setMaterial(i, mat);
    duct->getNormThick(i)[0] /= maxV[0];
    duct->getNormThick(i)[1] /= maxV[1];
  }

  dc->AddDuct(duct);
  return true;
}

void
inpFileHelper
::writePincell( std::ofstream &output,
                std::vector<cmbNucCore::boundaryLayer*> const& bls,
                cmbNucAssembly & assembly )
{
  if(assembly.GetNumberOfPinCells() == 0) return;
  output << "pincells " << assembly.GetNumberOfPinCells() << "\n";
  double pitchX = assembly.getPinPitchX();
  double pitchY = assembly.getPinPitchY();
  //TODO: consider different core layers with different coolent
  //TODO: more than one type of boundary layer
  //TODO: boundary layers inside pins

  cmbNucCore::boundaryLayer* bl_for_assembly = NULL;
  for(unsigned int i = 0; i < bls.size(); ++i)
  {
    if(assembly.has_boundary_layer_interface(bls[i]->interface_material))
    {
      bl_for_assembly = bls[i];
      break;
    }
  }

  for(size_t i = 0; i < assembly.GetNumberOfPinCells(); i++)
  {
    PinCell* pincell = assembly.GetPinCell(static_cast<int>(i));

    // count of attribute lines for the pincell. equal to the number
    // of frustums plus cylinders plus one for the pitch.
    // We are writing multiple cylinders/frustums on one line.
    size_t count = pincell->GetNumberOfParts() + 1;
    if(pincell->cellMaterialSet()) count++;

    output << pincell->getName() << " "
           << pincell->getLabel() << " " << count << "\n";

    output << "pitch " << pitchX;
    if(!assembly.IsHexType())
    {
      output << " " << pitchY << " " << 0;
    }
    else
    {
      output << " " << 0;
    }
    output << "\n";

    double minZ = 1e23;
    double maxZ = 0;

    for(size_t j = 0; j < pincell->GetNumberOfParts(); j++)
    {
      PinSubPart* part  = pincell->GetPart(static_cast<int>(j));
      bool iscylinder = part->GetType() == CMBNUC_ASSY_CYLINDER_PIN;
      output << ((iscylinder)?("cylinder "):("frustum "))
             << ( pincell->GetNumberOfLayers() +
                 ((bl_for_assembly != NULL)?1:0) )
             << " ";
      if(minZ > part->getZ1()) minZ = part->getZ1();
      if(maxZ < part->getZ2()) maxZ = part->getZ2();
      output << std::showpoint
             << part->x << " "
             << part->y << " "
             << part->getZ1() << " "
             << part->getZ2() << " ";
      double topR, bottomR;
      for(unsigned int l = 0; l < part->GetNumberOfLayers(); l++)
      {
        bottomR = part->getRadius(l, Frustum::BOTTOM);
        topR = part->getRadius(l, Frustum::TOP);
        output << std::showpoint << bottomR << " ";
        if(!iscylinder)
        {
          output << std::showpoint << topR << " ";
        }
      }
      if(bl_for_assembly != NULL)
      {
        output << std::showpoint << bottomR + bl_for_assembly->Thickness << " ";
        if(!iscylinder)
        {
          output << std::showpoint << topR + bl_for_assembly->Thickness << " ";
        }
      }
      for(unsigned int material = 0;
          material < part->GetNumberOfLayers(); material++)
      {
        output << part->GetMaterial(material)->getLabel().toStdString() << " ";
      }
      if(bl_for_assembly != NULL)
      {
        output << bl_for_assembly->interface_material->getLabel().toStdString()
                  + "_bl1 ";
      }
      output << "\n";
    }

    if(pincell->cellMaterialSet())
      {
        output << "cellmaterial " << minZ << " " << maxZ << " "
               << pincell->getCellMaterial()->getLabel().toStdString() << "\n";
      }
    }
}

bool
inpFileHelper
::readPincell( std::stringstream &input, cmbNucAssembly & assembly,
               cmbNucPinLibrary * pl,
               std::map<std::string, std::string> & newLabel )
{
  if(!input) return false;
  std::string value;
  std::string mlabel;
  int count = 0;
  input >> count;
  // for Hex type, the pitch is next input.
  double hexPicth = -1.0;
  bool pitchSet = false;
  bool material_not_found = false;
  if(assembly.IsHexType())
  {
    std::string hexPicthStr;
    std::getline(input, hexPicthStr);
    remove_if(hexPicthStr.begin(), hexPicthStr.end(), isspace);
    if(!hexPicthStr.empty())
    {
      hexPicth = atof(hexPicthStr.c_str());
      assembly.setPitch(hexPicth, hexPicth, false);
      pitchSet = true;
    }
  }

  for(int i = 0; i < count; i++)
  {
    PinCell* pincell = new PinCell();
    QPointer<cmbNucMaterial> firstMaterial = NULL;
    int attribute_count = 0;
    {
      std::string tmp;
      input >> tmp;
      pincell->setName(tmp);

      input >> tmp;
      pincell->setLabel(tmp);
      input >> attribute_count;
    }

    // initialize for HEX pincell pitch

    for(int j = 0; j < attribute_count; j++)
    {
      input >> value;
      std::transform(value.begin(), value.end(), value.begin(), ::tolower);

      if(value == "pitch")
      {
        // only one field for HEX type
        if(assembly.IsHexType())
        {
          double dHexPinPitch, junk;
          input >> dHexPinPitch >> junk;
          assembly.setPitch(dHexPinPitch, dHexPinPitch, pitchSet);
          pitchSet = true;
        }
        else
        {
          double tx,ty,junk;
          input >> tx >> ty >> junk;
          assembly.setPitch(tx, ty, pitchSet);
          pitchSet = true;
        }
      }
      else if(value == "cylinder")
      {
        int layers;
        input >> layers;
        Cylinder* cylinder = new Cylinder(0,0,0);
        std::vector<double> radii(layers);
        cylinder->SetNumberOfLayers(layers);

        double z1, z2;
        input >> cylinder->x >> cylinder->y >> z1 >> z2;
        cylinder->setZ1(z1);
        cylinder->setZ2(z2);
        for(int c=0; c < layers; c++)
        {
          input >> radii[c];
        }

        cylinder->r = radii.back();
        pincell->AddPart(cylinder);

        // let alpha be the normalization factor for the layers (outer most
        // would be 1.0)
        double alpha = 1.0 / cylinder->r;
        for(int c=0; c < layers; c++)
        {
          // Get the material of the layer - note that we read in the material
          // label that
          // maps to the actual material
          input >> mlabel;
          QPointer< cmbNucMaterial > tmp_mat =
              cmbNucMaterialColors::instance()->getUnknownMaterial();
          std::transform(mlabel.begin(), mlabel.end(), mlabel.begin(), ::tolower);
          map_iter it = materialLabelMap.find(mlabel);
          if(it != materialLabelMap.end())
          {
            tmp_mat = it->second;
          }
          else
          {
            material_not_found = mlabel != tmp_mat->getLabel().toStdString();
            labelIsDifferent = true;
          }
          // Lets save the first material to use to set the pin's color legend
          if (firstMaterial == NULL)
          {
            firstMaterial = tmp_mat;
          }
          cylinder->SetMaterial(c,tmp_mat);
          cylinder->setNormalizedThickness(c, radii[c] * alpha);
        }
      }
      else if(value == "cellmaterial")
      {
        double tmp;
        double v;
        std::string material;
        input >> tmp >> v >> material;
        QPointer< cmbNucMaterial > mat =
            cmbNucMaterialColors::instance()->getUnknownMaterial();
        std::transform(material.begin(), material.end(), material.begin(),
                       ::tolower);
        map_iter it = materialLabelMap.find(material);
        if(it != materialLabelMap.end())
          mat = it->second;
        else
        {
          material_not_found = mlabel != mat->getLabel().toStdString();
          labelIsDifferent = true;
        }
        pincell->setCellMaterial(mat);
      }
      else if(value == "frustum")
      {
        int layers;
        input >> layers;
        double t[] = {0,0};
        Frustum* frustum = new Frustum(t,0,0);
        std::vector<double> radii(layers*2);
        frustum->SetNumberOfLayers(layers);

        double z1, z2;
        input >> frustum->x >> frustum->y >> z1 >> z2;

        frustum->setZ1(z1);
        frustum->setZ2(z2);

        for(int c=0; c < layers; c++)
        {
          input >> radii[c*2+Frustum::TOP];
          input >> radii[c*2+Frustum::BOTTOM];
        }

        frustum->r[Frustum::TOP]    = radii[(2*layers) - Frustum::TOP - 1];
        frustum->r[Frustum::BOTTOM] = radii[(2*layers) - Frustum::BOTTOM - 1];
        pincell->AddPart(frustum);

        // let alpha be the normalization factor for the layers (outer most
        // would be 1.0) for first end of the frustrum
        // let beta be the normalization factor for the layers (outer most would
        // be 1.0) for other end of the frustrum
        double normF[2] = {1.0 / frustum->r[Frustum::TOP],
                           1.0 / frustum->r[Frustum::BOTTOM]};
        for(int c=0; c < layers; c++)
        {
          // Get the material of the layer - note that we read in the material
          // label that maps to the actual material
          std::string mname;
          input >> mlabel;
          QPointer< cmbNucMaterial > tmp =
              cmbNucMaterialColors::instance()->getUnknownMaterial();
          std::transform(mlabel.begin(), mlabel.end(), mlabel.begin(),
                         ::tolower);
          map_iter it = materialLabelMap.find(mlabel);
          if(it != materialLabelMap.end())
          {
            tmp = it->second;
          }
          else
          {
            material_not_found = mlabel != tmp->getLabel().toStdString();
            labelIsDifferent = true;
          }
          frustum->SetMaterial(c,tmp);
          double rtop = radii[2*c+Frustum::TOP]*normF[Frustum::TOP];
          double rbottom = radii[(2*c)+Frustum::BOTTOM]*normF[Frustum::BOTTOM];
          frustum->setNormalizedThickness( c, Frustum::TOP, rtop);
          frustum->setNormalizedThickness( c, Frustum::BOTTOM, rbottom);
        }
      }
    }
    if(material_not_found)
    {
      QMessageBox msgBox;
      msgBox.setText( QString("One or more of the pincell's materials"
                              " were not found, defaulting to "
                              "unknown material") );
      msgBox.exec();
    }
    if(firstMaterial != NULL)
    {
      pincell->SetLegendColor(firstMaterial->getColor());
    }
    std::string old_name = pincell->getName(), old_label = pincell->getLabel();

    switch(pl->addPin(&pincell, false, newLabel))
    {
      case cmbNucPinLibrary::PinAddFailed:
        log.push_back(pincell->getName() + " " + pincell->getLabel() +
                      " Failed to be added to pin library");
        delete pincell;
        pincell = NULL;
        break;
      case cmbNucPinLibrary::PinNull:
        log.push_back("Null Pin");
        break;
      case cmbNucPinLibrary::PinRenamed:
        log.push_back(old_name + " " + old_label +
                      " had a name conflict, renamed to " +
                      pincell->getName() + " " + pincell->getLabel());
        assembly.AddPinCell(pincell);
        break;
      case cmbNucPinLibrary::PinExists:
        if( old_label != pincell->getLabel() || old_name != pincell->getName())
        {
          log.push_back(old_name + " " + old_label +
                        " had a name conflict, renamed to " +
                        pincell->getName() + " " + pincell->getLabel() +
                        ".  Matching pin already exists.");
        }
        else
        {
          log.push_back(old_name + " " + old_label +
                        " A matching pin already exists");
        }
        assembly.AddPinCell(pincell);
        break;
      case cmbNucPinLibrary::PinAdded:
        assembly.AddPinCell(pincell);
        break;
    }
  }
  return true;
}

void inpFileHelper::writeLattice( std::ofstream &output, std::string key,
                                  bool useAmp, Lattice &lat, std::string forceLabel )
{
  enumGeometryType type = lat.GetGeometryType();
  int subType = lat.GetGeometrySubType();
  std::pair<int, int> dim = lat.GetDimensions();
  output << key;
  if(type == RECTILINEAR)
    {
    output  << " " << dim.second;
    }
  output << " " << dim.first;
  output << "\n";
  if(type == HEXAGONAL)
  {
    if(subType & ANGLE_360)
    {
      const size_t x = dim.first;
      size_t maxN = 2*x - 1;
      std::vector<std::vector<std::string> > hexArray;
      hexArray.resize(maxN);
      size_t numCols = 0;
      size_t delta=0;
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
      }

      for(int k = static_cast<int>(x)-1; k >= 0; k--) // HEX layers
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
              std::string label = lat.GetCell(k,static_cast<int>(layerIdx)).label;
              if(!lat.GetCell(k,static_cast<int>(layerIdx)).isBlank() &&
                 !forceLabel.empty())
              {
                label = forceLabel;
              }
              hexArray[i][j] = label;
            }
          }
          else // rows between first and last
          {
            // get the first and last column defined by start column
            layerIdx = 6*k-(i-startRow);
            std::string label =lat.GetCell(k,static_cast<int>(layerIdx)).label;
            if(!lat.GetCell(k,static_cast<int>(layerIdx)).isBlank() && 
               !forceLabel.empty())
            {
              label = forceLabel;
            }
            hexArray[i][startCol] = label;
            layerIdx = k+(i-startRow);
            size_t colIdx = hexArray[i].size() -1 - startCol;
            label =lat.GetCell(k,static_cast<int>(layerIdx)).label;
            if(!lat.GetCell(k,static_cast<int>(layerIdx)).isBlank() && 
               !forceLabel.empty())
            {
              label = forceLabel;
            }
            hexArray[i][colIdx] = label;
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
    else if(subType & (ANGLE_60|ANGLE_30))
    {
      size_t x = lat.getSize();
      std::string tmpVal;
      for(size_t i = 0; i < x; i++)
      {
        size_t start = (subType & FLAT)?(i):(i-(i)/2);
        size_t cols = ((subType & FLAT)?(i+1):(((i+1)-(i+2)%2)))+start;
        if(subType & ANGLE_30)
        {
          start = 2*i - i/2;
          cols = (i%2 ? (i+1)/2 :(i+2)/2) + start;
        }
        const int ti = static_cast<int>(i);
        for( size_t j = start; j < cols; j++)
        {
          const int tj = static_cast<int>(j);
          std::string label = lat.GetCell(ti,tj).label;
          if(label.empty())
          {
            label = "xx";
          }
          if( !lat.GetCell(ti,tj).isBlank() && !forceLabel.empty() )
          {
            label = forceLabel;
          }
          else if((label != "xx" || label != "XX" ) && useAmp) //core
          {
            label = Lattice::generate_string(label, lat.getDrawMode(tj, ti));
          }
          output << label << " ";
          }
        if(i < x-1 && useAmp) output << "&";
        output << "\n";
      }
    }
  }
  else
  {
    for(size_t i = 0; i < lat.getSize(); i++)
    {
      const size_t sizeati = lat.getSize(i);
      size_t ati = lat.getSize() - i - 1;
      for(size_t j = 0; j < sizeati; j++)
      {
        std::string label = lat.GetCell(static_cast<int>(ati), 
                                        static_cast<int>(j)).label;
        if( !lat.GetCell(static_cast<int>(ati), 
                         static_cast<int>(j)).isBlank() && 
            !forceLabel.empty() )
        {
          label = forceLabel;
        }
        if(label.empty())
        {
          label = "xx";
        }
        output << label << " ";
      }
      if(useAmp && i < lat.getSize()-1)
      {
        output << "&";
      }
      output << "\n";
    }
  }
}


bool inpFileHelper::readLattice( std::stringstream & input,
                                 Lattice &lattice )
{
  if(!input) return false;
  enumGeometryType type = lattice.GetGeometryType();
  int subType = lattice.GetGeometrySubType();
  size_t colsR=0;
  size_t rowsR=0;
  if(type == HEXAGONAL)
    {
    input >> rowsR;
    colsR = rowsR;
    }
  else
    {
    // the lattice 2d grid use y as rows, x as columns
    input >> colsR >> rowsR;
    }

  lattice.SetDimensions(static_cast<int>(rowsR), 
                        static_cast<int>(colsR));

  if(type == HEXAGONAL)
    {
    size_t x = rowsR;
    if(subType & ANGLE_360)
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
      for(int k = static_cast<int>(x)-1; k >= 0; k--) // HEX layers
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
              lattice.SetCell(k, static_cast<int>(layerIdx), hexArray[i][j]);
              }
            }
          else // rows between first and last
            {
            // get the first and last column defined by start column
            layerIdx = 6*k-(i-startRow);
            lattice.SetCell(k,static_cast<int>(layerIdx), hexArray[i][startCol]);
            layerIdx = k+(i-startRow);
            size_t colIdx = hexArray[i].size() -1 - startCol;
            lattice.SetCell(k, static_cast<int>(layerIdx), hexArray[i][colIdx]);
            }
          }
        }
      }
    else if(subType & (ANGLE_60|ANGLE_30))
      {
      std::string tmpVal;
      lattice.setInvalidCells();
      for(size_t i = 0; i < x; i++)
        {
        size_t start = (subType & FLAT)?(i):(i-(i)/2);
        size_t cols = ((subType & FLAT)?(i+1):(((i+1)-(i+2)%2)))+start;
        if(subType & ANGLE_30)
          {
          start = 2*i - i/2;
          cols = (i%2 ? (i+1)/2 :(i+2)/2) + start;
          }
        for( size_t j = start; j < cols; j++)
          {
          input >> tmpVal;
          lattice.SetCell(static_cast<int>(i), static_cast<int>(j), tmpVal);
          }
        }
      }
    }
  else
    {
    for(size_t i = 0; i < rowsR; i++)
      {
      size_t ati = rowsR-i-1;
      for(size_t j = 0; j < colsR; j++)
        {
          assert(i < lattice.getSize());
          assert(j < lattice.getSize(i));
        std::string label;
        input >> label;
        lattice.SetCell(static_cast<int>(ati), static_cast<int>(j), label);
        }
      }
    }
  return true;
}

bool inpFileHelper::readAssemblies( std::stringstream &input,
                                    cmbNucCore &core,
                                    std::string strPath,
                                    bool readAssy )
{
  if(!input) return false;
  int count;
  input >> count;
  double apx, apy;
  input >> apy;
  if(core.IsHexType()) // just one pitch
  {
    apx = apy;
  }
  else
  {
    input >> apx;
  }
  core.setAssemblyPitch(apx,apy);

  QString current = QDir::currentPath();
  QDir::setCurrent( strPath.c_str() );

  std::map<std::string, cmbNucAssemblyLink*> assemblyIdToLink;
  std::map<std::string, cmbNucAssembly *> fnameToAssy;

  // read in assembly files
  for(int i = 0; i < count; i++)
  {

    std::string assyfilename, assylabel, assyName, tmpPath = strPath;
    QString assyQString;
    input >> assyfilename >> assylabel;
    // since we don't have a QFileInfo, we just use substr to strip ext
    assyName = assyfilename.substr(0, assyfilename.find_last_of("."));
    assyQString = QString(assyfilename.c_str());
    if(assyQString.endsWith(".cub", Qt::CaseInsensitive))
    {
      assyQString.replace(".cub",".inp");
    }
    else if(assyQString.endsWith(".exo", Qt::CaseInsensitive))
    {
      assyQString.replace(".exo",".inp");
    }
    else
    {
      //do not recognize the file, continue
      continue;
    }
    std::string tmp;
    std::getline(input, tmp);
    tmp = QString(tmp.c_str()).trimmed().toStdString();
    if(tmp.empty())
    {
      QFileInfo assyInfo(assyQString);
      if(!assyInfo.exists())
      {
        //relative path
        assyInfo = QFileInfo(QString(tmpPath.c_str()) + "/" + assyQString);
      }
      if(assyInfo.exists() && readAssy)
      {
        cmbNucAssembly* assembly = new cmbNucAssembly;
        assembly->setCenterPins(false);
        fnameToAssy[assyfilename] = assembly;
        assembly->setLabel(assylabel);
        inpFileReader freader;
        freader.keepGoing = this->keepGoing;
        freader.pinAddMode = this->pinAddMode;
        freader.renamePin = this->renamePin;
        if(!freader.open(assyInfo.absoluteFilePath().toStdString()))
        {
          return false;
        }
        if(!freader.read(*assembly, core.getPinLibrary(),
                         core.getDuctLibrary()))
        {
          return false;
        }
        this->keepGoing = freader.keepGoing;
        this->pinAddMode = freader.pinAddMode;
        this->renamePin = freader.renamePin;
        core.AddAssembly(assembly);
        std::vector< std::string > tlog = freader.getLog();
        for(unsigned int tlat = 0; tlat < tlog.size(); ++tlat)
        {
          log.push_back( assyQString.toStdString() + " " + tlog[tlat] );
        }
      }
    }
    else
    {
      std::stringstream ss(tmp.c_str());
      std::string sameAs, assyfilenameOther;
      std::string msid, nsid;
      ss >> sameAs >> assyfilenameOther >> msid >> nsid;
      std::map<std::string, cmbNucAssembly *>::const_iterator it =
          fnameToAssy.find(assyfilenameOther);
      if(it == fnameToAssy.end())
      {
        cmbNucAssemblyLink * link = new cmbNucAssemblyLink(NULL, msid, nsid);
        link->setLabel(assylabel);
        assemblyIdToLink[assyfilenameOther] = link;
      }
      else
      {
        cmbNucAssemblyLink * link = new cmbNucAssemblyLink(it->second, msid, nsid);
        link->setLabel(assylabel);
        core.AddAssemblyLink(link);
      }
    }
  }
  for(std::map<std::string, cmbNucAssemblyLink*>::const_iterator iter = assemblyIdToLink.begin(); iter != assemblyIdToLink.end(); ++iter)
  {
    std::map<std::string, cmbNucAssembly *>::const_iterator it = fnameToAssy.find(iter->first);
    if(it == fnameToAssy.end())
    {
    }
    else
    {
      cmbNucAssemblyLink * tmp = iter->second;
      tmp->setLink(it->second);
      core.AddAssemblyLink(tmp);
    }
  }
  QDir::setCurrent( current );
  return true;
}

void inpFileHelper::writeAssemblies( std::ofstream &output,
                                     std::string outFileName,
                                     cmbNucCore &core )
{
  QFileInfo info(outFileName.c_str());
  std::string strPath = info.dir().path().toStdString();
  std::string coreName = info.fileName().toStdString();
  typedef std::map< std::string, std::set< Lattice::CellDrawMode > > CellMap;
  typedef std::set< Lattice::CellDrawMode > ModeSet;
  CellMap cells = core.getDrawModesForAssemblies();

  std::vector< cmbNucAssemblyLink* > usedLinks = core.GetUsedLinks();
  std::map< cmbNucAssemblyLink*, Lattice::CellDrawMode > usedLinksForWriteOut;
  std::map< cmbNucAssembly*, std::set<Lattice::CellDrawMode> > orphanedAssembilies;
  std::map< cmbNucAssemblyLink*, int > orphanCounts;

  // Given a vector of links (usedLinks)
  // Create a map between these links and the corresponding Lattice::CellDrawMode
  for (unsigned int i = 0; i < usedLinks.size(); i++)
  {
    cmbNucAssemblyLink * link = usedLinks[i];
    cmbNucAssembly * assembly = link->getLink();

    int orphanCount = 0;

    // Get the label of the assembly the link belongs to (ie 'Assy_1')
    std::string assyPartLabel = link->getLabel();

    // Get the drawmodes for this link
    std::set<Lattice::CellDrawMode> modes;

    for(CellMap::const_iterator cell_iter = cells.begin();
      cell_iter != cells.end(); ++cell_iter)
    {
      // if the cell pair is the one for this link
      if (assyPartLabel.compare(cell_iter->first) == 0)
      {
        // Get the mode corresponding to the matched cell
        modes = cell_iter->second;
        break;
      }
    }
    // todo: If modes unassigned, what do? link w/o modes?

    // loop through cell pairs of the assembly we want to link with
    // and make sure there is one for the same drawmode
    std::string linkTargetMode = assembly->getLabel();
    for(CellMap::const_iterator cell_iter = cells.begin();
      cell_iter != cells.end(); ++cell_iter)
    {
      // if the cell pair is not the one for this link, skip
      if (linkTargetMode.compare(cell_iter->first) != 0)
      {
          continue;
      }
      // cell_iter is the cellpair with our link's target
      // validModes is a set of modes that the "link target" is in
      std::set<Lattice::CellDrawMode> validModes = cell_iter->second;

      // iterate through all the modes the link could be in
      for(ModeSet::const_iterator modes_iter = modes.begin();
          modes_iter != modes.end(); ++modes_iter)
      {
        Lattice::CellDrawMode mode = *modes_iter;

        // regardless of whether we need to create a clone,
        // add to a clone, or make a link, we record this as an orphan
        orphanCount++;

        // search for the mode our link is in
        std::set<Lattice::CellDrawMode>::iterator it = validModes.find(mode);
        if (it == validModes.end())
        {
          //'v2' bug
          // cmbNucInpExporter::exportInpFiles() handles writing the assembly inp file
          // but the core.inp file still needs this v2 entry

          // check and see if an assembly clone is already in orphanedAssembilies
          // if so, instead of creating a new clone, just add the mode to a new form
          cmbNucAssembly* existingClone = NULL;
          for(std::map< cmbNucAssembly*, std::set<Lattice::CellDrawMode> >::iterator orphan_iter = orphanedAssembilies.begin();
              orphan_iter != orphanedAssembilies.end(); ++orphan_iter)
          {
            if (orphan_iter->first->getLabel().compare(assyPartLabel) == 0)
            {
              existingClone = orphan_iter->first;
            }
          }

          if (existingClone == NULL)
          {
            // we need to create a clone since one doesn't exist
            cmbNucAssembly* assy = assembly->clone(assembly->getPinLibrary()->clone(), assembly->getDuctLibrary()->clone());
            std::string tmpLabel = assyPartLabel;
            //tmpLabel = Lattice::generate_string(tmpLabel, mode);
            assy->setLabel(tmpLabel);
            std::string fname = "assembly_" + assy->getLabel() + ".inp";
            std::transform(fname.begin(), fname.end(), fname.begin(), ::tolower);
            assy->setFileName(fname);
            orphanedAssembilies[assy].insert(mode);
          }
          else
          {
            // Just add the mode to the existing clone
            orphanedAssembilies[existingClone].insert(mode);
          }

        }
        else
        {
          // The mode exists, Create/Assign our temp this link
          cmbNucAssemblyLink * tmpLink = new cmbNucAssemblyLink(assembly,
                                                                link->getMaterialStartID(),
                                                                link->getNeumannStartID());
          std::string tmpLabel = assyPartLabel;
          tmpLabel = Lattice::generate_string(tmpLabel, mode);
          tmpLink->setLabel(tmpLabel);

          usedLinksForWriteOut[tmpLink] = mode;
        }
      }
    }

    orphanCounts[link] = orphanCount;
  }

  size_t count = 0;
  for(CellMap::const_iterator iter = cells.begin(); iter != cells.end(); ++iter)
  {
    if(core.GetAssembly(iter->first) != NULL)
    {
      count += iter->second.size();
    }
  }
  for(std::vector<cmbNucAssemblyLink*>::iterator link_iter = usedLinks.begin();
      link_iter != usedLinks.end(); link_iter++)
  {
    // could probably replace this loop with std::accumulate
    count += orphanCounts[*link_iter];
  }

  output << "Assemblies " << count;
  output << " " << core.getAssemblyPitchX();
  if(!core.IsHexType())
  {
    output << " " << core.getAssemblyPitchY();
  }
  output << "\n";
  for(CellMap::const_iterator cell_iter = cells.begin();
      cell_iter != cells.end(); ++cell_iter)
  {
    cmbNucAssembly* assembly = core.GetAssembly(cell_iter->first);
    if(assembly == NULL)
      continue;
    std::set< Lattice::CellDrawMode > const& forms = cell_iter->second;

    for(ModeSet::const_iterator form_iter = forms.begin();
        form_iter != forms.end(); ++form_iter)
    {
      Lattice::CellDrawMode mode = *form_iter;
      std::string assemblyName = assembly->getFileName(mode, forms.size());
      assert(!assemblyName.empty());
      {
        QFileInfo temp(assemblyName.c_str());
        assemblyName = temp.completeBaseName().toStdString();
      }
      output << assemblyName << assembly->getOutputExtension() << " "
             << Lattice::generate_string(assembly->getLabel(), mode) << "\n";
    }
  }
  // Same as above, but for orphaned links that need have become assemblies
  for(std::map< cmbNucAssembly*, std::set<Lattice::CellDrawMode> >::const_iterator iter = orphanedAssembilies.begin();
      iter != orphanedAssembilies.end(); ++iter)
  {
      cmbNucAssembly* assembly = iter->first;
      std::set< Lattice::CellDrawMode > const& forms = iter->second;

      for(ModeSet::const_iterator form_iter = forms.begin();
          form_iter != forms.end(); ++form_iter)
      {
        Lattice::CellDrawMode mode = *form_iter;
        std::string assemblyName = assembly->getFileName(mode, forms.size());
        assert(!assemblyName.empty());
        {
          QFileInfo temp(assemblyName.c_str());
          assemblyName = temp.completeBaseName().toStdString();
        }
        output << assemblyName << assembly->getOutputExtension() << " "
               << Lattice::generate_string(assembly->getLabel(), mode) << "\n";
      }
  }
  for(std::map< cmbNucAssemblyLink*, Lattice::CellDrawMode >::const_iterator iter = usedLinksForWriteOut.begin();
      iter != usedLinksForWriteOut.end(); ++iter)
  {
    cmbNucAssemblyLink * link = iter->first;
    cmbNucAssembly * assembly = link->getLink();
    
    // get the number of modes. todo: use stl to fast find this
    size_t nom = 1; // getFileName default
    for(CellMap::const_iterator cell_iter = cells.begin();
        cell_iter != cells.end(); ++cell_iter)
    {
      // match the assembly's label to the cell label (ie 'Assy_0')
      if (cell_iter->first.compare(assembly->getLabel()) == 0)
      {
        nom = cell_iter->second.size();
        break;
      }
    }

    std::string assemblyName = assembly->getFileName(iter->second, nom);
    QFileInfo temp(assemblyName.c_str());
    assemblyName = temp.completeBaseName().toStdString();

    output << link->getLabel() << assembly->getOutputExtension() << " "
           << link->getLabel() << " same_as "
           << assemblyName << assembly->getOutputExtension() << " "
           << link->getMaterialStartID() << " " << link->getNeumannStartID()
           << "\n";

    delete iter->first;
  } 
  output.flush();
  qDebug() << "wrote assemplies";
}
