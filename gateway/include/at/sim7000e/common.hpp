#ifndef AT_SIM7000E_COMMON_HPP
#define AT_SIM7000E_COMMON_HPP

#include <string>
#include "at/common.hpp"
#include "at/prv.hpp"

namespace at::commands::sim7000e
{
    using namespace at::commands::prv;

    uart::uart_driver *get_uart_driver();

    result _at(std::string command, std::string &response, int timeout_ms = 1000);
    result configure();
    result check_sim7000e_present();

} // namespace at::sim7000e

#endif // AT_SIM7000E_COMMON_HPP