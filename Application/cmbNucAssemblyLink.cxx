#include "cmbNucAssemblyLink.h"

cmbNucAssemblyLink::cmbNucAssemblyLink(cmbNucAssembly *l, std::string const& msid, std::string const& nsid)
:link(l), materialStartID(msid), neumannStartID(nsid)
{
}

cmbNucAssemblyLink::cmbNucAssemblyLink()
:link(NULL), materialStartID(""), neumannStartID("")
{
}

bool cmbNucAssemblyLink::isValid()
{
  return this->link != NULL;
}

cmbNucAssembly * cmbNucAssemblyLink::getLink() const
{
  return this->link;
}

std::string const& cmbNucAssemblyLink::getMaterialStartID() const
{
  return this->materialStartID;
}

std::string const& cmbNucAssemblyLink::getNeumannStartID() const
{
  return this->neumannStartID;
}

void cmbNucAssemblyLink::setMaterialStartID( std::string const& msid )
{
  this->materialStartID = msid;
}

void cmbNucAssemblyLink::setNeumannStartID(std::string const& nsid)
{
  this->neumannStartID = nsid;
}

void cmbNucAssemblyLink::setLink(cmbNucAssembly * l)
{
  this->link = l;
}

enumNucPartsType cmbNucAssemblyLink::GetType() const
{
  return CMBNUC_ASSEMBLY_LINK;
}

QColor cmbNucAssemblyLink::GetLegendColor() const
{
  return this->legendColor;
}

void cmbNucAssemblyLink::SetLegendColor(const QColor& color)
{
  this->legendColor = color;
}

cmbNucAssembly * cmbNucAssemblyLink::clone()
{
  cmbNucAssembly * result = this->link->clone(this->link->getPinLibrary(),
                                              this->link->getDuctLibrary());
  std::string fname = "assembly_" + this->getLabel() + ".inp";
  std::transform(fname.begin(), fname.end(), fname.begin(), ::tolower);
  result->setLabel(this->getLabel());
  result->setFileName(fname);
  result->GetParameters()->MaterialSet_StartId = this->getMaterialStartID();
  result->GetParameters()->NeumannSet_StartId = this->getNeumannStartID();
  return result;
}
