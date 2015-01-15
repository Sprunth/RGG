#include "xmlFileIO.h"

#include "cmbNucCore.h"
#include "cmbNucMaterialColors.h"
#include "cmbNucDuctLibrary.h"

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

  bool write(pugi::xml_node & materialElement, cmbNucMaterialColors * materials);
  bool write(pugi::xml_node & materialElement, QPointer< cmbNucMaterial > material);

  bool write(pugi::xml_node & node, cmbNucDuctLibrary * dl);
  bool write(pugi::xml_node & node, DuctCell * dc);
  bool write(pugi::xml_node & node, Duct * dc);

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
    QString strcolor = QString("%1, %2, %3, %4").arg(v.redF()).arg(v.greenF()).arg(v.blueF()).arg(v.alphaF());
    return write(node, attName, strcolor);
  }
  bool write(pugi::xml_node & node, std::string attName, double const& v)
  {
    QString strcolor = QString("%1").arg(v);
    return write(node, attName, strcolor);
  }
  bool write(pugi::xml_node & node, std::string attName, double const* v, int size)
  {
    QString strcolor;
    switch(size)
    {
      case 1: return write(node, attName, *v);
      case 2: strcolor = QString("%1, %2").arg(v[0]).arg(v[1]); break;
      case 3: strcolor = QString("%1, %2, %3").arg(v[0]).arg(v[1]).arg(v[2]); break;
      case 4: strcolor = QString("%1, %2, %3, %4").arg(v[0]).arg(v[1]).arg(v[2]).arg(v[3]); break;
    }
    return write(node, attName, strcolor);
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
  r &= write(node, "Loc", QString("%1, %2, %3, %4").arg(dc->x).arg(dc->y).arg(dc->getZ1()).arg(dc->getZ2()));
  r &= write(node, "Thickness", dc->thickness, 2);

  size_t num = dc->NumberOfLayers();
  for(size_t i = 0; i < num; ++i)
  {
    pugi::xml_node xn = node.append_child("MaterialLayer");
    r &= this->write(xn, dc->getMaterialLayer(i));
  }
  return r;
}

bool xmlHelperClass::write(pugi::xml_node & node, cmbNucMaterialLayer const& v)
{
  bool r = true;
  r &= write(node, "Thickness", v.getThickness(), 2);
  r &= write(node, "Material", v.getMaterial()->getName());
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
