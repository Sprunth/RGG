#include "xmlFileIO.h"

#include "cmbNucCore.h"
#include "cmbNucAssembly.h"
#include "cmbNucMaterialColors.h"
#include "cmbNucDuctLibrary.h"
#include "cmbNucPinLibrary.h"

#define PUGIXML_HEADER_ONLY
#include "src/pugixml.cpp"

#include <QPointer.h>
#include <QString.h>
#include <QColor.h>

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
  r &= write(materialElement, "name", material->getName());
  r &= write(materialElement, "label", material->getLabel());
  r &= write(materialElement, "color", material->getColor());
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
    pugi::xml_node xn = node.append_child("PinCell");
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
  r &= write(node, "radius", c->r);
  r &= writePSP(node, c);
  return r;
}

bool xmlHelperClass::write(pugi::xml_node & node, Frustum * f)
{
  bool r = true;
  r &= write(node, "radius", f->r, 2);
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
      r &= write(paramNode, "Uknown", params->UnknownParams[i]);
    }

  }

  pugi::xml_node lnode = node.append_child("Lattice");
  r &= write( lnode, assy->getLattice());

  return r;
}

bool xmlHelperClass::writeToString(std::string & out, cmbNucCore & core)
{
  pugi::xml_document document;
  pugi::xml_node rootElement = document.append_child("NuclearCore");
  pugi::xml_node mnode = rootElement.append_child("Materials");
  if(!write(mnode, cmbNucMaterialColors::instance())) return false;
  pugi::xml_node dnode = rootElement.append_child("Ducts");
  if(!write(dnode, core.getDuctLibrary())) return false;

  pugi::xml_node pnode = rootElement.append_child("Pins");
  if(!write(pnode, core.getPinLibrary())) return false;

  pugi::xml_node lnode = rootElement.append_child("Lattice");
  if(!write(lnode, core.getLattice())) return false;

  int num = core.GetNumberOfAssemblies();

  for(int i = 0; i < num; ++i)
  {
    cmbNucAssembly* assy = core.GetAssembly(i);
    pugi::xml_node assyNode = rootElement.append_child("Assembly");
    if(!write(assyNode, assy)) return false;
  }

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
