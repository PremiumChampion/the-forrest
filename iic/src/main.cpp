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

    while (1)
    {
        struct tm time = iic::time::get_current_time();
        LOG_INF("Current time: %d-%d-%d %d:%d:%d", time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);

        k_sleep(K_MSEC(1000));
    }

    return 0;
}
