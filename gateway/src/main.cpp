#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "gpio/gpio.hpp"
#include "gpio/adc.hpp"

#include "iic/bus.hpp"
#include "iic/time.hpp"

#include "uart/uart0.hpp"
#include "uart/uart1.hpp"

#include "at/sim7000e/common.hpp"
#include "at/rylr433/rylr433.hpp"

#include "data/collection.hpp"
#include "data/transmission.hpp"
#include "data/reception.hpp"

LOG_MODULE_REGISTER(main);

int initialize();

int main(void)
{
    // first we need to initialize uart, iic, and gpio
    if (initialize() != 0)
    {
        LOG_ERR("Initialization failed");
        return -1;
    }

    // start different threads for:
    // * data collection (periodically check battery voltage, and soil humidity)
    // * data transmission (send data periodically to the server via the sim7000e)
    // * data reception (wait for data from the lora modules and store them in ram)

    data::collection::start_thread();
    data::transmission::start_thread();
    data::reception::start_thread();

    while (1)
        k_sleep(K_FOREVER);

    return 0;
}

int initialize()
{
    if (uart::uart0::uart0_driver.uart_init() != 0)
    {
        LOG_ERR("UART0 initialization failed");
        return -1;
    }

    if (uart::uart1::uart1_driver.uart_init() != 0)
    {
        LOG_ERR("UART1 initialization failed");
        return -1;
    }

    if (iic::bus::setup() != 0)
    {
        LOG_ERR("IIC initialization failed");
        return -1;
    }

    if (gpio::init() != 0)
    {
        LOG_ERR("GPIO initialization failed");
        return -1;
    }

    if(gpio::adc::setup() != 0)
    {
        LOG_ERR("ADC initialization failed");
        return -1;
    }

    // then we initalize the different modules
    // check if the DS1307 is connected
    struct tm time = iic::time::get_current_time();

    if (time.tm_year == 0 && time.tm_mon == 0 && time.tm_mday == 0)
    {
        LOG_ERR("DS1307 not found");
        return -1;
    }

    LOG_INF("DS1307 found");
    LOG_INF("Current time: %d-%d-%d %d:%d:%d", time.tm_year, time.tm_mon, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);

    // check if the SIM7000E is connected
    if (at::commands::sim7000e::configure() != at::commands::OK)
    {
        LOG_ERR("SIM7000E not found");
        return -1;
    }

    LOG_INF("SIM7000E found");

    // check if the lora module is present
    // todo: implement this

    at::commands::rylr433::init_lora_module();
    // initialisation done

    return 0;
}