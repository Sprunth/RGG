#include "extract_subset.h"
#include "SimpleMoab.h"

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
    for(unsigned int j = 0; j < remove_material.size(); ++j)
    {
      remove = remove || name == remove_material[j];
    }
    if(remove)
    {
      interface.remove(*i);
    }
  }
  interface.save(fout);
  return true;
}
