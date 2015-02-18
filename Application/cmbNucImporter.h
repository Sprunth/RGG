#ifndef cmbNucImporter_h
#define cmbNucImporter_h

class cmbNucCore;
class cmbNucMainWindow;

class cmbNucImporter
{
public:
  cmbNucImporter(cmbNucMainWindow * mw);
  bool importInpFile();
  bool importXMLPins();
protected:
  cmbNucMainWindow * mainWindow;
};

#endif
