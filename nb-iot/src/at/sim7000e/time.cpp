
#include "at/sim7000e/time.hpp"
#include "at/prv.hpp"
#include "iic/time.hpp"
#include <time.h>
#include "at/common.hpp"
#include <zephyr/logging/log.h>
#include "uart/uart.hpp"

LOG_MODULE_REGISTER(at_commands_sim7000e_time);

namespace at::commands::sim7000e::time
{
    result sync_time_ntp(struct tm &time)
    {
        std::string response = "";
        result setting_response = at::commands::prv::_at("AT+CNTP=\"pool.ntp.org\"\r\n", response);

        if (setting_response != OK)
        {
            LOG_ERR("Failed to set NTP server");
            return setting_response;
        }

        result execution = at::commands::prv::_at("AT+CNTP\r\n", response);

        if (execution != OK)
        {
            LOG_ERR("Failed to execute NTP sync");
            return execution;
        }

        uart::read_result uart_res;
        do
        {
            response = "";
            uart_res = uart::uart_read(response);
            if (uart_res == uart::read_result::UART_READ_OK)
            {
                if (response.find("+CNTP: ") != std::string::npos)
                {
                    response = response.substr(7, response.size() - 7); // remove "+CNTP: "

                    std::vector<std::string> parts = at::commands::prv::split(response, ',', 2);

                    // check result code
                    if (parts[0] != "1")
                    {
                        LOG_ERR("NTP sync failed: %s", parts[1].c_str());
                        return ERROR;
                    }

                    // check if we have the time
                    if (parts.size() < 2)
                    {
                        LOG_ERR("NTP sync failed: no time");
                        return ERROR;
                    }
                    // parse the time
                    // "2021/11/24,00:04:58"

                    time.tm_year = std::stoi(parts[1].substr(0, 4)) - 1900;
                    time.tm_mon = std::stoi(parts[1].substr(5, 2)) - 1;
                    time.tm_mday = std::stoi(parts[1].substr(8, 2));
                    time.tm_hour = std::stoi(parts[1].substr(11, 2));
                    time.tm_min = std::stoi(parts[1].substr(14, 2));
                    time.tm_sec = std::stoi(parts[1].substr(17, 2));

                    return OK;
                }
            }

            if (uart_res == uart::read_result::UART_READ_ERROR)
            {
                return ERROR;
            }
        } while (1);

        return ERROR;
    }

    result set_time()
    {
        // std::time_t time = std::time(nullptr);
        // struct tm *tm = std::gmtime(&time);
        struct tm time = iic::time::get_current_time();

        time.tm_hour %= 24;

        std::string year = std::to_string(time.tm_year + 1900 - 2000); // e.g. 2021 -> 21
        std::string month = std::to_string(time.tm_mon + 1);           // 0-indexed
        std::string day = std::to_string(time.tm_mday);
        std::string hour = std::to_string(time.tm_hour);
        std::string minute = std::to_string(time.tm_min);
        std::string second = std::to_string(time.tm_sec);

        // check if wee need to pad with a zero
        if (year.length() == 1)
        {
            year = "0" + year;
        }
        if (month.length() == 1)
        {
            month = "0" + month;
        }
        if (day.length() == 1)
        {
            day = "0" + day;
        }
        if (hour.length() == 1)
        {
            hour = "0" + hour;
        }
        if (minute.length() == 1)
        {
            minute = "0" + minute;
        }
        if (second.length() == 1)
        {
            second = "0" + second;
        }

        std::string response = "";
        // AT+CCLK="24/11/16,00:04:58+00"
        return at::commands::prv::_at("AT+CCLK=\"" + year + "/" + month + "/" + day + "," + hour + ":" + minute + ":" + second + "+00\"\r\n", response);
    }

    result get_time(std::string &response)
    {
        return at::commands::prv::_at("AT+CCLK?\r\n", response);
    }

} // namespace at::commands::sim7000e::time
