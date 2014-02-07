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

class cmbNucAssembly;
class cmbNucCore;

class inpFileReader
{
public:
  inpFileReader();
  enum FileType{ERROR, UNKNOWN, ASSEMBLY, CORE};
  FileType open(std::string fname);
  void close();
  bool read(cmbNucAssembly & assembly);
  bool read(cmbNucCore & core);
protected:
  std::string FileName;
  FileType Type;
  std::string CleanFile;
private:
};

class inpFileWriter
{
public:
  static bool write(std::string fname, cmbNucAssembly & assembly, bool updateFname = true);
  static bool write(std::string fname, cmbNucCore & core, bool updateFname = true);
private:
};


#endif
