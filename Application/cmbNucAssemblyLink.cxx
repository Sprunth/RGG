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
