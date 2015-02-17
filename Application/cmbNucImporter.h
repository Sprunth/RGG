#ifndef cmbNucImporter_h
#define cmbNucImporter_h

class cmbNucCore;
class cmbNucMainWindow;

class cmbNucImporter
{
public:
  cmbNucImporter(cmbNucMainWindow * mw);
  bool importInpFile();
protected:
  cmbNucMainWindow * mainWindow;
};

#endif
