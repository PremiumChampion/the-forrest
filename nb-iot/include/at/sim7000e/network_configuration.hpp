#ifndef AT_NETWORK_CONFIGURATION_HPP
#define AT_NETWORK_CONFIGURATION_HPP

#include "at/common.hpp"
#include <string>

namespace at::commands::sim7000e::network_configuration{

    // functions to setup the network connection

    result setup_apn(std::string apn);
    result get_ip(std::string &ip);
    result network_disconnect();

    // functions to maintain the network connection
    // functions to disconnect the network connection
    // functions to get the network connection status
}


#endif // AT_NETWORK_CONFIGURATION_HPP