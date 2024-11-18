#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <ctime>
#include <time.h>
#include <chrono>
#include "at/prv.hpp"
#include "at/https.hpp"
#include "uart/uart.hpp"

LOG_MODULE_REGISTER(at_commands_https);

namespace at::commands::sim7000e::https
{
    using namespace at::commands::prv;

    result reboot()
    {
        std::string response = "";
        return _at("AT+CFUN=1,1\r\n", response);
    }

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

    result check_sim7000e_present()
    {
        std::string response = "";
        return at::commands::prv::_at("AT\r\n", response);
    }
    result setup_apn(std::string apn)
    {
        std::string response = "";
        result r = _at("AT+CNACT=1,\"internet\"\r\n", response);

        if (r != OK)
        {
            LOG_ERR("%s", escape_body(response).c_str());
            return r;
        }

        // wait for the network to be ready
        int64_t start = k_uptime_get();

        while (k_uptime_get() - start < 10000)
        {
            uart::read_result uart_res = uart::uart_read(response);
            if (uart_res == uart::read_result::UART_READ_OK)
            {
                if (response.find("+APP PDP: ACTIVE") != std::string::npos)
                {
                    return OK;
                }
            }
        }

        return TIMEOUT;
    }
    result get_ip(std::string &ip)
    {
        std::string response = "";
        result r = _at("AT+CNACT?\r\n", response);

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

        std::vector<std::string> parts = split(status, ',');
        if (parts.size() != 2)
        {
            return ERROR;
        }

        ip = parts[1].substr(1, parts[1].size() - 2); // remove quotes

        return OK;
    }
    result ignore_ssl_timestamp()
    {
        std::string response = "";
        return _at("AT+CSSLCFG=\"ignorertctime\",1,1\r\n", response);
    }
    result set_ssl_version()
    {
        std::string response = "";
        return _at("AT+CSSLCFG=\"sslversion\",1,3\r\n", response);
    }
    result set_server_name_indication(std::string server_name)
    {
        std::string response = "";
        return _at("AT+CSSLCFG=\"sni\",1,\"" + server_name + "\"\r\n", response);
    }
    result trust_all_certificates()
    {
        std::string response = "";
        return _at("AT+SHSSL=1,\"\"\r\n", response);
    }
    result set_body_length(int length)
    {
        std::string response = "";
        return _at("AT+SHCONF=\"BODYLEN\"," + std::to_string(length) + "\r\n", response);
    }
    result set_header_length(int length)
    {
        std::string response = "";
        return _at("AT+SHCONF=\"HEADERLEN\"," + std::to_string(length) + "\r\n", response);
    }
    result set_domain(std::string url)
    {
        std::string response = "";
        return _at("AT+SHCONF=\"URL\",\"" + url + "\"\r\n", response);
    }
    result set_time()
    {
        // std::time_t time = std::time(nullptr);
        // struct tm *tm = std::gmtime(&time);
        struct tm time = {0};
        time.tm_year = 2024 - 2000 + 100;
        time.tm_mon = 11 - 1;
        time.tm_mday = 18;
        time.tm_hour = 18;
        time.tm_min = 15;
        time.tm_sec = 58;

        struct tm *tm = &time;

        int year = tm->tm_year + 1900 - 2000; // e.g. 2021 -> 21
        int month = tm->tm_mon + 1;           // 0-indexed
        int day = tm->tm_mday;
        int hour = tm->tm_hour;
        int minute = tm->tm_min;
        int second = tm->tm_sec;

        std::string response = "";
        // AT+CCLK="24/11/16,00:04:58+00"
        return _at("AT+CCLK=\"" + std::to_string(year) + "/" + std::to_string(month) + "/" + std::to_string(day) + "," + std::to_string(hour) + ":" + std::to_string(minute) + ":" + std::to_string(second) + "+00\"\r\n", response);
    }
    result start_ssl_session()
    {
        std::string response = "";
        return _at("AT+SHCONN\r\n", response);
    }
    result clear_header()
    {
        std::string response = "";
        return _at("AT+SHCHEAD\r\n", response);
    }
    result start_header()
    {
        std::string response = "";
        return _at("AT+SHAHEAD\r\n", response);
    }
    result set_header(std::string header, std::string value)
    {
        std::string response = "";
        return _at("AT+SHAHEAD=\"" + header + "\",\"" + value + "\"\r\n", response);
    }
    result set_body(std::string body)
    {
        std::string response = "";
        std::string escaped = escape_body(body);
        return _at("AT+SHBOD=\"" + escaped + "\"," + std::to_string(body.length()) + "\r\n", response);
    }
    result exec(std::string url, http_method method, int &http_status_code, int &length)
    {
        std::string response = "";
        result r = _at("AT+SHREQ=\"" + url + "\"," + std::to_string(static_cast<int>(method)) + "\r\n", response);
        if (r != OK)
        {
            return r;
        }

        // wait for the response
        int64_t start = k_uptime_get();
        while (k_uptime_get() - start < 10000)
        {
            uart::read_result uart_res = uart::uart_read(response);
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
    result read(std::string &response, int length)
    {

        result r = _at("AT+SHREAD=0," + std::to_string(length) + "\r\n", response);
        if (r != OK)
        {
            return r;
        }

        // wait for the response
        int64_t start = k_uptime_get();
        bool in_body = false;
        while (k_uptime_get() - start < 10000)
        {
            uart::read_result uart_res = uart::uart_read(response);
            if (uart_res == uart::read_result::UART_READ_OK)
            {
                if (!in_body && response.find("+SHREAD: ") != std::string::npos)
                {
                    size_t start = response.find("+SHREAD: ");
                    size_t end = response.find("\r\n", start);

                    if (start == std::string::npos || end == std::string::npos)
                    {
                        return ERROR;
                    }

                    response = response.substr(end + 2); // remove "...\r\n+SHREAD: ...\r\n"
                    in_body = true;
                }

                if (in_body && response.length() == static_cast<size_t>(length))
                {
                    return OK;
                }
            }
        }

        return ERROR;
    }
    result stop_ssl_session()
    {
        std::string response = "";
        return _at("AT+SHDISC\r\n", response);
    }
    result network_disconnect()
    {
        std::string response = "";
        return _at("AT+CNACT=0\r\n", response);
    }
} // namespace at::commands::sim7000e::https
