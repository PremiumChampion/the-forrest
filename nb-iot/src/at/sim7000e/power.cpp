#include "zephyr/logging/log.h"
#include "at/sim7000e/power.hpp"
#include "uart/uart.hpp"
#include <bitset>

LOG_MODULE_REGISTER(at_commands_power);

namespace at::commands::sim7000e::power
{
    result wake_up()
    {
        if (gpio::set(gpio::SIM7000_PWR, gpio::LOW) != gpio::OK)
        {
            return ERROR;
        }

        return OK;
    }

    result wait_for_boot()
    {
        // wait for the module to boot
        std::string result = "";
        while (at::commands::prv::_at("AT\r\n", result) != at::commands::OK)
        {
            LOG_INF("SIM7000E not responding");
        }
        uart::read_result uart_res;
        do
        {
            result = "";
            uart_res = uart::uart_read(result);
            if (uart_res == uart::read_result::UART_READ_OK)
            {
                if (result.find("SMS Ready") != std::string::npos)
                {
                    return OK;
                }
            }

            if (uart_res == uart::read_result::UART_READ_ERROR)
            {
                return ERROR;
            }
        } while (1);

        return OK;
    }

    result set_psm_event_report(psm_event_report_mode mode)
    {
        std::string command;
        std::string result = "";

        switch (mode)
        {
        case PSM_EVENT_REPORT_DISABLE:
            command = "AT+CPSMSTATUS=0\r\n";
            break;
        case PSM_EVENT_REPORT_ENABLE:
            command = "AT+CPSMSTATUS=1\r\n";
            break;
        }

        if (at::commands::prv::_at(command, result) != at::commands::OK)
        {
            return ERROR;
        }
    }

    result fix_baud_rate()
    {
        // set the baud rate to 115200
        std::string result = "";
        if (at::commands::prv::_at("AT+IPR=115200\r\n", result) != at::commands::OK)
        {
            return ERROR;
        }

        return OK;
    }

    result enable_PSM()
    {
        std::string result = "";

        // TrackingAreaUpdate (unit)
        // value    unit    min(s)  max(s)
        // 0 ->     10min   2400    18600
        // 1 ->     1h      21600   111600
        // 2 ->     10h     144000  1116000
        // 3 ->     2sec    0       62
        // 4 ->     30sec   90      930
        // 5 ->     1min    960     1860
        // 6 ->     320h    1152000 35712000

        // binary_TAU = <unit (3bit)><value (5bit)>

        uint8_t unit_TAU = 3;   // 2 sec // 0b010
        uint8_t value_TAU = 15; // 15 units = 30 sec // 0b1111 // max 5 bits
        std::string binary_TAU = std::bitset<3>(unit_TAU).to_string() + std::bitset<5>(value_TAU).to_string();

        // ActiveTime (unit)
        // value    unit    min(s)  max(s)
        // 0 ->     2sec    0       120
        // 1 ->     1min    120     1800
        // 2 ->     6min    1800    3600
        // binary_T3324 = <unit (3bit)><value (5bit)>

        uint8_t unit_T3324 = 0;  // 2 sec // 0b00
        uint8_t value_T3324 = 4; // 4 units = 8 sec // 0b0100 // max 5 bits
        std::string binary_T3324 = std::bitset<2>(unit_T3324).to_string() + std::bitset<4>(value_T3324).to_string();

        if (at::commands::prv::_at("AT+CPSMS=1,,,\"" + binary_TAU + "\",\"" + binary_T3324 + "\"\r\n", result) != at::commands::OK)
        {
            return ERROR;
        }

        return OK;
    }

    result wait_for_enter_psm(){
        std::string result = "";
        uart::read_result uart_res;
        do
        {
            result = "";
            uart_res = uart::uart_read(result);
            if (uart_res == uart::read_result::UART_READ_OK)
            {
                if (result.find("+CPSMSTATUS: \"ENTER PSM\"") != std::string::npos)
                {
                    return OK;
                }
            }

            if (uart_res == uart::read_result::UART_READ_ERROR)
            {
                return ERROR;
            }
        } while (1);

        return OK;
    }

    result wait_for_exit_psm(){
        std::string result = "";
        uart::read_result uart_res;
        do
        {
            result = "";
            uart_res = uart::uart_read(result);
            if (uart_res == uart::read_result::UART_READ_OK)
            {
                if (result.find("+CPSMSTATUS: \"EXIT PSM\"") != std::string::npos)
                {
                    return OK;
                }
            }

            if (uart_res == uart::read_result::UART_READ_ERROR)
            {
                return ERROR;
            }
        } while (1);

        return OK;
    }


    result exit_PSM()
    {
        std::string result = "";

        gpio::set(gpio::SIM7000_PWR, gpio::LOW);

        if (at::commands::prv::_at("AT+CPSMS=0\r\n", result) != at::commands::OK)
        {
            return ERROR;
        }

        gpio::set(gpio::SIM7000_PWR, gpio::HIGH);

        return OK;
    }

} // namespace at::commands::sim7000e::power