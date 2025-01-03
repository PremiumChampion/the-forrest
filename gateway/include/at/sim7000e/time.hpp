#ifndef AT_SIM7000E_TIME_HPP
#define AT_SIM7000E_TIME_HPP

#include "at/common.hpp"
#include "at/sim7000e/common.hpp" // at::commands::sim7000e::_at
#include <string>

namespace at::commands::sim7000e::time
{

    at::commands::result sync_time_ntp(struct tm &time);
    at::commands::result set_time();
    at::commands::result get_time(std::string &response);
}

#endif // AT_SIM7000E_TIME_HPP