#ifndef GPIO_HPP
#define GPIO_HPP

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#define SIM7000_PWR_GPIO_NODE DT_PATH(zephyr_user, sim7000_pwr_gpios)

#if DT_NODE_HAS_STATUS(SIM7000_PWR_GPIO_NODE, okay)
#define SIM7000_PWR_GPIO_CONTROLLER DT_GPIO_LABEL(SIM7000_PWR_GPIO_NODE, gpios)
#define SIM7000_PWR_GPIO_PIN DT_GPIO_PIN(SIM7000_PWR_GPIO_NODE, gpios)
#define SIM7000_PWR_GPIO_FLAGS DT_GPIO_FLAGS(SIM7000_PWR_GPIO_NODE, gpios)
#else
#error "Unsupported board: sim7000_pwr_gpios devicetree alias is not defined"
#define SIM7000_PWR_GPIO_CONTROLLER ""
#define SIM7000_PWR_GPIO_PIN 0
#define SIM7000_PWR_GPIO_FLAGS 0
#endif


namespace gpio
{
    enum gpio_result
    {
        OK,
        ERROR
    };

    enum gpio_state
    {
        LOW,
        HIGH
    };

    enum gpio_pin
    {
        SIM7000_PWR
    };

    gpio_result init();
    gpio_result set(gpio_pin pin, gpio_state value);
    gpio_result get(gpio_pin pin, gpio_state &value);
} // namespace gpio

#endif // GPIO_HPP