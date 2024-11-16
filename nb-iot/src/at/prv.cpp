#include <zephyr/logging/log.h>
#include <vector>
#include "at/prv.hpp"
#include "uart/uart.hpp"

LOG_MODULE_REGISTER(at_commands_prv);

namespace at::commands::prv
{
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

        result = uart::uart_write(command);

        if (result != uart::UART_WRITE_OK)
        {
            return result::TIMEOUT;
        }

        int64_t time_ms = k_uptime_get();
        while (k_uptime_get() - time_ms < timeout_ms)
        {

            read_result = uart::uart_read(response);

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

        return OK;
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
        LOG_INF("Response: %s", _escape_response(response).c_str());
        return r;
    }

    std::string _escape_response(std::string response)
    {
        return uart::_escape_response(response);
    }
    /**
     * @brief Set module to NB-IoT mode
     *
     * @return result
     */
    result cnmp()
    {
        LOG_INF("CNMP command");
        std::string response = "";
        result r = _at("AT+CNMP=38\r\n", response, 1000);
        LOG_INF("Response: %s", _escape_response(response).c_str());
        return r;
    }

    /**
     * @brief Enable Scrambling to improve signal quality
     *
     */
    result nbsc()
    {
        LOG_INF("NBSC command");
        std::string response = "";
        result r = _at("AT+NBSC=1\r\n", response, 1000);
        LOG_INF("Response: %s", _escape_response(response).c_str());
        return r;
    }

    result cops(std::string &provider)
    {
        LOG_INF("COPS command");
        std::string response = "";
        result r = _at("AT+COPS?\r\n", response, 1000);

        if (r != OK)
        {
            return r;
        }

        std::size_t configuration_start_index = response.find("+COPS: ");

        std::size_t configuration_end_index = response.find("\r\n", configuration_start_index);

        if (configuration_start_index == std::string::npos || configuration_end_index == std::string::npos)
        {
            return ERROR;
        }

        std::string configuration = response.substr(configuration_start_index, configuration_end_index - configuration_start_index);

        configuration = configuration.substr(7, configuration.size() - 7); // remove "+COPS: "

        std::string operator_name = "";
        int mode, format, netact;

        std::vector<std::string> parts = split(configuration, ',');

        if (parts.size() != 4)
        {
            return ERROR;
        }

        // +COPS: 0,0,"o2 - de",7
        mode = std::stoi(parts[0]);
        format = std::stoi(parts[1]);
        operator_name = parts[2];
        netact = std::stoi(parts[3]);

        provider = operator_name;

        LOG_INF("Response: %s", _escape_response(response).c_str());
        // log mode, format, operatorname, netact
        LOG_INF("Mode: %d, Format: %d, Operator: %s, Netact: %d", mode, format, operator_name.c_str(), netact);

        return r;
    }

    result sapbr(std::string param, std::string &response)
    {
        response = "";
        std::string command = "AT+SAPBR=" + param;
        LOG_INF("SAPBR command: %s", command.c_str());
        result r = _at(command + "\r\n", response, 1000);
        LOG_INF("Response: %s", _escape_response(response).c_str());
        return r;
    }

    result httpinit()
    {
        std::string response = "";
        LOG_INF("HTTPINIT command");
        result r = _at("AT+HTTPINIT\r\n", response, 1000);
        LOG_INF("Response: %s", _escape_response(response).c_str());

        return r;
    }

    result httppara(std::string param, std::string &response)
    {
        response = "";
        std::string command = "AT+HTTPPARA=" + param;
        LOG_INF("HTTPPARA command: %s", command.c_str());
        result r = _at(command + "\r\n", response, 1000);
        LOG_INF("Response: %s", _escape_response(response).c_str());
        return r;
    }

    result httpaction(std::string action, int &http_status_code, int timeout_ms)
    {
        std::string response = "";
        std::string command = "AT+HTTPACTION=" + action;
        LOG_INF("HTTPACTION command: %s", command.c_str());
        result r = _at(command + "\r\n", response, 1000);
        LOG_INF("Response: %s", _escape_response(response).c_str());

        if (r != OK)
        {
            return r;
        }

        int64_t time_ms = k_uptime_get();
        while (k_uptime_get() - time_ms < timeout_ms)
        {
            uart::read_result read_result = uart::uart_read(response);

            if (read_result == uart::UART_READ_TIMEOUT)
            {
                continue;
            }

            if (read_result == uart::UART_READ_ERROR)
            {
                return ERROR;
            }

            if (response.find("+HTTPACTION: ") != std::string::npos)
            {
                break;
            }
        }

        if (response.find("+HTTPACTION: ") == std::string::npos)
        {
            LOG_ERR("Result not found: %s", _escape_response(response).c_str());
            return ERROR;
        }

        std::size_t configuration_start_index = response.find("+HTTPACTION: ");

        std::size_t configuration_end_index = response.find("\r\n", configuration_start_index);

        if (configuration_start_index == std::string::npos || configuration_end_index == std::string::npos)
        {
            LOG_ERR("Malformed result: %s", _escape_response(response).c_str());
            return ERROR;
        }

        std::string configuration = response.substr(configuration_start_index, configuration_end_index - configuration_start_index);

        configuration = configuration.substr(13, configuration.size() - 13); // remove "+HTTPACTION: "

        std::vector<std::string> parts = split(configuration, ',');

        if (parts.size() != 3)
        {
            LOG_ERR("Malformed result: %s", _escape_response(response).c_str());
            return ERROR;
        }

        int method, status_code, data_length;

        method = std::stoi(parts[0]);
        status_code = std::stoi(parts[1]);
        data_length = std::stoi(parts[2]);

        LOG_INF("Method: %d, Status Code: %d, Data Length: %d", method, status_code, data_length);
        http_status_code = status_code;

        return OK;
    }

    result httpterm()
    {
        std::string response = "";
        LOG_INF("HTTPTERM command");
        result r = _at("AT+HTTPTERM\r\n", response, 1000);
        LOG_INF("Response: %s", _escape_response(response).c_str());

        return r;
    }
}