#include "xmlFileIO.h"

#include "cmbNucCore.h"
#include "cmbNucAssembly.h"
#include "cmbNucMaterialColors.h"
#include "cmbNucDuctLibrary.h"
#include "cmbNucPinLibrary.h"
#include "cmbNucDefaults.h"

#define PUGIXML_HEADER_ONLY
#include "src/pugixml.cpp"

#include <QPointer.h>
#include <QString.h>
#include <QColor.h>

const std::string CORE_TAG = "NuclearCore";
const std::string MATERIAL_TAG = "Materials";
const std::string DUCTS_TAG = "Ducts";
const std::string PINS_TAG = "Pins";
const std::string DEFAULTS_TAG = "Defaults";
const std::string PINCELL_TAG = "PinCell";

class xmlHelperClass
{
public:
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

  bool read(pugi::xml_node & node, cmbNucPinLibrary * dl);
  bool read(pugi::xml_node & node, PinCell * dc);
  bool read(pugi::xml_node & node, Cylinder * dc);
  bool read(pugi::xml_node & node, Frustum * dc);
  bool readPSP(pugi::xml_node & node, PinSubPart * dc);

  bool read(pugi::xml_node & node, cmbNucMaterialLayer * ml);

  bool read(pugi::xml_node & node, cmbNucDuctLibrary * dl);
  bool read(pugi::xml_node & node, DuctCell * dc);
  bool read(pugi::xml_node & node, Duct * dc);

  bool read(pugi::xml_node & node, std::string attName, QColor & v)
  {
    pugi::xml_attribute att = node.attribute(attName.c_str());
    if(att) return false;
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
    if(att) return false;
    v = std::string(att.value());
    return true;
  }

  bool read(pugi::xml_node & node, std::string attName, QString & v)
  {
    pugi::xml_attribute att = node.attribute(attName.c_str());
    if(att) return false;
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

bool xmlHelperClass::write(pugi::xml_node & materialElement,
                                    cmbNucMaterialColors * cnmc)
{
  std::vector< QPointer< cmbNucMaterial > > materials = cnmc->getMaterials();
  for(size_t i = 0; i < materials.size(); ++i)
  {
    pugi::xml_node mElement = materialElement.append_child("Material");
    this->write(mElement, materials[i]);
  }
  return true;
}

bool xmlHelperClass::write(pugi::xml_node & materialElement, QPointer< cmbNucMaterial > material)
{
  bool r = true;
  r &= write(materialElement, "Name", material->getName());
  r &= write(materialElement, "Label", material->getLabel());
  r &= write(materialElement, "Color", material->getColor());
  return r;
}

bool xmlHelperClass::write(pugi::xml_node & node, cmbNucDuctLibrary * dl)
{
  std::size_t num = dl->GetNumberOfDuctCells();
  bool r = true;
  for(size_t i = 0; i < num; ++i)
  {
    pugi::xml_node xn = node.append_child("DuctCell");
    r &= this->write(xn, dl->GetDuctCell(i));
  }
  return r;
}

bool xmlHelperClass::write(pugi::xml_node & node, DuctCell * dc)
{
  bool r = true;
  r &= write(node, "Name", dc->getName());
  size_t num = dc->numberOfDucts();
  for(size_t i = 0; i < num; ++i)
  {
    pugi::xml_node xn = node.append_child("DuctLayer");
    r &= this->write(xn, dc->getDuct(i));
  }
  return r;
}

bool xmlHelperClass::write(pugi::xml_node & node, Duct * dc)
{
  bool r = true;
  r &= write(node, "Loc", QString("%1, %2, %3, %4").arg(dc->x, 0, 'g', 9).arg(dc->y, 0, 'g', 9).arg(dc->getZ1(), 0, 'g', 9).arg(dc->getZ2(), 0, 'g', 9));
  r &= write(node, "Thickness", dc->thickness, 2);

  size_t num = dc->NumberOfLayers();
  for(size_t i = 0; i < num; ++i)
  {
    pugi::xml_node xn = node.append_child("MaterialLayer");
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
  r &= write(node, "Name", dc->getName());
  r &= write(node, "Label", dc->getLabel());
  r &= write(node, "LegendColor", dc->GetLegendColor());

  size_t num = dc->NumberOfCylinders();
  for(unsigned int i = 0; i < num; ++i)
  {
    pugi::xml_node xn = node.append_child("Cylinder");
    r &= this->write(xn, dc->GetCylinder(i));
  }

  num = dc->NumberOfFrustums();
  for(unsigned int i = 0; i < num; ++i)
  {
    pugi::xml_node xn = node.append_child("Frustrum");
    r &= this->write(xn, dc->GetFrustum(i));
  }
  return r;
}

bool xmlHelperClass::write(pugi::xml_node & node, Cylinder * c)
{
  bool r = true;
  r &= write(node, "Radius", c->r);
  r &= writePSP(node, c);
  return r;
}

bool xmlHelperClass::write(pugi::xml_node & node, Frustum * f)
{
  bool r = true;
  r &= write(node, "Radius", f->r, 2);
  r &= writePSP(node, f);
  return r;
}

bool xmlHelperClass::writePSP(pugi::xml_node & node, PinSubPart * p)
{
  bool r = true;
  r &= write(node, "Loc", QString("%1, %2, %3, %4").arg(p->x, 0, 'g', 9).arg(p->y, 0, 'g', 9).arg(p->z1).arg(p->z2, 0, 'g', 9));
  size_t num = p->GetNumberOfLayers();
  for(size_t i = 0; i < num; ++i)
  {
    pugi::xml_node xn = node.append_child("MaterialLayer");
    r &= this->write(xn, p->getMaterialLayer(i));
  }
  return true;
}

bool xmlHelperClass::write(pugi::xml_node & node, cmbNucMaterialLayer const& v)
{
  bool r = true;
  r &= write(node, "Thickness", v.getThickness(), 2);
  r &= write(node, "Material", v.getMaterial()->getName());
  return r;
}

bool xmlHelperClass::write(pugi::xml_node & node, Lattice & lattice)
{
  bool r = true;
  r &= write(node, "Type", static_cast<unsigned int>(lattice.GetType()));
  r &= write(node, "SubType", lattice.GetGeometrySubType());
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
  r &= write(node, "Grid", grid);
  return r;
}

#define WRITE_PARAM_VALUE(KEY, VALUE)\
  if(params->isValueSet(params->VALUE))\
    r &= write(paramNode, #KEY, params->VALUE);

bool xmlHelperClass::write(pugi::xml_node & node, cmbNucAssembly * assy)
{
  bool r = true;
  r &= write(node, "Label", assy->getLabel());
  r &= write(node, "Duct", assy->getAssyDuct().getName());
  r &= write(node, "Geometry", assy->getGeometryLabel());
  r &= write(node, "LegendColor", assy->GetLegendColor());
  r &= write(node, "CenterPins", assy->isPinsAutoCentered());
  r &= write(node, "Pitch", QString("%1, %2").arg(assy->getPinPitchX(), 0, 'g', 9).arg(assy->getPinPitchY(), 0, 'g', 9));

  pugi::xml_node transNodeRoot = node.append_child("Transformations");
  {
    size_t num = assy->getNumberOfTransforms();
    for(size_t i = 0; i < num; ++i)
    {
      pugi::xml_node tnode = transNodeRoot.append_child("Transform");
      cmbNucAssembly::Transform* x = assy->getTransform(i);
      //virtual bool reverse() const = 0;
      r &= write(tnode, "Type", x->getLabel());
      r &= write(tnode, "Value", x->getValue());
      r &= write(tnode, "Axis", static_cast<unsigned int>(x->getAxis()));
      r &= write(tnode, "Direction", x->reverse());
    }
  }

  pugi::xml_node paramNode = node.append_child("Parameters");
  {
    cmbAssyParameters * params = assy->GetParameters();

    //Other Parameters
    WRITE_PARAM_VALUE(Geometry, Geometry);
    WRITE_PARAM_VALUE(TetMeshSize, TetMeshSize);
    WRITE_PARAM_VALUE(RadialMeshSize, RadialMeshSize);
    if(params->MoveXYZ[0]!=0 || params->MoveXYZ[1]!=0 || params->MoveXYZ[2]!=0)
    {
      r &= write(paramNode, "Move", params->MoveXYZ);
    }
    r &= write(paramNode, "Center", params->CenterXYZ);

    WRITE_PARAM_VALUE(AxialMeshSize, AxialMeshSize);
    WRITE_PARAM_VALUE(HBlock, HBlock);
#define FUN_SIMPLE(TYPE,X,Var,Key,DEFAULT, DK)\
    if(params->isValueSet(params->Var))\
      r &= write(paramNode, #Key, params->Var);
    ASSYGEN_EXTRA_VARABLE_MACRO()
#undef FUN_SIMPLE

    for(unsigned int i = 0; i < params->UnknownParams.size(); ++i)
    {
      r &= write(paramNode, "Unknown", params->UnknownParams[i]);
    }

  }

  pugi::xml_node lnode = node.append_child("Lattice");
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
    pugi::xml_node nssnode = ttnode.append_child("NeumannValue");
    cmbNucCoreParams::NeumannSetStruct const& nss = nssv[i];
    r &= write(nssnode, "Side", nss.Side);
    r &= write(nssnode, "Id", nss.Id);
    r &= write(nssnode, "Equation", nss.Equation);
  }
  return r;
}

bool xmlHelperClass::write(pugi::xml_node & node, std::string attName,
                           cmbNucCoreParams::ExtrudeStruct const& es)
{
  bool r = true;
  pugi::xml_node ttnode = node.append_child(attName.c_str());
  r &= write(ttnode, "Size", es.Size);
  r &= write(ttnode, "Divisions", es.Divisions);
  return r;
}

bool xmlHelperClass::writeToString(std::string & out, cmbNucCore & core)
{
  pugi::xml_document document;
  pugi::xml_node rootElement = document.append_child(CORE_TAG.c_str());
  pugi::xml_node mnode = rootElement.append_child(MATERIAL_TAG.c_str());
  if(!write(mnode, cmbNucMaterialColors::instance())) return false;
  pugi::xml_node dnode = rootElement.append_child(DUCTS_TAG.c_str());
  if(!write(dnode, core.getDuctLibrary())) return false;

  pugi::xml_node pnode = rootElement.append_child(PINS_TAG.c_str());
  if(!write(pnode, core.getPinLibrary())) return false;

  //Write defaults
  {
    QPointer<cmbNucDefaults> defaults = core.GetDefaults();
    pugi::xml_node node = rootElement.append_child(DEFAULTS_TAG.c_str());

    double vd;
    int vi;
    QString vs;
    if(defaults->getAxialMeshSize(vd))
    {
      if(!write( node, "AxialMeshSize", vd)) return false;
    }
    if(defaults->getEdgeInterval(vi))
    {
      if(!write( node, "EdgeInterval", vi)) return false;
    }
    if(defaults->getMeshType(vs))
    {
      if(!write( node, "MeshType", vs)) return false;
    }
  }

  int num = core.GetNumberOfAssemblies();

  for(int i = 0; i < num; ++i)
  {
    cmbNucAssembly* assy = core.GetAssembly(i);
    pugi::xml_node assyNode = rootElement.append_child("Assembly");
    if(!write(assyNode, assy)) return false;
  }

  //write the background
  {
    pugi::xml_node node = rootElement.append_child("Background");
    if(!write(node, "Mode", static_cast<unsigned int>(core.Params.BackgroundMode))) return false;
    if(!write(node, "Generate", core.getHasCylinder())) return false;
    if(!write(node, "CylinderRadius", core.getCylinderRadius())) return false;
    if(!write(node, "CylinderOuterSpacing", core.getCylinderOuterSpacing())) return false;
    if(!write(node, "BackgroundFileName", core.Params.Background)) return false;
  }

  //Write parameters
  {
    pugi::xml_node node = rootElement.append_child("Parameters");
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
      if(!write(node, "Unknown", core.Params.UnknownKeyWords[i])) return false;
    }
  }

  pugi::xml_node lnode = rootElement.append_child("Lattice");
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
    pugi::xml_node node = rootElement.child(MATERIAL_TAG.c_str());
    if(!read(node, cmbNucMaterialColors::instance())) return false;
  }
  return true;
}

bool xmlHelperClass::read(pugi::xml_node & node, cmbNucMaterialColors * materials)
{
  if(materials == NULL) return false;
  QString name, label;
  QColor color;
  for (pugi::xml_node material = node.child("Material"); material; material = node.next_sibling("Material"))
  {
    if(!this->read(material, name, label, color)) return false;
    materials->AddOrUpdateMaterial(name, label, color);
  }
  return true;
}

bool xmlHelperClass::read(pugi::xml_node & node, QString & name, QString & label, QColor & color)
{
  if(!read(node, "Color", color)) return false;
  if(!read(node, "Name", name)) return false;
  if(!read(node, "Label", label)) return false;

  return true;
}

bool xmlHelperClass::read(pugi::xml_node & node, cmbNucPinLibrary * dl)
{
  if(dl == NULL) return false;

  for (pugi::xml_node pnode = node.child(PINCELL_TAG.c_str()); pnode;
       pnode = node.next_sibling(PINCELL_TAG.c_str()))
  {
    PinCell * pc = new PinCell();
    if(!this->read(pnode, pc)) return false;
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

bool xmlHelperClass::read(pugi::xml_node & node, PinCell * dc)
{
  if( dc == NULL ) return false;
  bool r = true;
  std::string name, label;
  QColor color;
  r &= read(node, "Name", name);
  r &= read(node, "Label", label);
  r &= read(node, "LegendColor", color);
  dc->setName(name);
  dc->setLabel(label);
  dc ->legendColor = color;

  for(pugi::xml_node tnode = node.child("Cylinder"); tnode;
      tnode = node.next_sibling("Cylinder"))
  {
    Cylinder * c = new Cylinder(0,0,0);
    r &= this->read(tnode, c);
    dc->AddCylinder(c);
  }

  double junk[2] = {0,0};
  for(pugi::xml_node tnode = node.child("Frustrum"); tnode;
      tnode = node.next_sibling("Frustrum"))
  {
    Frustum * f = new Frustum(junk, 0, 0);
    r &= this->read(tnode, f);
    dc->AddFrustum(f);
  }

  return r;
}

bool xmlHelperClass::read(pugi::xml_node & node, Cylinder * dc)
{
  if( dc == NULL ) return false;
  bool r = true;
  r &= read(node, "Radius", dc->r);
  r &= readPSP(node, dc);
  return r;
}

bool xmlHelperClass::read(pugi::xml_node & node, Frustum * dc)
{
  if( dc == NULL ) return false;
  bool r = true;
  r &= read(node, "Radius", dc->r, 2);
  r &= readPSP(node, dc);
  return r;
}

bool xmlHelperClass::readPSP(pugi::xml_node & node, PinSubPart * dc)
{
  if( dc == NULL ) return false;
  QString str;
  if(!this->read(node, "Loc", str)) return false;
  QStringList l = str.split(",");
  dc->x = l.value(0).toDouble();
  dc->y = l.value(1).toDouble();
  dc->z1 = l.value(2).toDouble();
  dc->z2 = l.value(3).toDouble();
  for(pugi::xml_node tnode = node.child("MaterialLayer"); tnode;
      tnode = node.next_sibling("MaterialLayer"))
  {
    cmbNucMaterialLayer * ml = new cmbNucMaterialLayer();
    if(!read(tnode, ml)) return false;
    dc->addMaterialLayer(ml);
  }
  return true;
}

bool xmlHelperClass::read(pugi::xml_node & node, cmbNucMaterialLayer * ml)
{
  bool r = true;
  QString materialName;
  r &= read(node, "Thickness", ml->getThickness(), 2);
  r &= read(node, "Material", materialName);
  QPointer<cmbNucMaterial> cnm = cmbNucMaterialColors::instance()->getMaterialByName(materialName);
  ml->changeMaterial(cnm);

  return r;
}

bool xmlHelperClass::read(pugi::xml_node & node, cmbNucDuctLibrary * dl)
{
  if(dl == NULL) return false;

  for(pugi::xml_node tnode = node.child("DuctCell"); tnode;
      tnode = node.next_sibling("DuctCell"))
  {
    DuctCell * dc  = new DuctCell();
    if(!this->read(tnode, dc)) return false;
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

bool xmlHelperClass::read(pugi::xml_node & node, DuctCell * dc)
{
  if(dc == NULL) return false;
  std::string name;
  if(!this->read(node, "Name", name)) return false;
  dc->setName(name);
  for(pugi::xml_node tnode = node.child("DuctLayer"); tnode;
      tnode = node.next_sibling("DuctLayer"))
  {
    Duct * d = new Duct(0,0,0);
    if(!read(tnode, d)) return false;
    dc->AddDuct(d);
  }
  return true;
}

bool xmlHelperClass::read(pugi::xml_node & node, Duct * dc)
{
  if(dc == NULL) return false;
  QString str;
  if(!read(node, "loc", str)) return false;
  QStringList l = str.split(",");

  dc->x = l.value(0).toDouble();
  dc->y = l.value(1).toDouble();

  dc->setZ1(l.value(2).toDouble());
  dc->setZ2(l.value(3).toDouble());

  if(!read(node, "Thickness", dc->thickness, 2)) return false;

  for(pugi::xml_node tnode = node.child("MaterialLayer"); tnode;
      tnode = node.next_sibling("MaterialLayer"))
  {
    cmbNucMaterialLayer * ml = new cmbNucMaterialLayer();
    if(!read(tnode, ml)) return false;
    dc->addMaterialLayer(ml);
  }

  return true;
}

//////////////////////////////////read write static functions////////////////////////////////
bool xmlFileReader::read(std::string fname, cmbNucCore & core)
{
  return true;
}

bool xmlFileWriter::write(std::string fname, cmbNucCore & core, bool /*updateFname*/) //TODO the file update
{
  std::string out;
  xmlHelperClass helper;
  return helper.writeToString(out, core) && helper.writeStringToFile(fname, out);
}

