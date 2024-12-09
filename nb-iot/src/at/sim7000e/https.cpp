#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <ctime>
#include <time.h>
#include <chrono>
#include "at/prv.hpp"
#include "at/sim7000e/https.hpp"
#include "uart/uart.hpp"
#include "iic/time.hpp"

LOG_MODULE_REGISTER(at_commands_https);

namespace at::commands::sim7000e::https
{
    using namespace at::commands::prv;

    result reboot()
    {
        std::string response = "";
        result res = _at("AT+CFUN=1,1\r\n", response);
        if (res != OK)
        {
            LOG_ERR("%s", escape_body(response).c_str());
        }

        uart::read_result uart_res = uart::uart_read(response);

        k_sleep(K_MSEC(2000));

        // wait for the module to reboot
        
        while (check_sim7000e_present() != at::commands::OK)
        {
            LOG_ERR("SIM7000E not responding");
        }

        // wait for the module to be ready

        while (true)
        {
            response = "";
            uart::read_result uart_res = uart::uart_read(response);
            if (uart_res == uart::read_result::UART_READ_OK)
            {
                if (response.find("SMS Ready") != std::string::npos)
                {
                    return OK;
                }
            }
        }
        

        return ERROR; 
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

    result start_ssl_session()
    {
        std::string response = "";
        return _at("AT+SHCONN\r\n", response, 10000);
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
    result read(std::string &response, int length, int timeout_ms)
    {
        if (length <= 0)
        {
            return ERROR;
        }
        
        result r = _at("AT+SHREAD=0," + std::to_string(length) + "\r\n", response);
        if (r != OK)
        {
            return r;
        }

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
            uart::read_result uart_res = uart::uart_read(response);
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
        return _at("AT+SHDISC\r\n", response);
    }
    
    result network_disconnect()
    {
        std::string response = "";
        return _at("AT+CNACT=0\r\n", response);
    }
} // namespace at::commands::sim7000e::https
