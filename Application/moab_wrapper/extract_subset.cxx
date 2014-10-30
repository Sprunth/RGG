#include "extract_subset.h"
#include "SimpleMoab.h"

#include <algorithm>

#include "../cmbNucMaterialColors.h"
#include "../cmbNucMaterial.h"

typedef cmbNucMaterialColors COLOR;

bool extract_subset::extract(std::string fin, std::string fout,
                             std::vector< std::vector<bool> > const& visible,
                             std::vector<std::string> const& remove_material)
{
  smoab::Interface interface(fin);
  { // volumns
    

  }
  { //material filter
    smoab::MaterialTag metTag;

    smoab::EntityHandle rootHandle = interface.getRoot();
    smoab::Range parents = interface.findEntityRootParents(rootHandle);
    smoab::Range dimEnts = interface.findEntitiesWithTag( metTag,
                                                          rootHandle);

    smoab::Range geomParents = smoab::intersect(parents,dimEnts);

    parents.clear(); //remove this range as it is unneeded
    dimEnts.clear();

    //now each item in range can be extracted into a different grid
    typedef smoab::Range::iterator iterator;
    int c = 0;
    for(iterator i=geomParents.begin(); i != geomParents.end(); ++i, ++c)
    {
      std::string name =
         COLOR::instance()->getMaterialByName(COLOR::createMaterialLabel(interface.name(*i).c_str()))->getName().toStdString();
      bool remove = !visible[5][c];
      std::transform(name.begin(), name.end(), name.begin(), ::tolower);
      for(unsigned int j = 0; j < remove_material.size(); ++j)
      {
        //NOTE: If this becomes a performance issue, this can be cached
        std::string rmMat = remove_material[j];
        std::transform(rmMat.begin(), rmMat.end(), rmMat.begin(), ::tolower);
        remove = remove || name == rmMat;
      }
      if(remove)
      {
        interface.remove(*i);
      }
    }
  }
  interface.save(fout);
  return true;
}
