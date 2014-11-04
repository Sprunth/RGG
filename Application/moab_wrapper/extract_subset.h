#ifndef extract_subset_h
#define extract_subset_h

#include <vector>
#include <string>

class extract_subset
{
public:
  static bool extract(std::string fin, std::string fout,
                      std::vector< std::vector<bool> > const& visible,
                      std::vector<std::string> const& remove_material);
};

#endif
