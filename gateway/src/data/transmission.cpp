#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "iic/time.hpp"
#include "data/storage.hpp"
#include "at/sim7000e/common.hpp"
#include "at/sim7000e/https.hpp"
#include "at/sim7000e/power.hpp"
#include "at/sim7000e/network_configuration.hpp"
#include "at/sim7000e/time.hpp"

namespace data::transmission
{
    LOG_MODULE_REGISTER(data_transmission);

    void data_transmission(void *arg1, void *arg2, void *arg3)
    {
        auto driver = at::commands::sim7000e::get_uart_driver();
        while (1)
        {

#if defined(CONFIG_SEMCON_DEMO_MODE)
            k_sleep(K_SECONDS(30));
#endif

            int web_requests = 0;
            // send the data
            while (data::storage_instance.size() > 0)
            {
                LOG_INF("Sending request %d", ++web_requests);
                int included_data_points = 0;
                struct at::commands::sim7000e::https::web_request request;

                request.domain = "https://static.woyte.dev";
                request.server_name = "static.woyte.dev";
                request.method = at::commands::sim7000e::https::http_method::POST;
                request.url = "/api/save_to_db";
                request.headers = {{"Content-Type", "application/json"}, {"User-Agent", "curl/7.47.0"}, {"Cache-control", "no-cache"}, {"Accept", "*/*"}};
                request.body = data::storage_instance.to_json(included_data_points);
                request.http_status_code = 0;
                request.response = "";
                if (request.body.length() > 5)
                {
                    if (at::commands::sim7000e::https::execute_web_request(request) != at::commands::OK)
                    {
                        LOG_ERR("Data transmission failed");
                        // LOG_ERR("Data: %s", request.body.c_str());
                    }
                    else
                    {
                        LOG_INF("Data transmission successful");

                        if (request.http_status_code == 200)
                        {
                            // synchronise local time with server response
                            // the body contains the current time in the format YYYY-MM-DDTHH:MM:SSZ
                            // e.g. 2025-01-03T23:21:29Z
                            // we need to convert this to a struct tm
                            struct tm time;
                            time.tm_year = std::stoi(request.response.substr(0, 4)) - 1900;
                            time.tm_mon = std::stoi(request.response.substr(5, 2)) - 1;
                            time.tm_mday = std::stoi(request.response.substr(8, 2));
                            time.tm_hour = std::stoi(request.response.substr(11, 2));
                            time.tm_min = std::stoi(request.response.substr(14, 2));
                            time.tm_sec = std::stoi(request.response.substr(17, 2));

                            // set the time
                            if (iic::time::set_time(time) != 0)
                            {
                                LOG_ERR("Failed to set time");
                            }

                            LOG_INF("Time set to %s", request.response.c_str());

                            // synchronise the time with sim7000e
                            if (at::commands::sim7000e::time::set_time() != at::commands::OK)
                            {
                                LOG_ERR("Failed to set time on SIM7000E");
                            }
                        }
                        else
                        {
                            LOG_ERR("Server responded with status code %d", request.http_status_code);
                        }
                    }
                }
                data::storage_instance.remove_first(included_data_points);
            }

#if not defined(CONFIG_SEMCON_DEMO_MODE)
            // network disconnect
            if (at::commands::sim7000e::network_configuration::network_disconnect() != at::commands::OK)
            {
                LOG_ERR("Failed to disconnect network");
            }

            // set baud rate to 9600
            if (at::commands::sim7000e::power::set_baudrate(uart::baudrate::Baud9600) != at::commands::OK)
            {
                LOG_ERR("Failed to set baud rate");
            }

            // set gpio high to enter psm
            if (gpio::set(gpio::SIM7000_PWR, gpio::HIGH) != gpio::OK)
            {
                LOG_ERR("SIM7000E PWRKEY set failed");
            }

            // wait for the module to enter PSM
            driver->sleep();
            if (at::commands::sim7000e::power::wait_for_enter_psm(K_FOREVER) != at::commands::OK)
            {
                LOG_ERR("Failed to enter PSM");
            }

            driver->sleep();
            // wait for the module to exit PSM
            if (at::commands::sim7000e::power::wait_for_exit_psm(K_FOREVER) != at::commands::OK)
            {
                LOG_ERR("Failed to exit PSM");
            }

            // set gpio high to enter psm
            if (gpio::set(gpio::SIM7000_PWR, gpio::LOW) != gpio::OK)
            {
                LOG_ERR("SIM7000E PWRKEY set failed");
            }

            // set baud rate to 115200
            if (at::commands::sim7000e::power::set_baudrate(uart::baudrate::Baud115200) != at::commands::OK)
            {
                LOG_ERR("Failed to set baud rate");
            }

            // network reconnect
            while (at::commands::sim7000e::network_configuration::setup_apn("internet") != at::commands::OK)
            {
                k_sleep(K_MSEC(1500));
            }
#endif

        }
    }

    K_THREAD_STACK_DEFINE(data_transmission_thread_stack, 2048);
    struct k_thread data_transmission_thread_data;

    void start_thread()
    {
        (void)k_thread_create(&data_transmission_thread_data, data_transmission_thread_stack,
                              K_THREAD_STACK_SIZEOF(data_transmission_thread_stack),
                              data_transmission,
                              NULL, NULL, NULL,
                              5, 0, K_NO_WAIT);

        LOG_INF("Data transmission thread started");
    }
} // namespace data::transmission
