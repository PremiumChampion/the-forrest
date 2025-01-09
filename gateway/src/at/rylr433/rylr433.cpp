#include <zephyr/logging/log.h>

#include "uart/uart0.hpp"
#include "at/prv.hpp"

#include "gpio/gpio.hpp"
#include "at/rylr433/rylr433.hpp"


namespace at::commands::rylr433
{
    LOG_MODULE_REGISTER(at_rylr433);
    uart::uart0::uart0_implementation uart_driver = uart::uart0::uart0_implementation{};
    uart::uart_driver *driver = &uart_driver;

    void join_network(int network_id, int dev_address)
    {
        std::string response = "";
        LOG_INF("Setting frequency band");
        at::commands::prv::_at(driver, "AT+BAND=433000000\r\n", response);
        LOG_INF("%s", uart::escape_response(response).c_str());
        response = "";

        LOG_INF("Setting address");
        at::commands::prv::_at(driver, "AT+ADDRESS=" + std::to_string(dev_address) + "\r\n", response);
        LOG_INF("%s", uart::escape_response(response).c_str());
        response = "";

        LOG_INF("Setting network id");
        at::commands::prv::_at(driver, "AT+NETWORKID=" + std::to_string(network_id) + "\r\n", response);
        LOG_INF("%s", uart::escape_response(response).c_str());
        response = "";
    }

    void init_lora_module()
    {
        std::string response = "";
        k_sleep(K_MSEC(4000));

        gpio::set(gpio::LORA_RESET, gpio::HIGH);
        k_sleep(K_MSEC(1000));

        at::commands::result result;
        do
        {
            response = "";
            result = at::commands::prv::_at(driver, "AT\r\n", response);
            LOG_INF("Waiting for module to respond");

            switch (result)
            {
            case at::commands::result::OK:
                LOG_INF("SUCCESS");
                break;
            case at::commands::result::TIMEOUT:
                LOG_INF("TIMEOUT");
                break;
            default:
                LOG_INF("UNKNOWN");
                break;
            }

            k_sleep(K_MSEC(1000));
        } while (result != at::commands::result::OK);

        response = "";
        LOG_INF("Setting parameters");

        result = at::commands::prv::_at(driver, "AT+PARAMETER=9,7,1,12\r\n", response);

        switch (result)
        {
        case at::commands::result::OK:
            LOG_INF("SUCCESS");
            break;
        case at::commands::result::TIMEOUT:
            LOG_INF("TIMEOUT");
            break;
        default:
            LOG_INF("UNKNOWN");
            break;
        }
        LOG_INF("%s", uart::escape_response(response).c_str());
        response = "";

        LOG_INF("Setting RF power");
        at::commands::prv::_at(driver, "AT+CRFOP=22\r\n", response);
        LOG_INF("%s", uart::escape_response(response).c_str());
        response = "";

        // Start joining network
        join_network(69, 420);
    }

    uart::uart_driver *get_uart_driver()
    {
        return driver;
    }
}
