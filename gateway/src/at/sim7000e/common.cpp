#include <zephyr/logging/log.h>

#include "uart/uart1.hpp"
#include "at/prv.hpp"
#include "at/sim7000e/common.hpp"
#include "at/sim7000e/power.hpp"
#include "at/sim7000e/https.hpp"
#include "at/sim7000e/time.hpp"
#include "at/sim7000e/network_configuration.hpp"
#include "gpio/gpio.hpp"

LOG_MODULE_REGISTER(at_sim7000e_common);

namespace at::commands::sim7000e
{
    using namespace at::commands::prv;

    uart::uart_driver *get_uart_driver()
    {
        return &uart::uart1::uart1_driver;
    }

    result _at(std::string command, std::string &response, int timeout_ms)
    {
        return at::commands::prv::_at(get_uart_driver(), command, response, timeout_ms);
    }

    result check_sim7000e_present()
    {
        std::string response = "";
        return at::commands::sim7000e::_at("AT\r\n", response);
    }

    // confgure the SIM7000E module
    // it checks if the module is responding to AT commands
    // and sets the baudrate to 115200
    result configure()
    {
        uart::uart_driver *driver = get_uart_driver();

        std::string response;
        result res;

        // available baud rates from highest to lowest
        std::vector<uart::baudrate> baudrates = {
            uart::baudrate::Baud115200,
            uart::baudrate::Baud9600,
        };

        size_t max_attempts = 10;

        bool connection_established = false;

        if (gpio::set(gpio::SIM7000_PWR, gpio::HIGH) != gpio::OK)
        {
            LOG_ERR("SIM7000E power on failed");
            return ERROR;
        }

        for (size_t i = 0; i < max_attempts && !connection_established; i++)
        {
            for (auto baudrate : baudrates)
            {
                if (driver->change_baudrate(baudrate) != 0)
                {
                    LOG_ERR("Failed to change baudrate to %d", baudrate);
                    continue;
                }

                res = _at("AT\r\n", response, 200);

                // check if wwe got a good respone from the module
                if (res == result::OK)
                {
                    connection_established = true;
                    break;
                }

                // check if we get an echo response from the module to see if we have the correct baudrate selected
                if (response.find("AT") != std::string::npos)
                {
                    connection_established = true;
                    break;
                }
            }

            LOG_ERR("Failed to reach SIM7000E module! Retrying in one second...");
            k_sleep(K_MSEC(1000));
        }

        if (!connection_established)
        {
            LOG_ERR("Failed to reach SIM7000E module!");
            return ERROR;
        }

        uart::baudrate current_baudrate;

        if (driver->get_baudrate(current_baudrate) != 0)
        {
            LOG_ERR("Failed to get current baudrate!");
            return ERROR;
        }

        if (current_baudrate != uart::baudrate::Baud115200)
        {
            // set baudrate to 115200
            if (sim7000e::power::set_baudrate(uart::baudrate::Baud115200) != result::OK)
            {
                LOG_ERR("Failed to set baudrate to 115200");
                return ERROR;
            }
        }

#ifdef CONFIG_REBOOT_SIM7000E_ON_RESET
        // reboot the module to make sure it is in a known state
        if (sim7000e::power::reboot() != result::OK)
        {
            LOG_ERR("Failed to reboot the module");
            return ERROR;
        }
#endif

        // after this we need to establish a lte connection
        // setup the apn
        if (sim7000e::network_configuration::setup_apn("internet") != result::OK)
        {
            LOG_ERR("Failed to setup APN");
            return ERROR;
        }


        // wait for the network to be ready
        std::string response = "";
        bool network_ready = false;
        int64_t start = k_uptime_get();
        while (k_uptime_get() - start < 20000 && !network_ready)
        {
            uart::read_result uart_res = driver->uart_read(response);
            if (uart_res == uart::read_result::UART_READ_OK)
            {
                if (response.find("+APP PDP: ACTIVE") != std::string::npos)
                {
                    network_ready = true;
                }
            }
        }

        response = "";

        if (!network_ready)
        {
            LOG_ERR("Failed to establish network connection");
            return ERROR;
        }

        // configure PSM 
        if (sim7000e::power::set_psm_event_report(sim7000e::power::psm_event_report_mode::PSM_EVENT_REPORT_ENABLE) != result::OK)
        {
            LOG_ERR("Failed to enable PSM event report");
        }

        // enable PSM
        if (sim7000e::power::enable_PSM() != result::OK)
        {
            LOG_ERR("Failed to enable PSM");
        }

        // configuring ssl
        if (at::commands::sim7000e::https::ignore_ssl_timestamp() != at::commands::OK)
        {
            LOG_ERR("SIM7000E SSL timestamp ignore failed");
        }

        if (at::commands::sim7000e::https::set_ssl_version() != at::commands::OK)
        {
            LOG_ERR("SIM7000E SSL version set failed");
        }

        if (at::commands::sim7000e::https::trust_all_certificates() != at::commands::OK)
        {
            LOG_ERR("SIM7000E trust all certificates failed");
        }

        // set the current time
        if (at::commands::sim7000e::time::set_time() != at::commands::OK)
        {
            LOG_ERR("SIM7000E time set failed");
        }

        return result::OK;
    }




} // namespace at::sim7000e