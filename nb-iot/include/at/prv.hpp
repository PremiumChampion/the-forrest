#ifndef AT_PRV_HPP
#define AT_PRV_HPP

#include "common.hpp"
#include <string>
#include <vector>

namespace at::commands::prv
{
    std::vector<std::string> split(std::string str, char delimiter);
    result _at(std::string command, std::string &response, int timeout_ms = 1000);
    std::string _escape_response(std::string response);
    result at();
    result cnmp();
    result nbsc();
    result cops(std::string &provider);
    result httpinit();
    result httppara(std::string param, std::string &response);
    result httpaction(std::string action, int &http_status_code, int timeout_ms = 10000);
    result httpterm();
    result sapbr(std::string param, std::string &response);
}

#endif // AT_PRV_HPP