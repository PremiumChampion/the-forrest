#include "zephyr/logging/log.h"
#include "at/sim7000e/power.hpp"
#include "uart/uart.hpp"
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
        do {
            result = "";
            uart_res = uart::uart_read(result);
            if(uart_res == uart::read_result::UART_READ_OK)
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
} // namespace at::commands::sim7000e::power