#include "gpio/gpio.hpp"


namespace gpio
{
    static const struct device *gpio_dev;

    gpio_result init()
    {
        gpio_dev = device_get_binding(SIM7000_PWR_GPIO_CONTROLLER);
        if (!gpio_dev)
        {
            return ERROR;
        }

        int ret = gpio_pin_configure(gpio_dev, SIM7000_PWR_GPIO_PIN, GPIO_OUTPUT_INACTIVE | SIM7000_PWR_GPIO_FLAGS);
        if (ret != 0)
        {
            return ERROR;
        }

        return OK;
    }

    gpio_result set(gpio_pin pin, gpio_state value)
    {
        if (pin == SIM7000_PWR)
        {
            return gpio_pin_set(gpio_dev, SIM7000_PWR_GPIO_PIN, value == HIGH ? 1 : 0) == 0 ? OK : ERROR;
        }

        return ERROR;
    }

    gpio_result get(gpio_pin pin, gpio_state &value)
    {
        if (pin == SIM7000_PWR)
        {
            return gpio_pin_get(gpio_dev, SIM7000_PWR_GPIO_PIN) == 0 ? OK : ERROR;
        }
        
        return ERROR;
    }
} // namespace gpio