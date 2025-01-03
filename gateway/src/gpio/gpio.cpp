#include "gpio/gpio.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio_gpio);

namespace gpio
{
    static const struct gpio_dt_spec power_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sim7000epower), gpios, {0});
    static const struct gpio_dt_spec dtr_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sim7000edtr), gpios, {0});
    static const struct gpio_dt_spec adc_converter_transistor_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(adcconvertertransistor), gpios, {0});

    gpio_result init()
    {
        if (!gpio_is_ready_dt(&power_pin))
        {
            LOG_ERR("SIM7000E power pin is not ready\n");
            return ERROR;
        }

        if (!gpio_is_ready_dt(&dtr_pin))
        {
            LOG_ERR("SIM7000E DTR pin is not ready\n");
            return ERROR;
        }

        if (!gpio_is_ready_dt(&adc_converter_transistor_pin))
        {
            LOG_ERR("ADC converter transistor pin is not ready\n");
            return ERROR;
        }

        // Configure the GPIO pins
        gpio_pin_configure_dt(&power_pin, GPIO_OUTPUT);
        gpio_pin_configure_dt(&dtr_pin, GPIO_OUTPUT);
        gpio_pin_configure_dt(&adc_converter_transistor_pin, GPIO_OUTPUT);

        return OK;
    }

    gpio_result set(gpio_pin pin, gpio_state value)
    {
        if (pin == SIM7000_PWR)
        {
            return gpio_pin_set_dt(&power_pin, value == HIGH ? 1 : 0) == 0 ? OK : ERROR;
        }

        if (pin == SIM7000_DTR)
        {
            return gpio_pin_set_dt(&dtr_pin, value == HIGH ? 1 : 0) == 0 ? OK : ERROR;
        }

        if (pin == ADC_CONVERTER_TRANSISTOR)
        {
            return gpio_pin_set_dt(&adc_converter_transistor_pin, value == HIGH ? 1 : 0) == 0 ? OK : ERROR;
        }

        return ERROR;
    }

    gpio_result get(gpio_pin pin, gpio_state &value)
    {
        if (pin == SIM7000_PWR)
        {
            value = gpio_pin_get_dt(&power_pin) == 0 ? LOW : HIGH;
            return OK;
        }

        if (pin == SIM7000_DTR)
        {
            value = gpio_pin_get_dt(&dtr_pin) == 0 ? LOW : HIGH;
            return OK;
        }

        if (pin == ADC_CONVERTER_TRANSISTOR)
        {
            value = gpio_pin_get_dt(&adc_converter_transistor_pin) == 0 ? LOW : HIGH;
            return OK;
        }

        return ERROR;
    }
} // namespace gpio