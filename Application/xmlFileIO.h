#ifndef xmlFileIO_h
#define xmlFileIO_h

#include <string>

class cmbNucCore;
class cmbNucPinLibrary;

class xmlFileReader
{
public:
  static bool read(std::string fname, cmbNucCore & core);
  //static bool read(std::string fname, cmbNucCore & core);
};

class xmlFileWriter
{
public:
  static bool write(std::string fname, cmbNucCore & core, bool updateFname = true);
private:
};



#endif
