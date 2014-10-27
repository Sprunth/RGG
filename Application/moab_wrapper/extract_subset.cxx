#include "extract_subset.h"
#include "SimpleMoab.h"

#include <algorithm>

bool extract_subset::extract(std::string fin, std::string fout,
                             std::vector<std::string> const& remove_material)
{
  smoab::Interface interface(fin);
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
  for(iterator i=geomParents.begin(); i != geomParents.end(); ++i)
  {
    std::string name = interface.name(*i);
    if(name.empty())
      name = "!!!!UnknownMaterial!!!!";
    else
      name = name.substr(2);
    bool remove = false;
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
  interface.save(fout);
  return true;
}
