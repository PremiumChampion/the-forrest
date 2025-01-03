#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

namespace data::reception
{

    LOG_MODULE_REGISTER(data_reception);

    void data_reception(void *arg1, void *arg2, void *arg3)
    {
        // in here we collect data from the RYLR498 module

        while (1)
        {
            // we wait until we recieve data from the module
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
