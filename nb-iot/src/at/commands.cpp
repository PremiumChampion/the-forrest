#include <at/commands.hpp>
#include <uart/uart.hpp>
#include <time.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(at_commands);

namespace at::commands
{

    result _at(std::string command, std::string &response, int timeout_ms = 1000);
    std::string _escape_response(std::string response);

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

    std::string _escape_response(std::string response)
    {
        std::string escaped = "";
        for (int i = 0; i < response.size(); i++)
        {
            switch (response[i])
            {
            case '\r':
                escaped += "\\r";
                break;
            case '\n':
                escaped += "\\n";
                break;
            default:
                escaped += response[i];
                break;
            }
        }
        return escaped;
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

    result cops(std::string &provider){
        LOG_INF("COPS command");
        std::string response = "";
        result r = _at("AT+COPS?\r\n" + provider + "\"\r\n", response, 1000);

        int provider_start_index = response.find("+COPS: 0,0,\"");
        if (provider_start_index != std::string::npos)
        {
            provider = response.substr(provider_start_index + 11, response.find("\"", provider_start_index + 11) - provider_start_index - 11);
        }


        LOG_INF("Response: %s", _escape_response(response).c_str());
        return r;
    }
}
