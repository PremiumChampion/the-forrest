#include <time.h>
#include <string>
#include <vector>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "at/prv.hpp"
#include "at/common.hpp"
#include "at/http.hpp"
#include "uart/uart.hpp"

LOG_MODULE_REGISTER(at_commands_http);

namespace at::commands::sim7000e::http
{


    result check_sim7000e_present()
    {
        return prv::at();
    }
    result set_network_mode_nb_iot()
    {
        return prv::cnmp();
    }
    result enable_scrambling()
    {
        return prv::nbsc();
    }
    result get_operator(std::string &provider)
    {
        return prv::cops(provider);
    }

    result configure_bearer_profile()
    {
        std::string response = "";
        return prv::sapbr("3,1,\"APN\",\"internet\"", response);
    }

    result enable_bearer_profile(){
        std::string response = "";
        return prv::sapbr("1,1", response);
    }

    result get_ip_address(std::string &ip_address)
    {
        std::string response = "";
        result r = prv::sapbr("2,1", response);

        if (r != OK)
        {
            return r;
        }

        std::size_t configuration_start_index = response.find("+SAPBR: ");

        std::size_t configuration_end_index = response.find("\r\n", configuration_start_index);

        if (configuration_start_index == std::string::npos || configuration_end_index == std::string::npos)
        {
            return ERROR;
        }

        std::string configuration = response.substr(configuration_start_index, configuration_end_index - configuration_start_index);

        configuration = configuration.substr(8, configuration.size() - 8); // remove "+SAPBR: "

        int cid, status;
        std::string ip;

        std::vector<std::string> parts = prv::split(configuration, ',');

        if (parts.size() != 3)
        {
            return ERROR;
        }

        // 1,1,"10.x.x.x"

        cid = std::stoi(parts[0]);
        status = std::stoi(parts[1]);
        ip = parts[2].substr(1, parts[2].size() - 2); // remove quotes

        ip_address = ip;

        LOG_INF("CID: %d, Status: %d, IP: %s", cid, status, ip.c_str());

        return OK;
    }

    result start_http_session()
    {
        std::string response = "";
        return prv::httpinit();
    }

    result set_context_id_1()
    {
        std::string response = "";
        return prv::httppara("\"CID\",1", response);
    }

    result allow_redirects()
    {
        std::string response = "";
        return prv::httppara("\"REDIR\",1", response);
    }

    result set_url(std::string url)
    {
        std::string response = "";
        return prv::httppara("\"URL\",\"" + url + "\"", response);
    }

    result set_content_type(std::string content_type)
    {
        std::string response = "";
        return prv::httppara("\"USERDATA\",\"" + content_type + "\"", response);
    }

    enum body_upload_state
    {
        WAIT_FOR_DOWNLOAD,
        DOWNLOAD_IN_PROGRESS,
        DOWNLOAD_COMPLETE
    };

    result add_body(std::string body)
    {
        // we have transfere rate of 115,200 baud so we send 14,400 byte / s
        uart::write_result result;
        uart::read_result read_result;

        int64_t data_transmission_min_time_ms = body.size() / 14400 * 1000;
        int64_t transmission_timeout_ms = data_transmission_min_time_ms + std::max(int(data_transmission_min_time_ms * 1.2), 2000);

        std::string command = "AT+HTTPDATA=" + std::to_string(body.size()) + "," + std::to_string(transmission_timeout_ms);
        std::string response = "";
        LOG_INF("HTTPDATA command: %s", command.c_str());
        result = uart::uart_write(command+"\r\n");

        if (result != uart::UART_WRITE_OK)
        {
            return result::TIMEOUT;
        }

        int64_t time_ms = k_uptime_get();

        body_upload_state state = body_upload_state::WAIT_FOR_DOWNLOAD;

        while (k_uptime_get() - time_ms < transmission_timeout_ms)
        {
            switch (state)
            {
            case body_upload_state::WAIT_FOR_DOWNLOAD:
                read_result = uart::uart_read(response);

                if (read_result == uart::UART_READ_TIMEOUT)
                {
                    continue;
                }

                if (read_result == uart::UART_READ_ERROR)
                {
                    return ERROR;
                }

                if (response.find("DOWNLOAD") != std::string::npos)
                {
                    state = body_upload_state::DOWNLOAD_IN_PROGRESS;
                    k_sleep(K_MSEC(1));
                }
                break;
            case body_upload_state::DOWNLOAD_IN_PROGRESS:
                result = uart::uart_write(body);

                if (result != uart::UART_WRITE_OK)
                {
                    return result::TIMEOUT;
                }

                state = body_upload_state::DOWNLOAD_COMPLETE;
                k_sleep(K_MSEC(1));
                response = "";
                break;
            case body_upload_state::DOWNLOAD_COMPLETE:
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
                break;
            }
        }

        return OK;
    }

    result exec_post(int &http_status_code)
    {
        return prv::httpaction("1", http_status_code, 10000);
    }

    result stop_http_session()
    {
        return prv::httpterm();
    }

    result stop_baeer_profile()
    {
        std::string response = "";
        return prv::sapbr("0,1", response);
    }
}
