
#include "at/sim7000e/time.hpp"
#include "at/prv.hpp"
#include "iic/time.hpp"
#include <time.h>
#include "at/common.hpp"

namespace at::commands::sim7000e::time
{
    result set_time()
    {
        // std::time_t time = std::time(nullptr);
        // struct tm *tm = std::gmtime(&time);
        struct tm time = iic::time::get_current_time();

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

    result get_time()
    {
        std::string response = "";
        return at::commands::prv::_at("AT+CCLK?\r\n", response);
    }

} // namespace at::commands::sim7000e::time
