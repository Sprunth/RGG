#ifndef inpFileIO_h
#define inpFileIO_h

#include <string>
#include <vector>
#include <set>
#include <map>
#include <sstream>
#include <iostream>
#include <fstream>

#include "cmbNucPartDefinition.h"
#include "cmbNucPinLibrary.h"
#include "cmbNucCore.h"

class inpFileReader
{
public:
  inpFileReader();
  bool keepGoing;
  bool renamePin;
  cmbNucPinLibrary::AddMode pinAddMode;
  enum FileType{ERROR_TYPE, UNKNOWN_TYPE, ASSEMBLY_TYPE, CORE_TYPE};
  FileType open(std::string fname);
  void close();
  bool read(cmbNucAssembly & assembly, cmbNucPinLibrary * pl, cmbNucDuctLibrary * dl);
  bool read(cmbNucCore & core, bool read_assemblies = true);
  bool read_defaults(cmbNucDefaults & defaults);
  bool read_defaults(cmbNucAssembly & assembly);
  std::vector<std::string> getLog()
  { return log; }
protected:
  std::string FileName;
  FileType Type;
  std::string CleanFile;
  std::vector<std::string> log;
private:
};

class inpFileWriter
{
public:
  static bool write(std::string fname, cmbNucAssembly & assembly,
                    std::vector<cmbNucCore::boundryLayer*> const& bl,
                    bool updateFname = true, bool limited = false);
  static bool write(std::string fname, cmbNucCore & core, bool updateFname = true);
  static bool writeGSH(std::string fname, cmbNucCore & core, std::string assyName);
private:
};


#endif
