#include "gpio/gpio.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio_gpio);


namespace gpio
{
    static const struct device *gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));

    #define SIM7000E_POWER_PIN      3
    #define SIM7000E_DTR_PIN        4

    gpio_result init()
    {

        int ret;

        // Check if the GPIO device is ready
        if (!device_is_ready(gpio_dev)) {
            LOG_ERR("GPIO device is not ready\n");
            return ERROR;
        }

        // Configure the GPIO pins
        ret = gpio_pin_configure(gpio_dev, SIM7000E_POWER_PIN, GPIO_OUTPUT);
        if (ret < 0) {
            LOG_ERR("Failed to configure SIM7000E power pin\n");
            return ERROR;
        }

        ret = gpio_pin_configure(gpio_dev, SIM7000E_DTR_PIN, GPIO_OUTPUT);
        if (ret < 0) {
            LOG_ERR("Failed to configure SIM7000E DTR pin\n");
            return ERROR;
        }

        return OK;
    }

    gpio_result set(gpio_pin pin, gpio_state value)
    {
        if (pin == SIM7000_PWR)
        {
            return gpio_pin_set(gpio_dev, SIM7000E_POWER_PIN, value == HIGH ? 1 : 0) == 0 ? OK : ERROR;
        }

        if(pin == SIM7000_DTR)
        {
            return gpio_pin_set(gpio_dev, SIM7000E_DTR_PIN, value == HIGH ? 1 : 0) == 0 ? OK : ERROR;
        }

        return ERROR;
    }

    gpio_result get(gpio_pin pin, gpio_state &value)
    {
        if (pin == SIM7000_PWR)
        {
            return gpio_pin_get(gpio_dev, SIM7000E_POWER_PIN) == 0 ? OK : ERROR;
        }

        if(pin == SIM7000_DTR)
        {
            return gpio_pin_get(gpio_dev, SIM7000E_DTR_PIN) == 0 ? OK : ERROR;
        }

        return ERROR;
    }
} // namespace gpio