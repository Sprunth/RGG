#ifndef cmbNucInpExporter_h
#define cmbNucInpExporter_h

#include <vector>
#include <map>

#include <QString>

class cmbNucCore;
class cmbNucAssembly;

class cmbNucInpExporter
{
public:
  cmbNucInpExporter();
  void setCore(cmbNucCore * core);
  void updateCoreLayers(bool ignore_regen = false);
  bool exportInpFiles();
protected:
  bool updateCoreLevelLayers();
  bool updateAssemblyLevelLayers(cmbNucAssembly * assy);
  bool exportInpFile(cmbNucAssembly * assy);
  QString requestInpFileName(QString name, QString type);
  struct layers
  {
    layers()
    {}
    std::vector<double> levels;
  };
  layers coreLevelLayers;
  cmbNucCore * NuclearCore;
};

#endif
