#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "gpio/adc.hpp"
#include "iic/time.hpp"
#include "data/storage.hpp"

namespace data::collection
{

    LOG_MODULE_REGISTER(data_collection);

    void data_collection(void *arg1, void *arg2, void *arg3)
    {
        while (1)
        {
            // wait for 1 hour
            // collect new datapoints
            // add them to the storage

            // wait for 1 hour
            k_sleep(K_SECONDS(10));

            // collect new datapoints
            int32_t humidity_voltag_mv, battery_voltage_mv;
            gpio::adc::read_channel(0, humidity_voltag_mv);
            gpio::adc::read_channel(1, battery_voltage_mv);


            // get the current time
            struct tm current_time = iic::time::get_current_time();

            // create a new data point
            data_point_t data_point{
                .device_id = "gateway",
                .timestamp = current_time,
                .battery_voltage_mv = battery_voltage_mv,
                .humidity_voltage_mv = humidity_voltag_mv};
            
            // add the data point to the storage
            data::storage_instance.add_data_point(data_point);
        }
    }
    K_THREAD_STACK_DEFINE(data_collection_thread_stack, 2048);
    struct k_thread data_collection_thread_data;
    // K_THREAD_DEFINE(data_collection_thread, 2048, data_collection, NULL, NULL, NULL, 5, 0, 0); // Define the data_collection_thread

    void start_thread()
    {
        k_tid_t my_tid = k_thread_create(&data_collection_thread_data, data_collection_thread_stack,
                                 K_THREAD_STACK_SIZEOF(data_collection_thread_stack),
                                 data_collection,
                                 NULL, NULL, NULL,
                                 5, 0, K_NO_WAIT);

        LOG_INF("Data collection thread started");
    }

} // namespace data::collection
