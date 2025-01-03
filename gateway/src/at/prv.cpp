#include <zephyr/logging/log.h>
#include <vector>
#include "at/prv.hpp"
#include "uart/uart.hpp"

LOG_MODULE_REGISTER(at_commands_prv);

namespace at::commands::prv
{
    std::vector<std::string> split(std::string str, char delimiter, int limit)
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
            } while (limit == -1 || (std::size_t)limit > v.size());
            v.push_back(str.substr(start));
        }

        return v;
    }

    result _at(uart::uart_driver *driver, std::string command, std::string &response, int timeout_ms)
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
}