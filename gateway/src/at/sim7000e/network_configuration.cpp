#include <string.h>
#include <zephyr/logging/log.h>

#include "at/sim7000e/network_configuration.hpp"
#include "at/sim7000e/common.hpp" // at::commands::sim7000e::prv::_at

LOG_MODULE_REGISTER(at_commands_network_configuration);

namespace at::commands::sim7000e::network_configuration
{
    result setup_apn(std::string apn, int timeout_ms)
    {
        std::string response = "";
        result r = at::commands::sim7000e::_at("AT+CNACT=1,\"" + apn + "\"\r\n", response, timeout_ms);

        if (r != OK)
        {
            return r;
        }
        // uart::uart_driver *driver = at::commands::sim7000e::get_uart_driver();
        // // wait for the network to be ready
        // int64_t start = k_uptime_get();
        // while (k_uptime_get() - start < timeout_ms)
        // {
        //     uart::read_result uart_res = driver->uart_read(response);
        //     if (uart_res == uart::read_result::UART_READ_OK)
        //     {
        //         if (response.find("+APP PDP: ACTIVE") != std::string::npos)
        //         {
        //             return OK;
        //         }
        //     }
        // }

        return OK;
    }
    result get_ip(std::string &ip)
    {
        std::string response = "";
        result r = at::commands::sim7000e::_at("AT+CNACT?\r\n", response);

        if (r != OK)
        {
            return r;
        }

        size_t start = response.find("+CNACT: ");
        size_t end = response.find("\r\n", start);

        if (start == std::string::npos || end == std::string::npos)
        {
            return ERROR;
        }

        std::string status = response.substr(start, end - start);
        status = status.substr(8, status.size() - 8); // remove "+CNACT: "

        std::vector<std::string> parts = at::commands::prv::split(status, ',');
        if (parts.size() != 2)
        {
            return ERROR;
        }

        ip = parts[1].substr(1, parts[1].size() - 2); // remove quotes

        return OK;
    }

    result network_disconnect()
    {
        std::string response = "";
        return at::commands::sim7000e::_at("AT+CNACT=0\r\n", response);
    }
} // namespace at::sim7000e::network_configuration