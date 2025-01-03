#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include "gpio/adc.hpp"
#include "gpio/gpio.hpp"

LOG_MODULE_REGISTER(gpio_adc);

namespace gpio::adc
{

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

/* The devicetree node identifier for the "led0" alias. */
#define LED1_NODE DT_NODELABEL(led1)

    /*
     * A build error on this line means your board is unsupported.
     * See the sample documentation for information on how to fix this.
     */
    static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
    !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
    ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

    /* Data of ADC io-channels specified in devicetree. */
    static const struct adc_dt_spec adc_channels[] = {
        DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
                             DT_SPEC_AND_COMMA)};

    uint16_t buf;
    struct adc_sequence sequence = {
        .buffer = &buf,
        /* buffer size in bytes, not number of samples */
        .buffer_size = sizeof(buf),
    };

    int setup()
    {
        int err;

        /* Configure channels individually prior to sampling. */
        for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++)
        {
            if (!adc_is_ready_dt(&adc_channels[i]))
            {
                LOG_ERR("ADC controller device %s not ready", adc_channels[i].dev->name);
                return -1;
            }

            err = adc_channel_setup_dt(&adc_channels[i]);
            if (err < 0)
            {
                LOG_ERR("Could not setup channel #%d (%d)", i, err);
                return -1;
            }
        }
        return 0;
    }

    void read_channel(int index, int32_t &voltage)
    {
        int err;

        (void)adc_sequence_init_dt(&adc_channels[index], &sequence);

        if (index == 1)
        {
            (void)gpio::set(gpio::ADC_CONVERTER_TRANSISTOR, gpio::HIGH);
        }

        err = adc_read_dt(&adc_channels[index], &sequence);

        if (index == 1)
        {
            (void)gpio::set(gpio::ADC_CONVERTER_TRANSISTOR, gpio::LOW);
        }

        if (err < 0)
        {
            LOG_ERR("Could not read (%d)", err);
        }

        /*
         * If using differential mode, the 16 bit value
         * in the ADC sample buffer should be a signed 2's
         * complement value.
         */
        if (adc_channels[index].channel_cfg.differential)
        {
            voltage = (int32_t)((int16_t)buf);
        }
        else
        {
            voltage = (int32_t)buf;
        }
        err = adc_raw_to_millivolts_dt(&adc_channels[index],
                                       &voltage);
        /* conversion to mV may not be supported, skip if not */
        if (err < 0)
        {
            LOG_ERR(" (value in mV not available)");
        }
    }
}
