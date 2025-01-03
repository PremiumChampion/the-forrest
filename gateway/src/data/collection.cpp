#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

namespace data::collection
{

    LOG_MODULE_REGISTER(data_collection);

    void data_collection(void *arg1, void *arg2, void *arg3)
    {
        while (1)
        {
            LOG_INF("Data collection thread running");
            k_sleep(K_SECONDS(1)); // Sleep for 1 second
        }
    }

    K_THREAD_DEFINE(data_collection_thread, 2048, data_collection, NULL, NULL, NULL, 5, 0, 0); // Define the data_collection_thread

    void start_thread()
    {
        k_thread_start(data_collection_thread); // Start the data_collection_thread
        LOG_INF("Data collection thread started");
    }

} // namespace data::collection
