#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <time.h>
#include <zephyr/logging/log.h>

#include "iic/bus.hpp"
#include "iic/time.hpp"

LOG_MODULE_REGISTER(app);

int main(void)

{
    if (iic::bus::setup() < 0)
    {
        LOG_ERR("I2C setup failed");
        return -1;
    }

    struct tm timeinfo;
    // Set the time components
    timeinfo.tm_year = 2025 - 1900; // Year since 1900
    timeinfo.tm_mon = 1 - 1;       // Month (0-11)
    timeinfo.tm_mday = 9;          // Day of the month (1-31)
    timeinfo.tm_hour = 16;          // Hour (0-23)
    timeinfo.tm_min = 17;           // Minutes (0-59)
    timeinfo.tm_sec = 0;           // Seconds (0-60, 60 for leap seconds)
    timeinfo.tm_isdst = 0;          // Daylight saving time flag (0 if not in effect)
    

    // if(iic::time::set_time(timeinfo)!= 0){
    //     LOG_ERR("Failed to set time");
    // }

    // LOG_INF("Time set successfully");

    while (1)
    {
        struct tm time = iic::time::get_current_time();
        LOG_INF("Current time: %d-%d-%d %d:%d:%d", time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);

        k_sleep(K_MSEC(1000));
    }

    return 0;
}
