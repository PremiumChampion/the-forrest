#ifndef GPIO_HPP
#define GPIO_HPP

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

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
        SIM7000_PWR,
        SIM7000_DTR,
        ADC_CONVERTER_TRANSISTOR,
        LORA_RESET
    };

    gpio_result init();
    gpio_result set(gpio_pin pin, gpio_state value);
    gpio_result get(gpio_pin pin, gpio_state &value);
} // namespace gpio

#endif // GPIO_HPP