#ifndef _cmbNucAssemblyLink_h_
#define _cmbNucAssemblyLink_h_

#include <QObject>
#include <QSet>

#include "cmbNucPartDefinition.h"
#include "cmbNucAssembly.h"

class cmbNucAssemblyLink : public AssyPartObj
{

public:
  cmbNucAssemblyLink(cmbNucAssembly *l, std::string const& msid, std::string const& nsid);
  cmbNucAssemblyLink();
  virtual ~cmbNucAssemblyLink(){}
  bool isValid();
  cmbNucAssembly * getLink() const;
  std::string const& getMaterialStartID() const;
  std::string const& getNeumannStartID() const;
  void setMaterialStartID(std::string const& msid );
  void setNeumannStartID(std::string const& nsid);
  void setLink(cmbNucAssembly *);

  enumNucPartsType GetType() const;
  QColor GetLegendColor() const;
  void SetLegendColor(const QColor& color);

  virtual std::string getTitle()
  { return "Assembly: " + AssyPartObj::getTitle() + " --> (" + getLink()->getLabel() +")"; }

  cmbNucAssembly * clone();

protected:
  cmbNucAssembly * link;
  std::string materialStartID;
  std::string neumannStartID;
  QColor legendColor;
};

#endif
