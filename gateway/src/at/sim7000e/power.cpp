#include <bitset>
#include <zephyr/logging/log.h>

#include "at/sim7000e/power.hpp"
#include "uart/uart.hpp"
#include "at/sim7000e/common.hpp" // at::commands::sim7000e::prv::_at

LOG_MODULE_REGISTER(at_commands_power);

namespace at::commands::sim7000e::power
{
    result wake_up()
    {
        if (gpio::set(gpio::SIM7000_PWR, gpio::LOW) != gpio::OK)
        {
            return ERROR;
        }

        if (gpio::set(gpio::SIM7000_DTR, gpio::LOW) != gpio::OK)
        {
            return ERROR;
        }

        return OK;
    }

    result wait_for_boot()
    {
        // wait for the module to boot
        std::string result = "";
        while (at::commands::sim7000e::_at("AT\r\n", result) != at::commands::OK)
        {
            LOG_INF("SIM7000E not responding");
        }
        uart::uart_driver *driver = at::commands::sim7000e::get_uart_driver();
        uart::read_result uart_res;
        do
        {
            result = "";
            uart_res = driver->uart_read(result);
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

        if (at::commands::sim7000e::_at(command, result) != at::commands::OK)
        {
            return ERROR;
        }

        return OK;
    }

    result fix_baud_rate()
    {
        // set the baud rate to 115200
        std::string result = "";
        if (at::commands::sim7000e::_at("AT+IPR=115200\r\n", result) != at::commands::OK)
        {
            return ERROR;
        }

        return OK;
    }

    result set_slow_clock(bool enable)
    {
        std::string result = "";
        std::string command = "";

        if (enable)
        {
            command = "AT+CSCLK=1\r\n";
        }
        else
        {
            command = "AT+CSCLK=0\r\n";
        }

        if (at::commands::sim7000e::_at(command, result) != at::commands::OK)
        {
            return ERROR;
        }

        return OK;
    }

    result enable_PSM()
    {
        std::string result = "";
        
        // period in which the module sends RAU (Routing Area Update) messages
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

        uint8_t unit_TAU = 1;   // 1h
        uint8_t value_TAU = 24; // 24 units = 24 h
        std::string binary_TAU = std::bitset<3>(unit_TAU).to_string() + std::bitset<5>(value_TAU).to_string();

        // time in which the module remains in idle mode
        // after this time the module enters PSM
        // ActiveTime (unit)
        // value    unit    min(s)  max(s)
        // 0 ->     2sec    0       120
        // 1 ->     1min    120     1800
        // 2 ->     6min    1800    3600
        // binary_T3324 = <unit (3bit)><value (5bit)>

        uint8_t unit_T3324 = 0;  // 2 sec
        uint8_t value_T3324 = 5; // 5 units = 10 sec
        std::string binary_T3324 = std::bitset<3>(unit_T3324).to_string() + std::bitset<5>(value_T3324).to_string();

        if (at::commands::sim7000e::_at("AT+CPSMS=1,,,\"" + binary_TAU + "\",\"" + binary_T3324 + "\"\r\n", result) != at::commands::OK)
        {
            return ERROR;
        }

        return OK;
    }

    result enter_idle_mode()
    {
        std::string result = "";
        if (at::commands::sim7000e::_at("AT+CFUN=0\r\n", result) != at::commands::OK)
        {
            return ERROR;
        }

        return OK;
    }

    result wait_for_enter_psm(k_timeout_t timeout)
    {
        std::string result = "";
        uart::uart_driver *driver = at::commands::sim7000e::get_uart_driver();
        uart::read_result uart_res;

        result = "";
        uart_res = driver->uart_read(result, timeout);
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

        return TIMEOUT;
    }

    result wait_for_exit_psm(k_timeout_t timeout)
    {
        std::string result = "";
        uart::uart_driver *driver = at::commands::sim7000e::get_uart_driver();
        uart::read_result uart_res;
        result = "";

        uart_res = driver->uart_read(result);
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

        return TIMEOUT;
    }

    result wait_for_enter_exit_psm(bool &entered_psm, k_timeout_t timeout){
        std::string result = "";
        uart::uart_driver *driver = at::commands::sim7000e::get_uart_driver();
        uart::read_result uart_res;
        result = "";

        uart_res = driver->uart_read(result, timeout);
        if (uart_res == uart::read_result::UART_READ_OK)
        {
            if (result.find("+CPSMSTATUS: \"EXIT PSM\"") != std::string::npos)
            {
                entered_psm = false;
                return OK;
            }

            if (result.find("+CPSMSTATUS: \"ENTER PSM\"") != std::string::npos)
            {
                entered_psm = true;
                return OK;
            }
        }

        if (uart_res == uart::read_result::UART_READ_ERROR)
        {
            return ERROR;
        }

        return TIMEOUT;
    }

    result exit_PSM()
    {
        std::string result = "";

        if (at::commands::sim7000e::_at("AT+CPSMS=0\r\n", result) != at::commands::OK)
        {
            return ERROR;
        }

        return OK;
    }

    result power_off(bool urgent)
    {
        std::string result = "";

        // if (at::commands::sim7000e::_at("AT+CPOF\r\n", result) != at::commands::OK)
        // {
        //     return ERROR;
        // }
        uart::uart_driver *driver = at::commands::sim7000e::get_uart_driver();
        result = "";
        if (urgent)
        {
            if (driver->uart_write("AT+CPOWD=1\r\n") != uart::write_result::UART_WRITE_OK)
            {
                return ERROR;
            }

            // we do not expect a response
        }
        else
        {
            // wait for the module to power off
            if (driver->uart_write("AT+CPOWD=1\r\n") != uart::write_result::UART_WRITE_OK)
            {
                return ERROR;
            }

            result = "";
            uart::read_result uart_res;
            while (1)
            {
                uart_res = driver->uart_read(result);
                if (uart_res == uart::read_result::UART_READ_OK)
                {
                    if (result.find("NORMAL POWER DOWN") != std::string::npos)
                    {
                        return OK;
                    }
                }

                if (uart_res == uart::read_result::UART_READ_ERROR)
                {
                    return ERROR;
                }
            }
        }

        return OK;
    }

    result set_baudrate(uart::baudrate baudrate)
    {
        std::string output = "";

        result res = at::commands::sim7000e::_at("AT+IPR=" + std::to_string(baudrate) + "\r\n", output);

        if (res != OK)
        {
            return ERROR;
        }

        uart::uart_driver *driver = at::commands::sim7000e::get_uart_driver();
        if (driver->change_baudrate(baudrate) != 0)
        {
            return ERROR;
        }

        return OK;
    }

    result reboot()
    {
        std::string response = "";
        result res = at::commands::sim7000e::_at("AT+CFUN=1,1\r\n", response);
        if (res != OK)
        {
            LOG_ERR("Failed to reboot the module");
        }
        uart::uart_driver *driver = at::commands::sim7000e::get_uart_driver();
        (void)driver->uart_read(response);

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
            uart::read_result uart_res = driver->uart_read(response);
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

} // namespace at::commands::sim7000e::power