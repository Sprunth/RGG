#include "xmlFileIO.h"

#include "cmbNucCore.h"
#include "cmbNucAssembly.h"
#include "cmbNucMaterialColors.h"
#include "cmbNucDuctLibrary.h"
#include "cmbNucPinLibrary.h"
#include "cmbNucDefaults.h"

#define PUGIXML_HEADER_ONLY
#include "src/pugixml.cpp"

#include <QPointer>
#include <QString>
#include <QColor>
#include <QFileInfo>
#include <QMessageBox>
#include <QDir>
#include <QDebug>

//TAGS
namespace
{
  const std::string CORE_TAG = "NuclearCore";
  const std::string MATERIALS_TAG = "Materials";
  const std::string MATERIAL_TAG = "Material";
  const std::string NAME_TAG = "Name";
  const std::string LABEL_TAG = "Label";
  const std::string COLOR_TAG = "Color";
  const std::string DUCT_CELL_TAG = "DuctCell";
  const std::string DUCT_LAYER_TAG = "DuctLayer";
  const std::string LEGEND_COLOR_TAG = "LegendColor";
  const std::string LOC_TAG = "Loc";
  const std::string THICKNESS_TAG = "Thickness";
  const std::string MATERIAL_LAYER_TAG = "MaterialLayer";
  const std::string CYLINDER_TAG = "Cylinder";
  const std::string FRUSTRUM_TAG = "Frustrum";
  const std::string RADIUS_TAG = "Radius";
  const std::string TYPE_TAG = "Type";
  const std::string SUB_TYPE_TAG = "SubType";
  const std::string GRID_TAG = "Grid";
  const std::string DUCT_TAG = "Duct";
  const std::string GEOMETRY_TAG = "Geometry";
  const std::string CENTER_PINS_TAG = "CenterPins";
  const std::string PITCH_TAG = "Pitch";
  const std::string VALUE_TAG = "Value";
  const std::string AXIS_TAG = "Axis";
  const std::string DIRECTION_TAG = "Direction";
  const std::string PARAMETERS_TAG = "Parameters";
  const std::string MOVE_TAG = "Move";
  const std::string CENTER_TAG = "Center";
  const std::string UNKNOWN_TAG = "Unknown";
  const std::string DUCTS_TAG = "Ducts";
  const std::string PINS_TAG = "Pins";
  const std::string DEFAULTS_TAG = "Defaults";
  const std::string PINCELL_TAG = "PinCell";
  const std::string NEUMANN_VALUE_TAG = "NeumannValue";
  const std::string LENGTH_TAG = "Length";
  const std::string Z0_TAG = "Z0";
  const std::string STR_TAG = "Str";
  const std::string LATTICE_TAG = "Lattice";
  const std::string SIDE_TAG = "Side";
  const std::string ID_TAG = "Id";
  const std::string EQUATION_TAG = "Equation";
  const std::string SIZE_TAG = "Size";
  const std::string DIVISIONS_TAG = "Divisions";
  const std::string AXIAL_MESH_SIZE_TAG = "AxialMeshSize";
  const std::string EDGE_INTERVAL_TAG = "EdgeInterval";
  const std::string MESH_TYPE_TAG = "MeshType";
  const std::string ASSEMBLY_TAG = "Assembly";
  const std::string BACKGROUND_TAG = "Background";
  const std::string MODE_TAG = "Mode";
  const std::string CYLINDER_RADIUS_TAG = "CylinderRadius";
  const std::string CYLINDER_OUTER_SPACING_TAG = "CylinderOuterSpacing";
  const std::string BACKGROUND_FILENAME_TAG = "BackgroundFileName";
  const std::string MESH_FILENAME_TAG = "MeshFileName";
  const std::string ROTATE_TAG = "Rotate";
}

class xmlHelperClass
{
public:
  bool openReadFile(std::string fname, pugi::xml_document & document);
  bool writeToString(std::string & out, cmbNucCore & core);
  bool writeStringToFile(std::string fname, std::string & out);

  bool write(pugi::xml_node & node, cmbNucAssembly * assy);

  bool write(pugi::xml_node & materialElement, cmbNucMaterialColors * materials);
  bool write(pugi::xml_node & materialElement, QPointer< cmbNucMaterial > material);

  bool write(pugi::xml_node & node, cmbNucDuctLibrary * dl);
  bool write(pugi::xml_node & node, DuctCell * dc);
  bool write(pugi::xml_node & node, Duct * dc);

  bool write(pugi::xml_node & node, cmbNucPinLibrary * dl);
  bool write(pugi::xml_node & node, PinCell * dc);
  bool write(pugi::xml_node & node, Cylinder * dc);
  bool write(pugi::xml_node & node, Frustum * dc);
  bool writePSP(pugi::xml_node & node, PinSubPart * dc);

  bool write(pugi::xml_node & node, Lattice & lattice);

  bool write(pugi::xml_node & node, cmbNucMaterialLayer const& v);

  bool write(pugi::xml_node & node, std::string attName, std::vector<cmbNucCoreParams::NeumannSetStruct> const&);

  bool write(pugi::xml_node & node, std::string attName, cmbNucCoreParams::ExtrudeStruct const&);

  bool write(pugi::xml_node & node, std::string attName, QString const& v)
  {
    return write(node, attName, v.toStdString());
  }
  bool write(pugi::xml_node & node, std::string attName, std::string const& v)
  {
    node.append_attribute(attName.c_str()).set_value(v.c_str());
    return true;
  }
  bool write(pugi::xml_node & node, std::string attName, QColor const& v)
  {
    QString str = QString("%1, %2, %3, %4").arg(v.redF()).arg(v.greenF()).arg(v.blueF()).arg(v.alphaF());
    return write(node, attName, str);
  }
  bool write(pugi::xml_node & node, std::string attName, double const& v)
  {
    QString str = QString("%1").arg(v, 0, 'g', 9);
    return write(node, attName, str);
  }
  bool write(pugi::xml_node & node, std::string attName, int const& v)
  {
    QString str = QString("%1").arg(v);
    return write(node, attName, str);
  }
  bool write(pugi::xml_node & node, std::string attName, unsigned int const& v)
  {
    QString str = QString("%1").arg(v);
    return write(node, attName, str);
  }
  bool write(pugi::xml_node & node, std::string attName, bool const& v)
  {
    QString str = QString("%1").arg(v);
    return write(node, attName, str);
  }
  bool write(pugi::xml_node & node, std::string attName, double const* v, int size)
  {
    QString str;
    switch(size)
    {
      case 1: return write(node, attName, *v);
      case 2: str = QString("%1, %2").arg(v[0], 0, 'g', 9).arg(v[1], 0, 'g', 9); break;
      case 3: str = QString("%1, %2, %3").arg(v[0]).arg(v[1], 0, 'g', 9).arg(v[2], 0, 'g', 9); break;
      case 4: str = QString("%1, %2, %3, %4").arg(v[0], 0, 'g', 9).arg(v[1], 0, 'g', 9).arg(v[2], 0, 'g', 9).arg(v[3], 0, 'g', 9); break;
    }
    return write(node, attName, str);
  }

  bool read(std::string const& in, cmbNucCore & core);
  bool read(pugi::xml_node & node, cmbNucMaterialColors * materials);
  bool read(pugi::xml_node & node, QString & name, QString & label, QColor & color);

  bool read(pugi::xml_node & node, cmbNucPinLibrary * dl, cmbNucMaterialColors * materials);
  bool read(pugi::xml_node & node, PinCell * dc, cmbNucMaterialColors * materials);
  bool read(pugi::xml_node & node, Cylinder * dc, cmbNucMaterialColors * materials);
  bool read(pugi::xml_node & node, Frustum * dc, cmbNucMaterialColors * materials);
  bool readPSP(pugi::xml_node & node, PinSubPart * dc, cmbNucMaterialColors * materials);

  bool read(pugi::xml_node & node, cmbNucMaterialLayer * ml, cmbNucMaterialColors * materials);

  bool read(pugi::xml_node & node, cmbNucDuctLibrary * dl, cmbNucMaterialColors * materials);
  bool read(pugi::xml_node & node, DuctCell * dc, cmbNucMaterialColors * materials);
  bool read(pugi::xml_node & node, Duct * dc, cmbNucMaterialColors * materials);

  bool read(pugi::xml_node & node, cmbNucAssembly* assy);

  bool read(pugi::xml_node & node, Lattice & lattice, std::map<QString, int> & used);

  bool read(pugi::xml_node & node, std::string attName,
            std::vector<cmbNucCoreParams::NeumannSetStruct> &);

  bool read(pugi::xml_node & node, std::string attName, cmbNucCoreParams::ExtrudeStruct &);

  bool read(pugi::xml_node & node, std::string attName, QColor & v)
  {
    pugi::xml_attribute att = node.attribute(attName.c_str());
    if(!att) return false;
    QString ts(att.value());
    QStringList list1 = ts.split(",");
    v = QColor::fromRgbF(list1.value(0).toDouble(),
                         list1.value(1).toDouble(),
                         list1.value(2).toDouble(),
                         list1.value(3).toDouble());
    return true;
  }

  bool read(pugi::xml_node & node, std::string attName, std::string & v)
  {
    pugi::xml_attribute att = node.attribute(attName.c_str());
    if(!att) return false;
    v = std::string(att.value());
    return true;
  }

  bool read(pugi::xml_node & node, std::string attName, QString & v)
  {
    pugi::xml_attribute att = node.attribute(attName.c_str());
    if(!att) return false;
    v = QString(att.value());
    return true;
  }

  bool read(pugi::xml_node & node, std::string attName, double & v)
  {
    QString str;
    if(!read(node, attName, str)) return false;
    v = str.toDouble();
    return true;
  }

  bool read(pugi::xml_node & node, std::string attName, int & v)
  {
    QString str;
    if(!read(node, attName, str)) return false;
    v = str.toInt();
    return true;
  }

  bool read(pugi::xml_node & node, std::string attName, unsigned int & v)
  {
    QString str;
    if(!read(node, attName, str)) return false;
    v = str.toUInt();
    return true;
  }

  bool read(pugi::xml_node & node, std::string attName, bool & v)
  {
    QString str;
    if(!read(node, attName, str)) return false;
    v = static_cast<bool>(str.toInt());
    return true;
  }

  bool read(pugi::xml_node & node, std::string attName, double * v, int size)
  {
    QString str;
    if(!read(node, attName, str)) return false;
    QStringList list1 = str.split(",");
    for(unsigned int at = 0; at < size; ++at)
    {
      v[at] = list1.value(at).toDouble();
    }
    return true;
  }
};

bool xmlHelperClass::openReadFile(std::string fname, pugi::xml_document & document)
{
  std::ifstream in(fname.c_str(), std::ios::in);
  if (!in)
  {
    return false;
  }

  // Allocate string
  std::string content;
  in.seekg(0, std::ios::end);
  content.resize(in.tellg());

  in.seekg(0, std::ios::beg);
  in.read(&content[0], content.size());
  in.close();

  xmlHelperClass helper;

  pugi::xml_parse_result presult = document.load_buffer(content.c_str(), content.size());
  if (presult.status != pugi::status_ok)
  {
    return false;
  }
  return true;
}

bool xmlHelperClass::write(pugi::xml_node & materialElement,
                           cmbNucMaterialColors * cnmc)
{
  std::vector< QPointer< cmbNucMaterial > > materials = cnmc->getMaterials();
  for(size_t i = 0; i < materials.size(); ++i)
  {
    pugi::xml_node mElement = materialElement.append_child(MATERIAL_TAG.c_str());
    this->write(mElement, materials[i]);
  }
  return true;
}

bool xmlHelperClass::write(pugi::xml_node & materialElement, QPointer< cmbNucMaterial > material)
{
  bool r = true;
  r &= write(materialElement, NAME_TAG.c_str(), material->getName());
  r &= write(materialElement, LABEL_TAG.c_str(), material->getLabel());
  r &= write(materialElement, COLOR_TAG.c_str(), material->getColor());
  return r;
}

bool xmlHelperClass::write(pugi::xml_node & node, cmbNucDuctLibrary * dl)
{
  std::size_t num = dl->GetNumberOfDuctCells();
  bool r = true;
  for(size_t i = 0; i < num; ++i)
  {
    pugi::xml_node xn = node.append_child(DUCT_CELL_TAG.c_str());
    r &= this->write(xn, dl->GetDuctCell(i));
  }
  return r;
}

bool xmlHelperClass::write(pugi::xml_node & node, DuctCell * dc)
{
  bool r = true;
  r &= write(node, NAME_TAG.c_str(), dc->getName());
  size_t num = dc->numberOfDucts();
  for(size_t i = 0; i < num; ++i)
  {
    pugi::xml_node xn = node.append_child(DUCT_LAYER_TAG.c_str());
    r &= this->write(xn, dc->getDuct(i));
  }
  return r;
}

bool xmlHelperClass::write(pugi::xml_node & node, Duct * dc)
{
  bool r = true;
  r &= write(node, LOC_TAG.c_str(), QString("%1, %2, %3, %4").arg(dc->x, 0, 'g', 9).arg(dc->y, 0, 'g', 9).arg(dc->getZ1(), 0, 'g', 9).arg(dc->getZ2(), 0, 'g', 9));
  r &= write(node, THICKNESS_TAG.c_str(), dc->thickness, 2);

  size_t num = dc->NumberOfLayers();
  for(size_t i = 0; i < num; ++i)
  {
    pugi::xml_node xn = node.append_child(MATERIAL_LAYER_TAG.c_str());
    r &= this->write(xn, dc->getMaterialLayer(i));
  }
  return r;
}

bool xmlHelperClass::write(pugi::xml_node & node, cmbNucPinLibrary * dl)
{
  std::size_t num = dl->GetNumberOfPinCells();
  bool r = true;
  for(size_t i = 0; i < num; ++i)
  {
    pugi::xml_node xn = node.append_child(PINCELL_TAG.c_str());
    r &= this->write(xn, dl->GetPinCell(i));
  }
  return r;
}

bool xmlHelperClass::write(pugi::xml_node & node, PinCell * dc)
{
  bool r = true;
  r &= write(node, NAME_TAG.c_str(), dc->getName());
  r &= write(node, LABEL_TAG.c_str(), dc->getLabel());
  r &= write(node, LEGEND_COLOR_TAG.c_str(), dc->GetLegendColor());

  for(unsigned int i = 0; i < dc->GetNumberOfParts(); ++i)
  {
    PinSubPart * part = dc->GetPart(i);
    if(part->GetType() == CMBNUC_ASSY_CYLINDER_PIN)
    {
      pugi::xml_node xn = node.append_child(CYLINDER_TAG.c_str());
      r &= this->write(xn, dynamic_cast<Cylinder*>(part));
    }
    else
    {
      pugi::xml_node xn = node.append_child(FRUSTRUM_TAG.c_str());
      r &= this->write(xn, dynamic_cast<Frustum*>(part));
    }
  }

  return r;
}

bool xmlHelperClass::write(pugi::xml_node & node, Cylinder * c)
{
  bool r = true;
  r &= write(node, RADIUS_TAG.c_str(), c->r);
  r &= writePSP(node, c);
  return r;
}

bool xmlHelperClass::write(pugi::xml_node & node, Frustum * f)
{
  bool r = true;
  r &= write(node, RADIUS_TAG.c_str(), f->r, 2);
  r &= writePSP(node, f);
  return r;
}

bool xmlHelperClass::writePSP(pugi::xml_node & node, PinSubPart * p)
{
  bool r = true;
  r &= write(node, LOC_TAG.c_str(), QString("%1, %2, %3, %4").arg(p->x, 0, 'g', 9).arg(p->y, 0, 'g', 9).arg(p->z1).arg(p->z2, 0, 'g', 9));
  size_t num = p->GetNumberOfLayers();
  for(size_t i = 0; i < num; ++i)
  {
    pugi::xml_node xn = node.append_child(MATERIAL_LAYER_TAG.c_str());
    r &= this->write(xn, p->getMaterialLayer(i));
  }
  return true;
}

bool xmlHelperClass::write(pugi::xml_node & node, cmbNucMaterialLayer const& v)
{
  bool r = true;
  r &= write(node, THICKNESS_TAG.c_str(), v.getThickness(), 2);
  r &= write(node, MATERIAL_TAG.c_str(), v.getMaterial()->getName());
  return r;
}

bool xmlHelperClass::write(pugi::xml_node & node, Lattice & lattice)
{
  bool r = true;
  r &= write(node, TYPE_TAG.c_str(), static_cast<unsigned int>(lattice.GetGeometryType()));
  r &= write(node, SUB_TYPE_TAG.c_str(), lattice.GetGeometrySubType());
  std::string grid;
  for(unsigned int i = 0; i < lattice.getSize(); ++i)
  {
    for(unsigned int j = 0; j < lattice.getSize(i); ++j)
    {
      if(j != 0) grid += ",";
      Lattice::LatticeCell cell = lattice.GetCell(i, j);
      grid += cell.label;
    }
    grid += ";";
  }
  r &= write(node, GRID_TAG.c_str(), grid);
  return r;
}

#define WRITE_PARAM_VALUE(KEY, VALUE)\
  if(params->isValueSet(params->VALUE))\
    r &= write(paramNode, #KEY, params->VALUE);

bool xmlHelperClass::write(pugi::xml_node & node, cmbNucAssembly * assy)
{
  bool r = true;
  r &= write(node, LABEL_TAG.c_str(), assy->getLabel());
  r &= write(node, DUCT_TAG.c_str(), assy->getAssyDuct().getName());
  r &= write(node, GEOMETRY_TAG.c_str(), assy->getGeometryLabel());
  r &= write(node, LEGEND_COLOR_TAG.c_str(), assy->GetLegendColor());
  r &= write(node, CENTER_PINS_TAG.c_str(), assy->isPinsAutoCentered());
  r &= write(node, PITCH_TAG.c_str(), QString("%1, %2").arg(assy->getPinPitchX(), 0, 'g', 9).arg(assy->getPinPitchY(), 0, 'g', 9));
  r &= write(node, ROTATE_TAG.c_str(), assy->getZAxisRotation());



  pugi::xml_node paramNode = node.append_child(PARAMETERS_TAG.c_str());
  {
    cmbAssyParameters * params = assy->GetParameters();

    //Other Parameters
    WRITE_PARAM_VALUE(Geometry, Geometry);
    WRITE_PARAM_VALUE(TetMeshSize, TetMeshSize);
    WRITE_PARAM_VALUE(RadialMeshSize, RadialMeshSize);
    if(params->MoveXYZ[0]!=0 || params->MoveXYZ[1]!=0 || params->MoveXYZ[2]!=0)
    {
      r &= write(paramNode, MOVE_TAG.c_str(), params->MoveXYZ, 3);
    }
    r &= write(paramNode, CENTER_TAG.c_str(), params->CenterXYZ);

    WRITE_PARAM_VALUE(AxialMeshSize, AxialMeshSize);
    WRITE_PARAM_VALUE(HBlock, HBlock);
#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, DK)\
    if(params->isValueSet(params->Var))\
    {\
      r &= write(paramNode, #Key, params->Var);\
    }
    ASSYGEN_EXTRA_VARABLE_MACRO()
#undef FUN_SIMPLE

    for(unsigned int i = 0; i < params->UnknownParams.size(); ++i)
    {
      pugi::xml_node tn = node.append_child(UNKNOWN_TAG.c_str());
      r &= write(tn, STR_TAG.c_str(), params->UnknownParams[i]);
    }

  }

  pugi::xml_node lnode = node.append_child(LATTICE_TAG.c_str());
  r &= write( lnode, assy->getLattice());

  return r;
}
#undef WRITE_PARAM_VALUE

bool xmlHelperClass::write(pugi::xml_node & node, std::string attName,
                           std::vector<cmbNucCoreParams::NeumannSetStruct> const& nssv)
{
  bool r = true;
  pugi::xml_node ttnode = node.append_child(attName.c_str());
  for(size_t i = 0; i < nssv.size(); ++i)
  {
    pugi::xml_node nssnode = ttnode.append_child(NEUMANN_VALUE_TAG.c_str());
    cmbNucCoreParams::NeumannSetStruct const& nss = nssv[i];
    r &= write(nssnode, SIDE_TAG.c_str(), nss.Side);
    r &= write(nssnode, ID_TAG.c_str(), nss.Id);
    r &= write(nssnode, EQUATION_TAG.c_str(), nss.Equation);
  }
  return r;
}

bool xmlHelperClass::write(pugi::xml_node & node, std::string attName,
                           cmbNucCoreParams::ExtrudeStruct const& es)
{
  bool r = true;
  pugi::xml_node ttnode = node.append_child(attName.c_str());
  r &= write(ttnode, SIZE_TAG.c_str(), es.Size);
  r &= write(ttnode, DIVISIONS_TAG.c_str(), es.Divisions);
  return r;
}

bool xmlHelperClass::writeToString(std::string & out, cmbNucCore & core)
{
  pugi::xml_document document;
  pugi::xml_node rootElement = document.append_child(CORE_TAG.c_str());
  pugi::xml_node mnode = rootElement.append_child(MATERIALS_TAG.c_str());
  if(!write(mnode, cmbNucMaterialColors::instance())) return false;
  pugi::xml_node dnode = rootElement.append_child(DUCTS_TAG.c_str());
  if(!write(dnode, core.getDuctLibrary())) return false;

  pugi::xml_node pnode = rootElement.append_child(PINS_TAG.c_str());
  if(!write(pnode, core.getPinLibrary())) return false;

  //Write defaults
  {
    QPointer<cmbNucDefaults> defaults = core.GetDefaults();
    pugi::xml_node node = rootElement.append_child(DEFAULTS_TAG.c_str());

    double DuctThick[2];
    double length, z0;
    defaults->getHeight(length);
    defaults->getDuctThickness(DuctThick[0], DuctThick[1]);
    defaults->getZ0(z0);

    if(!write(node, LENGTH_TAG.c_str(), length)) return false;
    if(!write(node, Z0_TAG.c_str(), z0)) return false;
    if(!write(node, THICKNESS_TAG.c_str(), DuctThick, 2)) return false;

    double vd;
    int vi;
    QString vs;
    if(defaults->getAxialMeshSize(vd))
    {
      if(!write( node, AXIAL_MESH_SIZE_TAG.c_str(), vd)) return false;
    }
    if(defaults->getEdgeInterval(vi))
    {
      if(!write( node, EDGE_INTERVAL_TAG.c_str(), vi)) return false;
    }
    if(defaults->getMeshType(vs))
    {
      if(!write( node, MESH_TYPE_TAG.c_str(), vs)) return false;
    }
  }

  int num = core.GetNumberOfAssemblies();

  for(int i = 0; i < num; ++i)
  {
    cmbNucAssembly* assy = core.GetAssembly(i);
    pugi::xml_node assyNode = rootElement.append_child(ASSEMBLY_TAG.c_str());
    if(!write(assyNode, assy)) return false;
  }

  //write the background
  {
    pugi::xml_node node = rootElement.append_child(BACKGROUND_TAG.c_str());
    if(!write(node, MODE_TAG.c_str(), static_cast<unsigned int>(core.Params.BackgroundMode))) return false;
    if(!write(node, CYLINDER_RADIUS_TAG.c_str(), core.getCylinderRadius())) return false;
    if(!write(node, CYLINDER_OUTER_SPACING_TAG.c_str(), core.getCylinderOuterSpacing())) return false;
    if(!write(node, BACKGROUND_FILENAME_TAG.c_str(), core.Params.Background)) return false;
  }

  //Write parameters
  {
    pugi::xml_node node = rootElement.append_child(PARAMETERS_TAG.c_str());
    if(!write(node, MESH_FILENAME_TAG.c_str(), core.h5mFile)) return false;
#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, MSG) \
    if( core.Params.Var##IsSet() ) \
    {\
      if(!write(node, #Key, core.Params.Var)) return false; \
    }
#define FUN_STRUCT(TYPE,X,Var,Key,DEFAULT, MSG) FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, MSG)

    EXTRA_VARABLE_MACRO()
#undef FUN_SIMPLE
#undef FUN_STRUCT
    for(unsigned int i = 0; i < core.Params.UnknownKeyWords.size(); ++i)
    {
      pugi::xml_node tn = node.append_child(UNKNOWN_TAG.c_str());
      if(!write(tn, STR_TAG.c_str(), core.Params.UnknownKeyWords[i])) return false;
    }
  }

  pugi::xml_node lnode = rootElement.append_child(LATTICE_TAG.c_str());
  if(!write(lnode, core.getLattice())) return false;

  std::stringstream oss;
  unsigned int flags = pugi::format_indent;
  document.save(oss, "  ", flags);
  out = oss.str();

  return true;
}

bool xmlHelperClass::writeStringToFile(std::string fname, std::string & out)
{
  std::ofstream outfile;
  outfile.open(fname.c_str());
  if(!outfile) return false;
  outfile << out;
  outfile.close();
  return true;
}

///////////////////////////////////Read//////////////////////////////////////////////////////
bool xmlHelperClass::read(std::string const& in, cmbNucCore & core)
{
  pugi::xml_document document;
  pugi::xml_parse_result presult = document.load_buffer(in.c_str(), in.size());
  if (presult.status != pugi::status_ok)
  {
    return false;
  }
  pugi::xml_node rootElement = document.child(CORE_TAG.c_str());

  //Read material
  {
    pugi::xml_node node = rootElement.child(MATERIALS_TAG.c_str());
    if(!read(node, cmbNucMaterialColors::instance())) return false;
  }
  {
    pugi::xml_node node = rootElement.child(DUCTS_TAG.c_str());
    if(!read(node, core.getDuctLibrary(), cmbNucMaterialColors::instance())) return false;
  }
  {
    pugi::xml_node node = rootElement.child(PINS_TAG.c_str());
    if(!read(node, core.getPinLibrary(), cmbNucMaterialColors::instance())) return false;
  }

  {
    pugi::xml_node node = rootElement.child(BACKGROUND_TAG.c_str());
    unsigned int mode;
    if(!read(node, MODE_TAG.c_str(), mode)) return false;
    double r;
    if(!read(node, CYLINDER_RADIUS_TAG.c_str(), r)) return false;
    int s;
    if(!read(node, CYLINDER_OUTER_SPACING_TAG.c_str(), s)) return false;
    if(!read(node, BACKGROUND_FILENAME_TAG.c_str(), core.Params.Background)) return false;
    switch( mode )
    {
      case 0:
        core.Params.BackgroundMode = cmbNucCoreParams::None;
        core.Params.Background = "";
        break;
      case 1:
      {
        core.Params.BackgroundMode = cmbNucCoreParams::External;
        //check to make sure the file exists.
        QFileInfo tmpFI( QFileInfo(core.CurrentFileName.c_str()).dir(),
                         core.Params.Background.c_str() );
        //qDebug() << tmpFI;
        if(!tmpFI.exists())
        {
          core.Params.BackgroundMode = cmbNucCoreParams::None;
          QMessageBox msgBox;
          msgBox.setText( QString(core.Params.Background.c_str()) +
                         QString(" was not found in same director as the core inp file.  Will be ingored."));
          msgBox.exec();
        }
        core.Params.BackgroundFullPath = tmpFI.absoluteFilePath().toStdString();
        break;
      }
      case 2:
        core.Params.BackgroundMode = cmbNucCoreParams::Generate;
        core.drawCylinder(r, s);
    }
  }

  //read defaults
  {
    core.initDefaults();
    QPointer<cmbNucDefaults> defaults = core.GetDefaults();
    pugi::xml_node node = rootElement.child(DEFAULTS_TAG.c_str());

    double DuctThick[2];
    double length;
    double z0;

    if(!read(node, LENGTH_TAG.c_str(), length)) return false;
    if(!read(node, Z0_TAG.c_str(),z0))
    {
      z0 = 0;
    }
    if(!read(node, THICKNESS_TAG.c_str(), DuctThick, 2)) return false;

    defaults->setHeight(length);
    defaults->setDuctThickness(DuctThick[0], DuctThick[1]);
    defaults->setZ0(z0);

    double vd;
    int vi;
    QString vs;
    if(read(node, AXIAL_MESH_SIZE_TAG.c_str(), vd))
    {
      defaults->setAxialMeshSize(vd);
    }
    if(read(node, EDGE_INTERVAL_TAG.c_str(), vi))
    {
      defaults->setEdgeInterval(vi);
    }
    if(read( node, MESH_TYPE_TAG.c_str(), vs))
    {
      defaults->setMeshType(vs);
    }
    core.sendDefaults();
  }

  //read assemblies
  for(pugi::xml_node tnode = rootElement.child(ASSEMBLY_TAG.c_str()); tnode;
      tnode = tnode.next_sibling(ASSEMBLY_TAG.c_str()))
  {
    cmbNucAssembly* assy = new cmbNucAssembly;
    core.AddAssembly(assy);
    if(!read(tnode, assy)) return false;
  }

  //Read parameters
  {
    pugi::xml_node node = rootElement.child(PARAMETERS_TAG.c_str());
    read(node, MESH_FILENAME_TAG.c_str(), core.h5mFile);
#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, MSG) \
    read(node, #Key, core.Params.Var);
#define FUN_STRUCT(TYPE,X,Var,Key,DEFAULT, MSG) FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, MSG)
    EXTRA_VARABLE_MACRO()
#undef FUN_SIMPLE
#undef FUN_STRUCT
    for(pugi::xml_node tnode = node.child(UNKNOWN_TAG.c_str()); tnode;
        tnode = tnode.next_sibling(UNKNOWN_TAG.c_str()))
    {
      std::string tmp;
      if(read(tnode, STR_TAG.c_str(), tmp))
        core.Params.UnknownKeyWords.push_back(tmp);
    }
  }

  pugi::xml_node lnode = rootElement.child(LATTICE_TAG.c_str());
  std::map<QString, int> used;
  if(!read(lnode, core.getLattice(), used)) return false;
  core.setUsedLabels(used);

  if(core.IsHexType())
  {
    core.getLattice().setFullCellMode(Lattice::HEX_FULL);
    for(unsigned int i = 0; i < core.GetNumberOfAssemblies(); ++i)
    {
      if( core.getLattice().GetGeometrySubType() & ANGLE_60 &&
          core.getLattice().GetGeometrySubType() & VERTEX )
      {
        core.GetAssembly(i)->getLattice().setFullCellMode(Lattice::HEX_FULL);
      }
      else
      {
        core.GetAssembly(i)->getLattice().setFullCellMode(Lattice::HEX_FULL_30);
      }
    }
  }

  return true;
}

bool xmlHelperClass::read(pugi::xml_node & node, cmbNucMaterialColors * materials)
{
  if(materials == NULL) return false;
  QString name, label;
  QColor color;
  for (pugi::xml_node material = node.child(MATERIAL_TAG.c_str()); material;
       material = material.next_sibling(MATERIAL_TAG.c_str()))
  {
    if(!this->read(material, name, label, color)) return false;
    materials->AddOrUpdateMaterial(name, label, color);
  }
  return true;
}

bool xmlHelperClass::read(pugi::xml_node & node, QString & name, QString & label, QColor & color)
{
  if(!read(node, COLOR_TAG.c_str(), color)) return false;
  if(!read(node, NAME_TAG.c_str(), name)) return false;
  if(!read(node, LABEL_TAG.c_str(), label)) return false;

  return true;
}

bool xmlHelperClass::read(pugi::xml_node & node, cmbNucPinLibrary * dl,
                          cmbNucMaterialColors * materials)
{
  if(dl == NULL) return false;

  for (pugi::xml_node pnode = node.child(PINCELL_TAG.c_str()); pnode;
       pnode = pnode.next_sibling(PINCELL_TAG.c_str()))
  {
    PinCell * pc = new PinCell();
    if(!this->read(pnode, pc, materials)) return false;
    std::string name = pc->getName(), label = pc->getLabel();
    int count = 0;
    //Mainly for when importing a pin file
    while (dl->labelConflicts(label))
    {
      label = (QString(label.c_str()) + QString::number(count++)).toStdString();
    }
    count = 0;
    while(dl->nameConflicts(name))
    {
      name = (QString(name.c_str()) + QString::number(count++)).toStdString();
    }
    pc->setName(name);
    pc->setLabel(label);
    dl->addPin(&pc, cmbNucPinLibrary::KeepOriginal);
  }

  return true;
}

bool xmlHelperClass::read(pugi::xml_node & node, PinCell * dc,
                          cmbNucMaterialColors * materials)
{
  if( dc == NULL ) return false;
  bool r = true;
  std::string name, label;
  QColor color;
  r &= read(node, NAME_TAG.c_str(), name);
  r &= read(node, LABEL_TAG.c_str(), label);
  r &= read(node, LEGEND_COLOR_TAG.c_str(), color);
  dc->setName(name);
  dc->setLabel(label);
  dc ->legendColor = color;

  for(pugi::xml_node tnode = node.child(CYLINDER_TAG.c_str()); tnode;
      tnode = tnode.next_sibling(CYLINDER_TAG.c_str()))
  {
    Cylinder * c = new Cylinder(0,0,0);
    r &= this->read(tnode, c, materials);
    dc->AddPart(c);
  }

  double junk[2] = {0,0};
  for(pugi::xml_node tnode = node.child(FRUSTRUM_TAG.c_str()); tnode;
      tnode = tnode.next_sibling(FRUSTRUM_TAG.c_str()))
  {
    Frustum * f = new Frustum(junk, 0, 0);
    r &= this->read(tnode, f, materials);
    dc->AddPart(f);
  }

  return r;
}

bool xmlHelperClass::read(pugi::xml_node & node, Cylinder * dc,
                          cmbNucMaterialColors * materials)
{
  if( dc == NULL ) return false;
  bool r = true;
  r &= read(node, RADIUS_TAG.c_str(), dc->r);
  r &= readPSP(node, dc, materials);
  return r;
}

bool xmlHelperClass::read(pugi::xml_node & node, Frustum * dc,
                          cmbNucMaterialColors * materials)
{
  if( dc == NULL ) return false;
  bool r = true;
  r &= read(node, RADIUS_TAG.c_str(), dc->r, 2);
  r &= readPSP(node, dc, materials);
  return r;
}

bool xmlHelperClass::readPSP(pugi::xml_node & node, PinSubPart * dc,
                             cmbNucMaterialColors * materials)
{
  if( dc == NULL ) return false;
  QString str;
  if(!this->read(node, LOC_TAG.c_str(), str)) return false;
  QStringList l = str.split(",");
  dc->x = l.value(0).toDouble();
  dc->y = l.value(1).toDouble();
  dc->z1 = l.value(2).toDouble();
  dc->z2 = l.value(3).toDouble();
  int i = 0;
  for(pugi::xml_node tnode = node.child(MATERIAL_LAYER_TAG.c_str()); tnode;
      tnode = tnode.next_sibling(MATERIAL_LAYER_TAG.c_str()))
  {
    cmbNucMaterialLayer * ml = new cmbNucMaterialLayer();
    if(!read(tnode, ml, materials)) return false;
    dc->setMaterialLayer(i++, ml);
  }
  return true;
}

bool xmlHelperClass::read(pugi::xml_node & node, cmbNucMaterialLayer * ml,
                          cmbNucMaterialColors * materials)
{
  bool r = true;
  QString materialName;
  r &= read(node, THICKNESS_TAG.c_str(), ml->getThickness(), 2);
  r &= read(node, MATERIAL_TAG.c_str(), materialName);
  QPointer<cmbNucMaterial> cnm = materials->getMaterialByName(materialName);
  ml->changeMaterial(cnm);

  return r;
}

bool xmlHelperClass::read(pugi::xml_node & node, cmbNucDuctLibrary * dl,
                          cmbNucMaterialColors * materials)
{
  if(dl == NULL) return false;

  for(pugi::xml_node tnode = node.child(DUCT_CELL_TAG.c_str()); tnode;
      tnode = tnode.next_sibling(DUCT_CELL_TAG.c_str()))
  {
    DuctCell * dc  = new DuctCell();
    if(!this->read(tnode, dc, materials)) return false;
    std::string name = dc->getName();
    int count = 0;
    while(dl->nameConflicts(name))
    {
      name = (QString(name.c_str()) + QString::number(count++)).toStdString();
    }
    dc->setName(name);
    dl->addDuct(dc);
  }

  return true;
}

bool xmlHelperClass::read(pugi::xml_node & node, DuctCell * dc,
                          cmbNucMaterialColors * materials)
{
  if(dc == NULL) return false;
  std::string name;
  if(!this->read(node, NAME_TAG.c_str(), name)) return false;
  dc->setName(name);
  for(pugi::xml_node tnode = node.child(DUCT_LAYER_TAG.c_str()); tnode;
      tnode = tnode.next_sibling(DUCT_LAYER_TAG.c_str()))
  {
    Duct * d = new Duct(0,0,0);
    if(!read(tnode, d, materials)) return false;
    dc->AddDuct(d);
  }
  return true;
}

bool xmlHelperClass::read(pugi::xml_node & node, Duct * dc,
                          cmbNucMaterialColors * materials)
{
  if(dc == NULL) return false;
  QString str;
  if(!read(node, LOC_TAG.c_str(), str)) return false;
  QStringList l = str.split(",");

  dc->x = l.value(0).toDouble();
  dc->y = l.value(1).toDouble();

  dc->setZ1(l.value(2).toDouble());
  dc->setZ2(l.value(3).toDouble());

  if(!read(node, THICKNESS_TAG.c_str(), dc->thickness, 2)) return false;

  int i = 0;
  for(pugi::xml_node tnode = node.child(MATERIAL_LAYER_TAG.c_str()); tnode;
      tnode = tnode.next_sibling(MATERIAL_LAYER_TAG.c_str()))
  {
    cmbNucMaterialLayer * ml = new cmbNucMaterialLayer();
    if(!read(tnode, ml, materials)) return false;
    dc->setMaterialLayer(i++, ml);
  }

  return true;
}

#define READ_PARAM_VALUE(KEY, VALUE)\
read(paramNode, #KEY, params->VALUE);

bool xmlHelperClass::read(pugi::xml_node & node, cmbNucAssembly * assy)
{
  bool r = true;
  std::string tmp;
  if(!read(node, LABEL_TAG.c_str(), tmp)) return false;
  assy->setLabel(tmp);
  if(!read(node, DUCT_TAG.c_str(), tmp)) return false;
  cmbNucDuctLibrary * dl = assy->getDuctLibrary();
  DuctCell * d = dl->GetDuctCell(tmp);
  assy->setDuctCell(d);
  if(!read(node, GEOMETRY_TAG.c_str(), tmp)) return false;
  assy->setGeometryLabel(tmp);
  QColor color;
  if(!read(node, LEGEND_COLOR_TAG.c_str(), color)) return false;
  assy->SetLegendColor(color);
  bool iac;
  if(!read(node, CENTER_PINS_TAG.c_str(), iac)) return false;
  assy->setCenterPins(iac);
  double pitch[2];
  if(!read(node, PITCH_TAG.c_str(), pitch, 2)) return false;
  assy->setPitch(pitch[0], pitch[1], false);

  double degree;
  if(read(node, ROTATE_TAG.c_str(), degree))
  {
    assy->setZAxisRotation(degree);
  }

  pugi::xml_node paramNode = node.child(PARAMETERS_TAG.c_str());
  {
    cmbAssyParameters * params = assy->GetParameters();

    //Other Parameters
    READ_PARAM_VALUE(Geometry, Geometry);
    READ_PARAM_VALUE(TetMeshSize, TetMeshSize);
    READ_PARAM_VALUE(RadialMeshSize, RadialMeshSize);
    read(paramNode, MOVE_TAG.c_str(), params->MoveXYZ, 3);
    read(paramNode, CENTER_TAG.c_str(), params->CenterXYZ);


    READ_PARAM_VALUE(AxialMeshSize, AxialMeshSize);
    READ_PARAM_VALUE(HBlock, HBlock);
#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, DK)\
    read(paramNode, #Key, params->Var);
    ASSYGEN_EXTRA_VARABLE_MACRO()
#undef FUN_SIMPLE


    for(pugi::xml_node tnode = node.child(UNKNOWN_TAG.c_str()); tnode;
        tnode = tnode.next_sibling(UNKNOWN_TAG.c_str()))
    {
      std::string tmp;
      if(read(tnode, STR_TAG.c_str(), tmp))
        params->UnknownParams.push_back(tmp);
    }
  }

  pugi::xml_node lnode = node.child(LATTICE_TAG.c_str());
  std::map<QString, int> used;
  if(!read( lnode, assy->getLattice(), used )) return false;
  assy->setUsedLabels(used);

  return true;
}
#undef READ_PARAM_VALUE

bool xmlHelperClass::read(pugi::xml_node & node, Lattice & lattice, std::map<QString, int> & used)
{
  unsigned int type;
  if(!read(node, TYPE_TAG.c_str(), type)) return false;
  int subtype;
  if(!read(node, SUB_TYPE_TAG.c_str(), subtype)) return false;

  lattice.SetGeometryType(static_cast<enumGeometryType>(type));
  lattice.SetGeometrySubType(subtype);

  std::vector< std::vector< std::string > > grid;
  QString sgrid;
  if(!read(node, GRID_TAG.c_str(), sgrid)) return false;
  QStringList rs = sgrid.split(";");

  for(unsigned int i = 0; i < rs.size(); ++i)
  {
    QString & t = rs[i];
    if(t.isEmpty()) continue;
    size_t at = grid.size();
    grid.resize(grid.size() + 1);
    std::vector<std::string> & v = grid[at];
    QStringList tl = t.split(",");
    for(unsigned int j = 0; j < tl.size(); ++j)
    {
      v.push_back(tl[j].toStdString());
    }
  }

  size_t xs = grid.size();
  if(xs == 0) return false;
  size_t ys = grid[0].size();
  lattice.SetDimensions(xs, ys, true);
  for(unsigned int i = 0; i < grid.size(); ++i)
  {
    for(unsigned int j = 0; j < grid[i].size(); ++j)
    {
      used[QString( grid[i][j].c_str())]++;
      lattice.SetCell( i, j, grid[i][j]);
    }
  }

  return true;
}

bool xmlHelperClass::read(pugi::xml_node & node, std::string attName,
                          std::vector<cmbNucCoreParams::NeumannSetStruct> & out)
{
  bool r = true;
  pugi::xml_node nnode = node.child(attName.c_str());
  for(pugi::xml_node tnode = nnode.child(NEUMANN_VALUE_TAG.c_str()); tnode;
      tnode = tnode.next_sibling(NEUMANN_VALUE_TAG.c_str()))
  {
    cmbNucCoreParams::NeumannSetStruct nss;
    r &= read(tnode, SIDE_TAG.c_str(), nss.Side);
    r &= read(tnode, ID_TAG.c_str(), nss.Id);
    r &= read(tnode, EQUATION_TAG.c_str(), nss.Equation);
    out.push_back(nss);
  }
  return r;
}

bool xmlHelperClass::read(pugi::xml_node & node, std::string attName,
                           cmbNucCoreParams::ExtrudeStruct & es)
{
  bool r = true;
  pugi::xml_node ttnode = node.child(attName.c_str());
  r &= read(ttnode, SIZE_TAG.c_str(), es.Size);
  r &= read(ttnode, DIVISIONS_TAG.c_str(), es.Divisions);
  return r;
}

//////////////////////////////////read write static functions////////////////////////////////
bool xmlFileReader::read(std::string fname, cmbNucCore & core)
{
  std::ifstream in(fname.c_str(), std::ios::in);
  if (!in)
  {
    return false;
  }

  // Allocate string
  std::string content;
  in.seekg(0, std::ios::end);
  content.resize(in.tellg());

  // Read file
  in.seekg(0, std::ios::beg);
  in.read(&content[0], content.size());
  in.close();

  xmlHelperClass helper;

  core.CurrentFileName = fname;

  return helper.read(content, core);
}

bool xmlFileReader::read(std::string fname, cmbNucMaterialColors * materials)
{
  xmlHelperClass helper;

  pugi::xml_document document;
  if(!helper.openReadFile(fname,document)) return false;
  pugi::xml_node node = document.child(CORE_TAG.c_str()).child(MATERIALS_TAG.c_str());
  return helper.read(node, materials);
}

bool xmlFileReader::read(std::string fname, std::vector<PinCell*> & pincells, cmbNucMaterialColors * materials)
{
  xmlHelperClass helper;

  pugi::xml_document document;
  if(!helper.openReadFile(fname,document)) return false;

  pugi::xml_node node = document.child(CORE_TAG.c_str()).child(MATERIALS_TAG.c_str());
  if(!helper.read(node, materials)) return false;

  node = document.child(CORE_TAG.c_str()).child(PINS_TAG.c_str());

  for (pugi::xml_node pnode = node.child(PINCELL_TAG.c_str()); pnode;
       pnode = pnode.next_sibling(PINCELL_TAG.c_str()))
  {
    PinCell * pc = new PinCell();
    if(!helper.read(pnode, pc, materials)) return false;
    pincells.push_back(pc);
  }
  return true;
}

bool xmlFileReader::read(std::string fname, std::vector<DuctCell*> & ductcells,
                         cmbNucMaterialColors * materials)
{
  xmlHelperClass helper;

  pugi::xml_document document;
  if(!helper.openReadFile(fname,document)) return false;

  pugi::xml_node node = document.child(CORE_TAG.c_str()).child(MATERIALS_TAG.c_str());
  if(!helper.read(node, materials)) return false;

  node = document.child(CORE_TAG.c_str()).child(DUCTS_TAG.c_str());

  for (pugi::xml_node pnode = node.child(DUCT_CELL_TAG.c_str()); pnode;
       pnode = pnode.next_sibling(DUCT_CELL_TAG.c_str()))
  {
    DuctCell * dc = new DuctCell();
    if(!helper.read(pnode, dc, materials)) return false;
    ductcells.push_back(dc);
  }
  return true;
}

bool xmlFileReader::read(std::string fname, std::vector<cmbNucAssembly*> & assys,
                         cmbNucPinLibrary * pl,
                         cmbNucDuctLibrary * dl,
                         cmbNucMaterialColors * materials)
{
  if(pl == NULL) return false;
  if(dl == NULL) return false;
  xmlHelperClass helper;

  pugi::xml_document document;
  if(!helper.openReadFile(fname,document)) return false;

  pugi::xml_node rnode = document.child(CORE_TAG.c_str());

  {
    pugi::xml_node node = rnode.child(MATERIALS_TAG.c_str());
    if(!helper.read(node, materials)) return false;
  }

  {
    pugi::xml_node node = rnode.child(DUCTS_TAG.c_str());
    if(!helper.read(node, dl, materials)) return false;
  }

  {
    pugi::xml_node node = rnode.child(PINS_TAG.c_str());
    if(!helper.read(node, pl, materials)) return false;
  }

  for(pugi::xml_node tnode = rnode.child(ASSEMBLY_TAG.c_str()); tnode;
      tnode = tnode.next_sibling(ASSEMBLY_TAG.c_str()))
  {
    cmbNucAssembly* assy = new cmbNucAssembly;
    assy->setPinLibrary(pl);
    assy->setDuctLibrary(dl);
    if(!helper.read(tnode, assy)) return false;
    assys.push_back(assy);
  }
  return true;
}


bool xmlFileWriter::write(std::string fname, cmbNucCore & core, bool /*updateFname*/) //TODO the file update
{
  std::string out;
  xmlHelperClass helper;
  if(helper.writeToString(out, core) && helper.writeStringToFile(fname, out))
  {
    core.setAndTestDiffFromFiles(false);
    return true;
  }
  return false;
}

