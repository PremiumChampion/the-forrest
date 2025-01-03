#ifndef AT_PRV_HPP
#define AT_PRV_HPP

#include "common.hpp"
#include <string>
#include <vector>
#include "uart/uart.hpp"

namespace at::commands::prv
{
    std::vector<std::string> split(std::string str, char delimiter, int limit = -1);
    result _at(uart::uart_driver *driver, std::string command, std::string &response, int timeout_ms = 1000);
    
}

#endif // AT_PRV_HPP