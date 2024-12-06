#ifndef AT_SIM7000E_TIME_HPP
#define AT_SIM7000E_TIME_HPP

#include "at/common.hpp"

namespace at::commands::sim7000e::time {

    at::commands::result set_time();
    at::commands::result get_time();
}

#endif // AT_SIM7000E_TIME_HPP