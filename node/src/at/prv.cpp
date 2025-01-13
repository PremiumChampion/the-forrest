#include <zephyr/logging/log.h>
#include <vector>
#include "at/prv.hpp"
#include "uart/uart0.hpp"

LOG_MODULE_REGISTER(at_commands_prv);

namespace at::commands::prv
{
uart::uart0::uart0_implementation uaart0driver = uart::uart0::uart0_implementation{};
uart::uart_driver *driver = &uaart0driver;

    std::vector<std::string> split(std::string str, char delimiter)
    {
        std::vector<std::string> v;
        if (!str.empty())
        {
            int start = 0;
            do
            {
                // Find the index of occurrence
                std::size_t idx = str.find(delimiter, start);
                if (idx == std::string::npos)
                {
                    break;
                }

                // If found add the substring till that
                // occurrence in the vector
                int length = idx - start;
                v.push_back(str.substr(start, length));
                start += (length + 1); // Move the start to after the delimiter (1)
            } while (true);
            v.push_back(str.substr(start));
        }

        return v;
    }

    result _at(std::string command, std::string &response, int timeout_ms)
    {
        uart::write_result result;
        uart::read_result read_result;

        result = driver->uart_write(command);

        if (result != uart::UART_WRITE_OK)
        {
            return result::TIMEOUT;
        }

        int64_t time_ms = k_uptime_get();
        while (k_uptime_get() - time_ms < timeout_ms)
        {

            read_result = driver->uart_read(response);

            if (read_result == uart::UART_READ_TIMEOUT)
            {
                continue;
            }

            if (read_result == uart::UART_READ_ERROR)
            {
                return ERROR;
            }

            if (response.find("OK") != std::string::npos)
            {
                return OK;
            }

            if (response.find("ERROR") != std::string::npos)
            {
                return ERROR;
            }
        }

        return TIMEOUT;
    }

    
    /**
     * @brief Check for connectivity
     *
     * @return result
     */
    result at()
    {
        LOG_INF("AT command");
        std::string response = "";
        result r = _at("AT\r\n", response, 1000);
        LOG_INF("%s", uart::escape_response(response).c_str());
        return r;
    }
}