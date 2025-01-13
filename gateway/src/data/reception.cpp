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
                // +RCV=1,11,"<nodeid>,<value_sensor0>,<value_sensor1>",-42,11\r\n

                // <nodeid>,<value_sensor0>,<value_sensor1>\r\n

                // check if we got a good response
                auto start = response.find("+RCV=1");

                if(start == std::string::npos)
                {
                    continue;
                }

                start = response.find("\"", start);

                auto end = response.find("\r\n", start);
                if (end == std::string::npos)
                {
                    continue;
                }

                end = response.find("\"", start + 1);

                // extract the data
                std::string data = response.substr(start + 1, end - start -1);
                LOG_INF("Data: %s", data.c_str());
                auto temp = at::commands::prv::split(data, ',');
                
                if (temp.size() != 3)
                {
                    LOG_ERR("Invalid data received");
                    continue;
                }

                data::data_point_t datapoint{};
                datapoint.device_id = temp[0];
                datapoint.humidity_voltage_mv = std::stoi(temp[1]);
                datapoint.battery_voltage_mv = std::stoi(temp[2]);
                datapoint.timestamp = iic::time::get_current_time();

                data::storage_instance.add_data_point(datapoint);
                response = "";
            }
            // then we parse the data and store it in the storage
        }
    }

    K_THREAD_STACK_DEFINE(data_reception_thread_stack, 2048);
    struct k_thread data_reception_thread_data;

    void start_thread()
    {
        (void)k_thread_create(&data_reception_thread_data, data_reception_thread_stack,
                                         K_THREAD_STACK_SIZEOF(data_reception_thread_stack),
                                         data_reception,
                                         NULL, NULL, NULL,
                                         5, 0, K_NO_WAIT);

        LOG_INF("Data reception thread started");
    }

}
