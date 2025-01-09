#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "uart/uart.hpp"
#include "at/rylr433/rylr433.hpp"
#include "data/storage.hpp"
#include "iic/time.hpp"

namespace data::reception
{

    LOG_MODULE_REGISTER(data_reception);

    void data_reception(void *arg1, void *arg2, void *arg3)
    {
        // in here we collect data from the RYLR498 module
        uart::uart_driver *driver = at::commands::rylr433::get_uart_driver();
        std::string response = "";
        while (1)
        {
            // we wait until we recieve data from the module
            // receive data
            driver->sleep();
            if (driver->uart_read(response, K_FOREVER) == uart::UART_READ_OK)
            {
                LOG_INF("%s", uart::escape_response(response).c_str());
                auto temp = at::commands::prv::split(response, ',');
                data::data_point_t datapoint{};
                datapoint.device_id = temp[0];
                datapoint.humidity_voltage_mv = std::stoi(temp[1]);
                datapoint.battery_voltage_mv = std::stoi(temp[2]);
                datapoint.timestamp = iic::time::get_current_time();

                data::storage_instance.add_data_point(datapoint);
                response = "";
            }
            // then we parse the data and store it in the storage

            LOG_INF("Data reception thread running");
            k_sleep(K_SECONDS(1)); // Sleep for 1 second
        }
    }

    K_THREAD_DEFINE(data_reception_thread, 2048, data_reception, NULL, NULL, NULL, 5, 0, 0); // Define the data_reception_thread

    void start_thread()
    {
        k_thread_start(data_reception_thread); // Start the data_reception_thread
        LOG_INF("Data reception thread started");
    }

}
