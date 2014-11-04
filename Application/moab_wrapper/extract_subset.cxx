#include "extract_subset.h"
#include "SimpleMoab.h"

#include <algorithm>

#include "../cmbNucMaterialColors.h"
#include "../cmbNucMaterial.h"

typedef cmbNucMaterialColors COLOR;

namespace
{
  void remove_visiblity(smoab::Interface & interface,
                        smoab::Tag const& tag,
                        std::vector<bool> const& visible)
  {
    smoab::EntityHandle rootHandle = interface.getRoot();
    smoab::Range parents = interface.findEntityRootParents(rootHandle);
    smoab::Range dimEnts = interface.findEntitiesWithTag( tag, rootHandle);

    smoab::Range geomParents = smoab::intersect(parents,dimEnts);

    parents.clear(); //remove this range as it is unneeded
    dimEnts.clear();

    //now each item in range can be extracted into a different grid
    typedef smoab::Range::iterator iterator;
    int c = 0;
    for(iterator i=geomParents.begin(); i != geomParents.end(); ++i, ++c)
    {
      if(!visible[c])
      {
        interface.remove(*i);
      }
    }
  }

  void remove_visiblity_and_material(smoab::Interface & interface,
                                     smoab::Tag const& tag,
                                     std::vector<bool> const& visible,
                                     std::vector<std::string> const& remove_material )
  {
    smoab::EntityHandle rootHandle = interface.getRoot();
    smoab::Range parents = interface.findEntityRootParents(rootHandle);
    smoab::Range dimEnts = interface.findEntitiesWithTag( tag,
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
      bool remove = !visible[c];
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
}

bool extract_subset::extract(std::string fin, std::string fout,
                             std::vector< std::vector<bool> > const& visible,
                             std::vector<std::string> const& remove_material)
{
  smoab::Interface interface(fin);
  { // volumns
    smoab::GeomTag geom3Tag(3);
    remove_visiblity(interface, geom3Tag, visible[0]);
  }
  { //boundry
    //TODO:  I am not sure how to do this one, so for now I am just ignoring it
  }
  { //surfaceRoot
    smoab::GeomTag geom2Tag(2);
    remove_visiblity(interface, geom2Tag, visible[2]);
  }
  { //neumann
    smoab::NeumannTag neTag;
    remove_visiblity_and_material( interface, neTag, visible[3], remove_material );
  }
  { //Dirichlet
    smoab::DirichletTag diTag;
    remove_visiblity_and_material( interface, diTag, visible[4], remove_material );
  }
  { //material filter
    smoab::MaterialTag metTag;
    remove_visiblity_and_material( interface, metTag, visible[5], remove_material );
  }
  interface.save(fout);
  return true;
}
