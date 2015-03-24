#ifndef cmbNucImporter_h
#define cmbNucImporter_h

#include <cmbNucMaterial.h>
#include <map>
#include <string>
#include <vector>

#include <QStringList>

class cmbNucCore;
class cmbNucMainWindow;
class PinCell;
class DuctCell;

class cmbNucImporter
{
public:
  cmbNucImporter(cmbNucMainWindow * mw);
  bool importInpFile();
  bool importXMLPins();
  bool importXMLDucts();
  bool importXMLAssembly();
  void clearLog()
  { log.clear(); }
  std::vector<std::string> const& getLog()
  { return log; }
protected:
  void addPin(PinCell * pc, double dh, std::map<std::string, std::string> & nc);
  void addDuct(DuctCell * dc, double dh, double dt[2], std::map<std::string, std::string> & nc);
  QStringList getXMLFiles();
  QPointer<cmbNucMaterial> getMaterial(QPointer<cmbNucMaterial> in);
  cmbNucMainWindow * mainWindow;
  std::vector< std::string > log;
};

#endif
