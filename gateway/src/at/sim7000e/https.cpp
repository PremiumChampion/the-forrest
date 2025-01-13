#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <ctime>
#include <time.h>
#include <chrono>

#include "at/sim7000e/common.hpp" // at::commands::sim7000e::_at
#include "at/sim7000e/https.hpp"
#include "uart/uart.hpp"
#include "iic/time.hpp"

LOG_MODULE_REGISTER(at_commands_https);

namespace at::commands::sim7000e::https
{
    using namespace at::commands::prv;

    

    std::string escape_body(std::string body)
    {
        std::string escaped = "";
        for (std::size_t i = 0; i < body.size(); i++)
        {
            if (body[i] == '\"')
            {
                escaped += "\\\"";
            }
            else
            {
                escaped += body[i];
            }
        }
        return escaped;
    }


    result ignore_ssl_timestamp()
    {
        std::string response = "";
        return at::commands::sim7000e::_at("AT+CSSLCFG=\"ignorertctime\",0,1\r\n", response);
    }

    result set_ssl_version()
    {
        std::string response = "";
        return at::commands::sim7000e::_at("AT+CSSLCFG=\"sslversion\",1,3\r\n", response);
    }
    result set_server_name_indication(std::string server_name)
    {
        std::string response = "";
        return at::commands::sim7000e::_at("AT+CSSLCFG=\"sni\",1,\"" + server_name + "\"\r\n", response);
    }
    result trust_all_certificates()
    {
        std::string response = "";
        return at::commands::sim7000e::_at("AT+SHSSL=1,\"\"\r\n", response);
    }
    result set_body_length(int length)
    {
        std::string response = "";
        return at::commands::sim7000e::_at("AT+SHCONF=\"BODYLEN\"," + std::to_string(length) + "\r\n", response);
    }
    result set_header_length(int length)
    {
        std::string response = "";
        return at::commands::sim7000e::_at("AT+SHCONF=\"HEADERLEN\"," + std::to_string(length) + "\r\n", response);
    }
    result set_domain(std::string url)
    {
        std::string response = "";
        return at::commands::sim7000e::_at("AT+SHCONF=\"URL\",\"" + url + "\"\r\n", response);
    }

    result start_ssl_session()
    {
        std::string response = "";
        return at::commands::sim7000e::_at("AT+SHCONN\r\n", response, 10000);
    }
    result clear_header()
    {
        std::string response = "";
        return at::commands::sim7000e::_at("AT+SHCHEAD\r\n", response);
    }
    result start_header()
    {
        std::string response = "";
        return at::commands::sim7000e::_at("AT+SHAHEAD\r\n", response);
    }
    result set_header(std::string header, std::string value)
    {
        std::string response = "";
        return at::commands::sim7000e::_at("AT+SHAHEAD=\"" + header + "\",\"" + value + "\"\r\n", response);
    }
    result set_body(std::string body)
    {
        std::string response = "";
        std::string escaped = escape_body(body);
        return at::commands::sim7000e::_at("AT+SHBOD=\"" + escaped + "\"," + std::to_string(body.length()) + "\r\n", response);
    }
    result exec(std::string url, http_method method, int &http_status_code, int &length)
    {
        std::string response = "";
        result r = at::commands::sim7000e::_at("AT+SHREQ=\"" + url + "\"," + std::to_string(static_cast<int>(method)) + "\r\n", response);
        if (r != OK)
        {
            return r;
        }
        uart::uart_driver *driver = at::commands::sim7000e::get_uart_driver();
        // wait for the response
        int64_t start = k_uptime_get();
        while (k_uptime_get() - start < 10000)
        {
            uart::read_result uart_res = driver->uart_read(response);
            if (uart_res == uart::read_result::UART_READ_OK)
            {
                if (response.find("+SHREQ: ") != std::string::npos)
                {
                    size_t start = response.find("+SHREQ: ");
                    size_t end = response.find("\r\n", start);

                    if (start == std::string::npos || end == std::string::npos)
                    {
                        return ERROR;
                    }

                    std::string status = response.substr(start, end - start);
                    status = status.substr(8, status.size() - 8); // remove "+SHREQ: "

                    std::vector<std::string> parts = split(status, ',');
                    if (parts.size() != 3)
                    {
                        return ERROR;
                    }

                    http_status_code = std::stoi(parts[1]);
                    length = std::stoi(parts[2]);

                    return OK;
                }
            }
        }

        return TIMEOUT;
    }
    result read(std::string &response, int length, int timeout_ms)
    {
        if (length <= 0)
        {
            return ERROR;
        }

        result r = at::commands::sim7000e::_at("AT+SHREAD=0," + std::to_string(length) + "\r\n", response);
        if (r != OK)
        {
            return r;
        }

        uart::uart_driver *driver = at::commands::sim7000e::get_uart_driver();

        // wait for the response
        int64_t start = k_uptime_get();

        // 0 - not in body
        // 1 - in body
        // 2 - has ok
        uint8_t state = 0;

        // we want to parse the following structure
        // ...\r\n+SHREAD: num_of_bytes\r\n<data>\r\nOK\r\n
        // we are interested in <data>

        std::size_t indicated_length = 0;

        while (k_uptime_get() - start < timeout_ms)
        {
            uart::read_result uart_res = driver->uart_read(response);
            if (uart_res == uart::read_result::UART_READ_OK)
            {
                if (state == 0 && response.find("+SHREAD: ") != std::string::npos)
                {
                    size_t start = response.find("+SHREAD: ");
                    size_t end = response.find("\r\n", start);

                    if (start == std::string::npos || end == std::string::npos)
                    {
                        return ERROR;
                    }

                    // extract the indicated number of bytes
                    std::string status = response.substr(start, end - start);
                    status = status.substr(9, status.size() - 9); // remove "+SHREAD: "
                    // parse the number of bytes
                    indicated_length = std::stoi(status);

                    response = response.substr(end + 2); // remove "...\r\n+SHREAD: number_of_bytes\r\n"

                    state = 1;
                }

                if (state == 1 && (response.length() >= static_cast<size_t>(length) || response.length() >= indicated_length))
                {
                    state = 2; // using this we are able to recieve OK\r\n in the response. only if we have recieved length bytes we interpret OK again
                    LOG_INF("Specified length: %d, Recieved length: %d", length, indicated_length);
                    response = response.substr(0, indicated_length);
                    return OK;
                }
            }
        }

        return ERROR;
    }

    result stop_ssl_session()
    {
        std::string response = "";
        return at::commands::sim7000e::_at("AT+SHDISC\r\n", response);
    }


    result execute_web_request(web_request &request){
        // set server name indication
        if (set_server_name_indication(request.server_name) != OK)
        {
            LOG_ERR("SIM7000E server name indication set failed");
            return ERROR;
        }

        // set body length
        if (set_body_length(request.body.length() * 1.1) != OK) // add 10% for safety
        {
            // set body length
            if (set_body_length(1024) != OK) // add 10% for safety
            {
                LOG_ERR("SIM7000E body length set failed");
                return ERROR;
            }
        }

        // calculate header length
        int header_length = 0;
        for (auto [header, value] : request.headers)
        {
            header_length += header.length() + value.length() + 5; // 5 is for the quotes and comma
        }

        // set header length
        if (set_header_length(header_length) != OK)
        {
            LOG_ERR("SIM7000E header length set failed");
            return ERROR;
        }

        // set domain
        if (set_domain(request.domain) != OK)
        {
            LOG_ERR("SIM7000E domain set failed");
            return ERROR;
        }

        // start ssl session
        if (start_ssl_session() != OK)
        {
            LOG_ERR("SIM7000E SSL session start failed");
            return ERROR;
        }

        // clear header
        if (clear_header() != OK)
        {
            LOG_ERR("SIM7000E header clear failed");
            return ERROR;
        }

        // set headers
        for (auto [header, value] : request.headers)
        {
            if (set_header(header, value) != OK)
            {
                LOG_ERR("SIM7000E header set failed");
                return ERROR;
            }
        }

        // set body
        if (set_body(request.body) != OK)
        {
            LOG_ERR("SIM7000E body set failed");
            return ERROR;
        }

        // execute request
        int length;
        if (exec(request.url, request.method, request.http_status_code, length) != OK)
        {
            LOG_ERR("SIM7000E request execution failed");
            return ERROR;
        }

        // read response
        if(read(request.response, length) != OK)
        {
            LOG_ERR("SIM7000E response read failed");
            return ERROR;
        }

        // stop ssl session
        if (stop_ssl_session() != OK)
        {
            LOG_ERR("SIM7000E SSL session stop failed");
            return ERROR;
        }

        return OK;
    }

} // namespace at::commands::sim7000e::https
