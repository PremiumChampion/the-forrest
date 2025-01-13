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
