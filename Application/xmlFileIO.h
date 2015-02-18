#ifndef xmlFileIO_h
#define xmlFileIO_h

#include <string>
#include <vector>

class cmbNucCore;
class PinCell;
class DuctCell;
class cmbNucMaterialColors;

class xmlFileReader
{
public:
  static bool read(std::string fname, cmbNucCore & core);
  static bool read(std::string fname, cmbNucMaterialColors * materials);
  static bool read(std::string fname, std::vector<PinCell*> & pincells,
                   cmbNucMaterialColors * materials);
  static bool read(std::string fname, std::vector<DuctCell*> & ductcells,
                   cmbNucMaterialColors * materials);
};

class xmlFileWriter
{
public:
  static bool write(std::string fname, cmbNucCore & core, bool updateFname = true);
private:
};



#endif
