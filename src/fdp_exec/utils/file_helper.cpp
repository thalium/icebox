#include "file_helper.hpp"

void file_helper::write_file(std::string file_name, std::string s)
{
    std::ofstream outfile;
    outfile.open(file_name);
    outfile << s.data();
    outfile << std::endl;
    outfile.close();
}
