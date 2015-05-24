#ifndef cmbNucInpExporter_h
#define cmbNucInpExporter_h

#include <vector>
#include <map>
#include <set>

#include "cmbNucLattice.h"

#include <QString>

class cmbNucCore;
class cmbNucAssembly;

class cmbNucInpExporter
{
public:
  struct layers
  {
    layers()
    {}
    std::vector<double> levels;
  };

  cmbNucInpExporter();
  void setCore(cmbNucCore * core);
  void updateCoreLayers(bool ignore_regen = false);
  bool exportInpFiles();
  bool exportCylinderINPFile(QString filename, QString rand);
  layers const& getLayers() const
  {
    return coreLevelLayers;
  }
protected:
  bool updateCoreLevelLayers();
  bool updateAssemblyLevelLayers(cmbNucAssembly * assy);
  bool exportInpFile(cmbNucAssembly * assy, bool isCylinder,
                     std::set< Lattice::CellDrawMode > const& mode);
  QString requestInpFileName(QString name, QString type);
  layers coreLevelLayers;
  cmbNucCore * NuclearCore;
};

#endif
