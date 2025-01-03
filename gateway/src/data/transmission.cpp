#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "iic/time.hpp"
#include "data/storage.hpp"
#include "at/sim7000e/common.hpp"
#include "at/sim7000e/https.hpp"
#include "at/sim7000e/power.hpp"
#include "at/sim7000e/time.hpp"

namespace data::transmission
{
    LOG_MODULE_REGISTER(data_transmission);

    bool is_in_psm = false;
    void wait_for_hour_in_PSM(int hour)
    {
        uart::uart_driver *driver = at::commands::sim7000e::get_uart_driver();
        struct tm current_time = iic::time::get_current_time();
        // get the current time
        // if the current time is the same as the hour we want to wait for
        // we wait until the next hour, then we wait for specified hour
        if (current_time.tm_hour == hour)
        {
            wait_for_hour_in_PSM((hour + 1) % 24);
        }
        // wait until the specified time
        while (1)
        {
            current_time = iic::time::get_current_time();

            // calculate the time until <hour>:00 UTC in seconds
            // check if we have passed the current hour
            int time_until_sec = (hour - current_time.tm_hour) * 3600 - current_time.tm_min * 60 - current_time.tm_sec;
            LOG_INF("Waiting until %d:00 UTC: %d seconds", hour, time_until_sec);

            // wait until <hour>:00 UTC

            driver->sleep();
            at::commands::result rc = at::commands::sim7000e::power::wait_for_enter_exit_psm(is_in_psm, K_SECONDS(time_until_sec));
            // wait for entering/exit PSM

            if (rc == at::commands::TIMEOUT)
            {
                // if we reached the time we wanted to wait for
                return;
            }

            // when we wake up via uart we need to reenable sleep mode to save power of the nrf
        }
    }

    void data_transmission(void *arg1, void *arg2, void *arg3)
    {
        // get the uart driver
        uart::uart_driver *driver = at::commands::sim7000e::get_uart_driver();

        // set the baudrate to 9600
        if (at::commands::sim7000e::power::set_baudrate(uart::baudrate::Baud9600) != at::commands::OK)
        {
            LOG_ERR("SIM7000E baudrate set failed");
        }

        while (1)
        {
            // wait until 13:00 UTC
            wait_for_hour_in_PSM(13);

            bool transmission_successful = false;
            while (!transmission_successful)
            {
                if (is_in_psm)
                {
                    driver->wakeup();

                    // wake up the module
                    if (at::commands::sim7000e::power::wake_up() != at::commands::OK)
                    {
                        LOG_ERR("SIM7000E wakeup failed");
                    }

                    if (at::commands::sim7000e::power::wait_for_exit_psm() != at::commands::OK)
                    {
                        LOG_ERR("SIM7000E wait for exit PSM failed");
                    }
                }

                // set the baudrate to 115200
                if (at::commands::sim7000e::power::set_baudrate(uart::baudrate::Baud115200) != at::commands::OK)
                {
                    LOG_ERR("SIM7000E baudrate set failed");
                }

                // send the data
                LOG_INF("Sending data");

                struct at::commands::sim7000e::https::web_request request;

                request.domain = "https://static.woyte.dev";
                request.server_name = "static.woyte.dev";
                request.method = at::commands::sim7000e::https::http_method::POST;
                request.url = "/api/save_to_db";
                request.headers = {{"Content-Type", "application/json"}, {"User-Agent", "curl/7.47.0"}, {"Cache-control", "no-cache"}, {"Accept", "*/*"}};
                request.body = data::storage_instance.to_json();
                request.http_status_code = 0;
                request.response = "";

                if (at::commands::sim7000e::https::execute_web_request(request) != at::commands::OK)
                {
                    LOG_ERR("Data transmission failed");
                    LOG_ERR("Data: %s", data::storage_instance.to_json().c_str());
                    LOG_ERR("Retying in 5 minutes");
                    k_sleep(K_SECONDS(300));
                }
                else
                {
                    LOG_INF("Data transmission successful");
                    data::storage_instance.clear();

                    if (request.http_status_code == 200)
                    {
                        transmission_successful = true;

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
                        LOG_ERR("Retying in 5 minutes");
                        k_sleep(K_SECONDS(300));
                    }
                }

                if (at::commands::sim7000e::power::set_baudrate(uart::baudrate::Baud9600) != at::commands::OK)
                {
                    LOG_ERR("SIM7000E baudrate set failed");
                }

                driver->sleep();
            }

            // wait at least until 14:00 UTC otherwise we send data indefinetely
            wait_for_hour_in_PSM(14);
        }
    }

    K_THREAD_DEFINE(data_transmission_thread, 2048, data_transmission, NULL, NULL, NULL, 5, 0, 0); // Define the data_transmission_thread

    void start_thread()
    {
        k_thread_start(data_transmission_thread); // Start the data_transmission_thread
        LOG_INF("Data transmission thread started");
    }
} // namespace data::transmission
