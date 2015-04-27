#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>

#include <stdlib.h>

#include <string>
#include <map>

int main( int argc, char** argv )
{
  if(argc != 2) return 1;

  std::ifstream in(argv[1]);
  if(!in) return 1;

  std::string input_jnl_file;
  std::string output_inp;
  std::string materials;
  std::string src_file;
  std::string fixmaterials;
  std::string neumannset;
  std::string thickness;
  std::string intervals;
  std::string bias;
  std::string output_mesh;
  std::getline(in, input_jnl_file);
  std::getline(in, output_inp);
  std::getline(in, src_file);
  std::getline(in, fixmaterials);
  std::getline(in, neumannset);
  std::getline(in, thickness);
  std::getline(in, intervals);
  std::getline(in, bias);
  std::getline(in, output_mesh);

  std::map<std::string, int> material_to_id;
  std::map<std::string, int> side_to_id;
  std::ifstream jou(input_jnl_file.c_str());
  if(!jou) return 2;
  while(jou)
  {
    std::string line;
    std::getline(jou, line);
    std::stringstream ss(line);
    std::string tmp;
    ss >> tmp;
    if(tmp == "block")
    {
      int id;
      std::string ctr, dk, name;
      ss >> id >> ctr;
      if(ctr == "body")
      {
        ss >> dk >> name;
        material_to_id[name] = id;
      }
    }
    else if(tmp == "sideset")
    {
      int id;
      std::string ctr, name;
      ss >> id >> ctr >> name;
      if(ctr == "name")
      {
        std::cout << name.substr(1,name.size()-2) << std::endl;
        side_to_id[name.substr(1, name.size()-2)] = id;
      }
    }
    //else discard line
  }
  //TODO update to support multiple material sets

  std::ofstream output(output_inp.c_str());
  if(!output)
  {
    return false;
  }
  output << "MeshFile " <<  src_file << std::endl;
  output << "NeumannSet " << side_to_id[neumannset] << std::endl;
  output << "Fixmat " << material_to_id[fixmaterials] << std::endl;
  output << "Thickness " << thickness << std::endl;
  output << "Intervals " << intervals << std::endl;
  output << "Bias " << bias << std::endl;
  output << "Outfile " <<  output_mesh << std::endl;
  output << "debug 0" << std::endl;
  output << "END" << std::endl;
}
