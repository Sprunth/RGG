#ifndef __cmbNucPartDefinition_h
#define __cmbNucPartDefinition_h

#include <vector>
#include <QPointer>
#include <sstream>
#include <assert.h>

#include <vtkSmartPointer.h>
#include "vtkMultiBlockDataSet.h"
#include "cmbNucMaterial.h"

enum enumNucPartsType
{
  CMBNUC_CORE,
  CMBNUC_ASSEMBLY,
  CMBNUC_ASSY_DUCTCELL,
  CMBNUC_ASSY_DUCT,
  CMBNUC_ASSY_LATTICE,
  CMBNUC_ASSY_PINCELL,
  CMBNUC_ASSY_CYLINDER_PIN,
  CMBNUC_ASSY_FRUSTUM_PIN,
  CMBNUC_ASSY_BASEOBJ
};

enum enumGeometryType {
  RECTILINEAR = 0x0140,
  HEXAGONAL = 0x0241,
};

enum enumGeometryControls
{
  FLAT = 0x0001,
  VERTEX = 0x0002,
  JUST_HEX_SUBTYPE = 0x00FF,
  ANGLE_60 = 0x00100,
  ANGLE_30 = 0x00200,
  ANGLE_360 = 0x00400,
  JUST_ANGLE = 0x0FF00,
};

class AssyPartObj
{
public:
  AssyPartObj(){}
  virtual enumNucPartsType GetType() const {return CMBNUC_ASSY_BASEOBJ;}
  bool operator==(const AssyPartObj&){return false;}
  template<class T> static void removeObj(T* obj, std::vector<T*>& objs)
  {
    for(typename std::vector<T*>::iterator fit=objs.begin();
        fit!=objs.end(); ++fit)
    {
      if(*fit == obj)
      {
        delete obj;
        objs.erase(fit);
        break;
      }
    }
  }
  template<class T> static void deleteObjs(std::vector<T*>& objs)
  {
    for(typename std::vector<T*>::iterator fit=objs.begin();
        fit!=objs.end(); ++fit)
    {
      if(*fit)
      {
        delete *fit;
      }
    }
    objs.clear();
  }
  virtual std::string const& getLabel() const
  { return this->Label; }
  virtual std::string getTitle()
  { return this->Name + " (" + this->Label + ")"; }
  virtual std::string const& getName() const
  { return this->Name; }
  virtual void setLabel(std::string l)
  { this->Label = l; }
  virtual void setName(std::string n)
  { this->Name = n; }
  virtual std::string getFileName(){return "";}
  virtual QColor GetLegendColor() const
  { return Qt::white; }
protected:
  std::string Label;
  std::string Name;
};

#endif
