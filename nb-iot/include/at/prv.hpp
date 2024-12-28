#ifndef AT_PRV_HPP
#define AT_PRV_HPP

#include "common.hpp"
#include <string>
#include <vector>

namespace at::commands::prv
{
    std::vector<std::string> split(std::string str, char delimiter, int limit = -1);
    result _at(std::string command, std::string &response, int timeout_ms = 1000);
    
}

#endif // AT_PRV_HPP